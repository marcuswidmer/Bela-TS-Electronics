"""Run with: python3 -m unittest discover -s projects/wi_pepper/simulator -p 'test_*.py'"""
from pathlib import Path
import tempfile
import ctypes
import math
import time
import unittest
from run import Engine, build


class SimulatorTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="pepper-test-")
        cls.library = Path(cls.temp.name) / "libpepper.so"
        build(cls.library)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.engine = Engine(self.library)

    def tearDown(self):
        self.engine.close()

    def advance(self, frames):
        self.engine.lib.pepper_advance(self.engine.handle, frames)

    def button(self, index, pressed):
        self.engine.command({"type": "button", "index": index, "pressed": pressed})

    def test_steady_startup_indicator(self):
        for _ in range(2000):
            self.advance(44)
            state = self.engine.state()
            self.assertEqual(state["leds"][:3], [True, False, False])
            self.assertTrue(state["indicating"])

    def test_button_debounce_hold_and_switch(self):
        self.button(0, True)
        self.advance(220)
        self.assertEqual(self.engine.state()["program"], 0)
        self.advance(1)
        self.assertEqual(self.engine.state()["program"], 1)
        self.advance(88200)
        state = self.engine.state()
        self.assertEqual(state["program"], 1)
        self.assertTrue(state["buttons"][0])
        self.advance(44)
        self.assertEqual(self.engine.state()["leds"], [False, True] + [False] * 8)
        self.button(0, False)
        self.advance(221)
        self.button(0, True)
        self.advance(221)
        self.assertEqual(self.engine.state()["program"], 2)
        self.button(0, False)
        self.advance(221)
        self.button(0, True)
        self.advance(221)
        self.assertEqual(self.engine.state()["program"], 0)

    def test_controls_and_countdown(self):
        self.engine.command({"type": "pot", "index": 0, "value": .75})
        self.assertAlmostEqual(self.engine.state()["pots"][0], .75)
        self.advance(60000)  # The program indication remains lit.
        self.engine.state()
        self.button(2, True)
        self.advance(441)
        state = self.engine.state()
        self.assertEqual(state["leds"][:3], [True, False, False])
        self.assertTrue(state["leds"][9])
        self.assertEqual(state["io"], {"audio": True, "cv": False})

    def test_direct_mode_led(self):
        self.button(1, True)
        self.advance(221)
        state = self.engine.state()
        self.assertEqual(state["leds"][:4], [True, False, False, True])
        self.advance(44)  # Clear pulses retained from before the mode switch.
        self.assertFalse(self.engine.state()["leds"][9])
        self.button(1, False)
        self.advance(221)
        self.button(1, True)
        self.advance(221)
        self.engine.state()  # Drain the previously lit indicator retained for polling.
        self.advance(44)
        self.assertFalse(self.engine.state()["leds"][3])

    def select_juno(self):
        for _ in range(2):
            self.button(0, True)
            self.advance(221)
            self.button(0, False)
            self.advance(221)
        for index, value in enumerate([.7, .7, .1, .5, 0, .3, .8, 0]):
            self.engine.command({"type": "pot", "index": index, "value": value})

    def render(self, count):
        output = (ctypes.c_float * (count * 2))()
        self.engine.lib.pepper_render(self.engine.handle, count, output)
        return list(output)

    def test_juno_audio_note_release_and_panic(self):
        self.select_juno()
        self.assertTrue(all(x == 0 for x in self.render(4410)))
        self.engine.command({"type": "note", "index": 60, "velocity": 100})
        samples = self.render(8820)
        self.assertTrue(all(math.isfinite(x) for x in samples))
        self.assertGreater(max(abs(x) for x in samples), .01)
        self.assertNotEqual(samples[::2], samples[1::2])  # Stereo chorus.
        self.engine.command({"type": "note", "index": 60, "velocity": 0})
        self.render(44100)
        self.assertLess(max(abs(x) for x in self.render(4410)), .0001)
        self.engine.command({"type": "note", "index": 64, "velocity": 100})
        self.render(4410)
        self.engine.command({"type": "panic"})
        self.assertTrue(all(x == 0 for x in self.render(4410)))

    def test_audio_stream_cursors_and_bounded_buffer(self):
        self.select_juno()
        self.engine.command({"type": "note", "index": 60, "velocity": 100})
        self.engine.thread.start()
        time.sleep(.25)
        cursor, data = self.engine.audio(0)
        self.assertGreater(cursor, 8192)
        self.assertGreater(len(data), 0)
        self.assertLessEqual(len(data), 8192 * 8)
        self.assertEqual(len(data) % 8, 0)
        self.assertEqual(self.engine.audio(10**12)[1], b"")
        time.sleep(.03)
        new_cursor, fresh = self.engine.audio(cursor)
        self.assertEqual(len(fresh), (new_cursor - cursor) * 8)

    def test_invalid_commands(self):
        for command in [[], {}, {"type": "pot", "index": 8, "value": .5},
                        {"type": "pot", "index": 0, "value": float("nan")},
                        {"type": "pot", "index": 0, "value": 2},
                        {"type": "button", "index": 0, "pressed": "false"},
                        {"type": "note", "index": 128, "velocity": 100},
                        {"type": "note", "index": 60, "velocity": -1},
                        {"type": "note", "index": 60, "velocity": True}]:
            with self.assertRaises(ValueError):
                self.engine.command(command)


if __name__ == "__main__":
    unittest.main()
