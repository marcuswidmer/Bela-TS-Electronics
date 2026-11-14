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
from urllib.parse import urlsplit, parse_qs
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parent


def build(destination):
    subprocess.run([
        "g++", "-std=c++14", "-O2", "-shared", "-fPIC",
        str(ROOT / "PepperSimulator.cc"),
        *[str(PROJECT / name) for name in
          ("WiPepper.cpp", "WiSequencer.cpp", "WiAudioDrivenDrumMachine.cpp", "KarplusResonator.cpp", "JunoSynth.cpp")],
        "-o", str(destination),
    ], check=True)


class Engine:
    def __init__(self, library):
        self.lib = ctypes.CDLL(str(library))
        self.lib.pepper_create.restype = ctypes.c_void_p
        self.lib.pepper_load_samples.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        self.lib.pepper_load_samples.restype = ctypes.c_int
        self.lib.pepper_sample_error.argtypes = [ctypes.c_void_p]
        self.lib.pepper_sample_error.restype = ctypes.c_char_p
        self.lib.pepper_pattern_velocity.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_uint]
        self.lib.pepper_pattern_velocity.restype = ctypes.c_float
        self.lib.pepper_pattern_name.argtypes = [ctypes.c_uint]
        self.lib.pepper_pattern_name.restype = ctypes.c_char_p
        self.lib.pepper_voice_name.argtypes = [ctypes.c_uint, ctypes.c_uint]
        self.lib.pepper_voice_name.restype = ctypes.c_char_p
        self.lib.pepper_drum_pattern_count.restype = ctypes.c_uint
        self.pattern_count = self.lib.pepper_drum_pattern_count()
        self.lib.pepper_drum_step_count.restype = ctypes.c_uint
        self.lib.pepper_pattern_step_count.argtypes = [ctypes.c_uint]
        self.lib.pepper_pattern_step_count.restype = ctypes.c_uint
        self.lib.pepper_drum_sample_count.restype = ctypes.c_uint
        self.step_count = self.lib.pepper_drum_step_count()
        self.step_counts = [self.lib.pepper_pattern_step_count(i) for i in range(self.pattern_count)]
        self.sample_count = self.lib.pepper_drum_sample_count()
        self.drum_patterns = [[[self.lib.pepper_pattern_velocity(pattern, voice, step)
                                for step in range(self.step_counts[pattern])] for voice in range(7)] for pattern in range(self.pattern_count)]
        self.pattern_names = [self.lib.pepper_pattern_name(i).decode() for i in range(self.pattern_count)]
        self.voice_names = [[self.lib.pepper_voice_name(pattern, voice).decode() for voice in range(7)] for pattern in range(self.pattern_count)]
        self.lib.pepper_destroy.argtypes = [ctypes.c_void_p]
        self.lib.pepper_advance.argtypes = [ctypes.c_void_p, ctypes.c_uint]
        self.lib.pepper_render.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.POINTER(ctypes.c_float)]
        self.lib.pepper_note.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint]
        self.lib.pepper_panic.argtypes = [ctypes.c_void_p]
        self.lib.pepper_source.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_float]
        self.lib.pepper_clock.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_uint]
        self.lib.pepper_midi_byte.argtypes = [ctypes.c_void_p, ctypes.c_uint]
        self.lib.pepper_drum_state.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
        self.audio_buffer = bytearray()
        self.audio_frames = 0
        self.lib.pepper_pot.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_float]
        self.lib.pepper_button.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int]
        self.lib.pepper_state.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
        for name in ("destroy", "advance", "render", "note", "panic", "pot", "button", "state",
                     "source", "clock", "midi_byte", "drum_state"):
            getattr(self.lib, "pepper_" + name).restype = None
        self.handle = self.lib.pepper_create()
        if not self.lib.pepper_load_samples(self.handle, str(PROJECT).encode()):
            error = self.lib.pepper_sample_error(self.handle).decode()
            self.lib.pepper_destroy(self.handle)
            raise RuntimeError(error)
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
                    self.lib.pepper_panic(self.handle)
                output = (ctypes.c_float * (count * 2))()
                self.lib.pepper_render(self.handle, count, output)
                self.audio_buffer.extend(bytes(output))
                self.audio_frames += count
                del self.audio_buffer[:-8192 * 8]

    def audio(self, cursor):
        with self.lock:
            end = self.audio_frames
            start = max(end - len(self.audio_buffer) // 8, min(cursor, end))
            offset = (start - (end - len(self.audio_buffer) // 8)) * 8
            return end, bytes(self.audio_buffer[offset:])

    def state(self):
        values = (ctypes.c_float * 24)()
        drums = (ctypes.c_float * 47)()
        with self.lock:
            self.last_client = time.monotonic()
            self.lib.pepper_state(self.handle, values)
            self.lib.pepper_drum_state(self.handle, drums)
        pattern = int(drums[45])
        return {"program": int(values[0]), "pots": list(values[1:9]),
                "buttons": [bool(x) for x in values[9:13]],
                "leds": [bool(x) for x in values[13:23]],
                "indicating": bool(values[23]),
                "io": {"audio": True, "cv": False},
                "drums": {"loaded": int(drums[0]), "step": int(drums[1]), "bpm": drums[2],
                          "external": bool(drums[3]), "levels": list(drums[16:23]),
                          "triggers": [int(x) for x in drums[23:30]],
                          "intensities": list(drums[30:37]), "velocities": list(drums[37:44]),
                          "pattern": self.drum_patterns[pattern], "patternIndex": pattern,
                          "name": self.pattern_names[pattern], "patterns": self.pattern_names,
                          "kit": "Lo-fi" if pattern < 3 else "Lo-fi + Yamaha rim" if pattern < 6 else "Real kit",
                          "stepCount": self.step_counts[pattern], "timeSignature": "6/8" if pattern == 6 else "4/4", "sampleCount": self.sample_count,
                          "voices": self.voice_names[pattern], "filter": drums[46]},
                "clock": {"bpm": drums[4], "running": bool(drums[5])},
                "source": {"enabled": bool(drums[6]), "frequency": drums[7],
                           "level": drums[8], "eq": list(drums[9:16]),
                           "waveform": "sine" if drums[44] else "saw"}}

    def command(self, message):
        if not isinstance(message, dict):
            raise ValueError("Expected an object")
        kind, index = message.get("type"), message.get("index")
        if kind in ("source", "clock", "midi"):
            self.extra_command(kind, message)
            return
        if kind == "panic":
            with self.lock:
                self.lib.pepper_panic(self.handle)
            return
        if type(index) is not int:
            raise ValueError("Expected an integer index")
        with self.lock:
            self.last_client = time.monotonic()
            if kind == "note" and 0 <= index <= 127:
                velocity = message.get("velocity")
                if type(velocity) is not int or not 0 <= velocity <= 127:
                    raise ValueError("Velocity must be an integer between 0 and 127")
                self.lib.pepper_note(self.handle, index, velocity)
            elif kind == "pot" and 0 <= index < 8:
                value = message.get("value")
                if type(value) not in (float, int) or not math.isfinite(value) or not 0 <= value <= 1:
                    raise ValueError("Pot value must be between 0 and 1")
                self.lib.pepper_pot(self.handle, index, value)
            elif kind == "button" and 0 <= index < 4 and type(message.get("pressed")) is bool:
                self.lib.pepper_button(self.handle, index, message["pressed"])
            else:
                raise ValueError("Unknown control")

    def extra_command(self, kind, message):
        def number(key, low, high):
            value = message.get(key)
            if type(value) not in (int, float) or not math.isfinite(value) or not low <= value <= high:
                raise ValueError(f"{key} must be between {low} and {high}")
            return value
        if kind == "source":
            if type(message.get("enabled")) is not bool:
                raise ValueError("enabled must be a boolean")
            frequency, level = number("frequency", 35, 8000), number("level", 0, 1)
            waveform = message.get("waveform", "saw")
            if waveform not in ("saw", "sine"):
                raise ValueError("waveform must be saw or sine")
            eq = message.get("eq")
            if not isinstance(eq, list) or len(eq) != 7 or any(
                type(x) not in (int, float) or not math.isfinite(x) or not -24 <= x <= 24 for x in eq
            ):
                raise ValueError("eq must contain seven gains between -24 and 24 dB")
            with self.lock:
                self.last_client = time.monotonic()
                for index, value in enumerate([message["enabled"], frequency, level, *eq]):
                    self.lib.pepper_source(self.handle, index, value)
                self.lib.pepper_source(self.handle, 10, waveform == "sine")
        elif kind == "clock":
            bpm = number("bpm", 20, 300)
            transport = message.get("transport")
            if transport not in ("tempo", "start", "stop", "continue"):
                raise ValueError("Unknown clock transport")
            with self.lock:
                self.last_client = time.monotonic()
                self.lib.pepper_clock(self.handle, bpm, ("tempo", "start", "stop", "continue").index(transport))
        else:
            data = message.get("bytes")
            if not isinstance(data, list) or not 1 <= len(data) <= 128 or any(
                type(x) is not int or not 0 <= x <= 255 for x in data
            ):
                raise ValueError("Expected 1 to 128 MIDI bytes")
            with self.lock:
                self.last_client = time.monotonic()
                for byte in data:
                    self.lib.pepper_midi_byte(self.handle, byte)

    def close(self):
        self.stop.set()
        if self.thread.is_alive():
            self.thread.join()
        self.lib.pepper_destroy(self.handle)


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, engine, **kwargs):
        self.engine = engine
        super().__init__(*args, directory=str(ROOT / "web"), **kwargs)

    def end_headers(self):
        # The panel and native API evolve together. Never reuse an old script
        # after restarting the simulator with a newer backend.
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, *_):
        pass

    def reply(self, status, data):
        payload = json.dumps(data).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        if urlsplit(self.path).path == "/api/audio":
            try:
                cursor = int(parse_qs(urlsplit(self.path).query).get("cursor", ["0"])[0])
            except ValueError:
                self.reply(400, {"error": "Invalid audio cursor"})
                return
            end, payload = self.engine.audio(cursor)
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(len(payload)))
            self.send_header("X-Audio-Cursor", str(end))
            self.end_headers()
            self.wfile.write(payload)
        elif self.path == "/api/state":
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
