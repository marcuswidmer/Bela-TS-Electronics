#!/usr/bin/env python3
"""Build the real Pepper core and serve its local front panel. Python stdlib only."""
import argparse
import ctypes
import functools
import json
import math
from pathlib import Path
import subprocess
import tempfile
import threading
import time
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parent


def build(destination):
    subprocess.run([
        "g++", "-std=c++14", "-O2", "-shared", "-fPIC",
        str(ROOT / "PepperSimulator.cc"),
        *[str(PROJECT / name) for name in
          ("WiPepper.cpp", "WiSequencer.cpp", "KarplusResonator.cpp", "JunoSynth.cpp")],
        "-o", str(destination),
    ], check=True)


class Engine:
    def __init__(self, library):
        self.lib = ctypes.CDLL(str(library))
        self.lib.pepper_create.restype = ctypes.c_void_p
        self.lib.pepper_destroy.argtypes = [ctypes.c_void_p]
        self.lib.pepper_advance.argtypes = [ctypes.c_void_p, ctypes.c_uint]
        self.lib.pepper_pot.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_float]
        self.lib.pepper_button.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int]
        self.lib.pepper_state.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
        for name in ("destroy", "advance", "pot", "button", "state"):
            getattr(self.lib, "pepper_" + name).restype = None
        self.handle = self.lib.pepper_create()
        self.lock = threading.Lock()
        self.stop = threading.Event()
        self.last_client = time.monotonic()
        self.thread = threading.Thread(target=self.run, daemon=True)

    def run(self):
        previous = time.monotonic()
        remainder = 0.0
        while not self.stop.wait(0.005):
            now = time.monotonic()
            # Do not replay an arbitrarily long backlog after machine sleep.
            samples = min(now - previous, 0.1) * 44100 + remainder
            count = int(samples)
            remainder = samples - count
            previous = now
            with self.lock:
                if now - self.last_client > 1.0:
                    for i in range(4):
                        self.lib.pepper_button(self.handle, i, 0)
                self.lib.pepper_advance(self.handle, count)

    def state(self):
        values = (ctypes.c_float * 24)()
        with self.lock:
            self.last_client = time.monotonic()
            self.lib.pepper_state(self.handle, values)
        return {"program": int(values[0]), "pots": list(values[1:9]),
                "buttons": [bool(x) for x in values[9:13]],
                "leds": [bool(x) for x in values[13:23]],
                "indicating": bool(values[23]),
                "io": {"audio": False, "cv": False}}

    def command(self, message):
        if not isinstance(message, dict):
            raise ValueError("Expected an object")
        kind, index = message.get("type"), message.get("index")
        if type(index) is not int:
            raise ValueError("Expected an integer index")
        with self.lock:
            self.last_client = time.monotonic()
            if kind == "pot" and 0 <= index < 8:
                value = message.get("value")
                if type(value) not in (float, int) or not math.isfinite(value) or not 0 <= value <= 1:
                    raise ValueError("Pot value must be between 0 and 1")
                self.lib.pepper_pot(self.handle, index, value)
            elif kind == "button" and 0 <= index < 4 and type(message.get("pressed")) is bool:
                self.lib.pepper_button(self.handle, index, message["pressed"])
            else:
                raise ValueError("Unknown control")

    def close(self):
        self.stop.set()
        if self.thread.is_alive():
            self.thread.join()
        self.lib.pepper_destroy(self.handle)


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, engine, **kwargs):
        self.engine = engine
        super().__init__(*args, directory=str(ROOT / "web"), **kwargs)

    def log_message(self, *_):
        pass

    def reply(self, status, data):
        payload = json.dumps(data).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        if self.path == "/api/state":
            self.reply(200, self.engine.state())
        else:
            super().do_GET()

    def do_POST(self):
        if self.path != "/api/control":
            self.reply(404, {"error": "Unknown endpoint"})
            return
        # Local controls only; reject cross-origin browser requests.
        origin = self.headers.get("Origin")
        if origin and origin != "http://" + self.headers.get("Host", ""):
            self.reply(403, {"error": "Origin mismatch"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 1024:
                raise ValueError("Invalid request size")
            self.engine.command(json.loads(self.rfile.read(length)))
            self.reply(200, {"ok": True})
        except (ValueError, TypeError, json.JSONDecodeError) as error:
            self.reply(400, {"error": str(error)})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="pepper-simulator-") as temp:
        library = Path(temp) / "libpepper.so"
        print("Building Pepper simulator…", flush=True)
        build(library)
        engine = Engine(library)
        try:
            server = ThreadingHTTPServer(("127.0.0.1", args.port), functools.partial(Handler, engine=engine))
            engine.thread.start()
            print(f"Pepper is ready: http://127.0.0.1:{args.port}", flush=True)
            try:
                server.serve_forever()
            except KeyboardInterrupt:
                pass
            finally:
                server.server_close()
        finally:
            engine.close()


if __name__ == "__main__":
    main()
