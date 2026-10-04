"""Run with: python3 -m unittest discover -s projects/wi_pepper/simulator -p 'test_*.py'"""
from pathlib import Path
import tempfile
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
            self.assertEqual(state["leds"], [True] + [False] * 9)
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
        self.button(1, True)
        self.advance(441)
        state = self.engine.state()
        self.assertEqual(state["leds"], [True] + [False] * 9)
        self.assertEqual(state["io"], {"audio": False, "cv": False})

    def test_invalid_commands(self):
        for command in [[], {}, {"type": "pot", "index": 8, "value": .5},
                        {"type": "pot", "index": 0, "value": float("nan")},
                        {"type": "pot", "index": 0, "value": 2},
                        {"type": "button", "index": 0, "pressed": "false"}]:
            with self.assertRaises(ValueError):
                self.engine.command(command)


if __name__ == "__main__":
    unittest.main()
