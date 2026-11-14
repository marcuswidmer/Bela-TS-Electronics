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

    def test_controls_and_pattern_selection(self):
        self.engine.command({"type": "pot", "index": 0, "value": .75})
        self.assertAlmostEqual(self.engine.state()["pots"][0], .75)
        self.advance(60000)  # The program indication remains lit.
        self.engine.state()
        self.button(3, True)
        self.advance(441)
        state = self.engine.state()
        self.assertEqual(state["leds"][:3], [True, False, False])
        self.assertEqual(state["drums"]["patternIndex"], 1)
        self.assertFalse(state["leds"][5])
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

    def test_sequence_lock_led_and_held_button(self):
        self.advance(44)
        self.assertFalse(self.engine.state()["leds"][4])
        self.button(2, True)
        self.advance(220)
        self.assertFalse(self.engine.state()["leds"][4])
        self.advance(1)
        self.assertTrue(self.engine.state()["leds"][4])
        self.advance(88200)  # Holding does not toggle the lock again.
        self.assertTrue(self.engine.state()["leds"][4])
        self.button(2, False)
        self.advance(221)
        self.button(2, True)
        self.advance(221)
        self.engine.state()  # Drain the previously lit LED retained for polling.
        self.advance(44)
        self.assertFalse(self.engine.state()["leds"][4])

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

    def source(self, enabled=True, eq=None, frequency=110, waveform="saw", level=.65):
        self.engine.command({"type": "source", "enabled": enabled, "frequency": frequency,
                             "waveform": waveform, "level": level,
                             "eq": [0] * 7 if eq is None else eq})

    def test_drums_play_without_input_and_filter_closes_to_kick(self):
        self.assertEqual(self.engine.state()["drums"]["loaded"], 20)
        self.assertFalse(self.engine.state()["source"]["enabled"])
        self.engine.command({"type": "pot", "index": 0, "value": (8-.4)/14})
        # Allow the existing internal-tempo smoothing to settle, then a full one-bar loop.
        samples = self.render(44100*8)
        self.assertGreater(max(map(abs,samples)), .001)
        self.assertTrue(all(math.isfinite(x) and abs(x)<=1 for x in samples))
        triggers = self.engine.state()["drums"]["triggers"]
        for voice in (0,2,6):  # Kick, snare and hats form the core; other parts may rest.
            self.assertGreater(triggers[voice],0)
        self.engine.command({"type": "pot", "index": 5, "value": 0})
        self.render(11025)
        before=self.engine.state()["drums"]
        self.render(88200)
        after=self.engine.state()["drums"]
        self.assertGreater(after["triggers"][0],before["triggers"][0])
        self.assertEqual(after["triggers"][1:],before["triggers"][1:])
        self.assertEqual(after["intensities"],[1]+[0]*6)
        self.engine.command({"type":"pot","index":4,"value":0})
        self.render(44100)
        self.assertLess(max(map(abs,self.render(4410))),.00001)

    def test_loads_selected_lofi_rim_and_real_kit(self):
        with tempfile.TemporaryDirectory(prefix="lofi-rim-") as folder:
            root=Path(folder)
            project=Path(__file__).resolve().parents[1]
            (root/"minimal_lofi_sounds").symlink_to(project/"minimal_lofi_sounds", target_is_directory=True)
            # Missing rim rejects replacement and preserves the already loaded pool.
            self.assertEqual(self.engine.lib.pepper_load_samples(self.engine.handle,str(root).encode()),0)
            self.assertEqual(self.engine.state()["drums"]["loaded"],20)
            yamaha=root/"Yamaha_Electone_HS5"
            yamaha.mkdir()
            filename="use_HS5 - SnareDrum Rim.wav"
            (yamaha/filename).symlink_to(project/"Yamaha_Electone_HS5"/filename)
            self.assertEqual(self.engine.lib.pepper_load_samples(self.engine.handle,str(root).encode()),0)
            self.assertIn(b"real_kit/snare.wav", self.engine.lib.pepper_sample_error(self.engine.handle))
            self.assertEqual(self.engine.state()["drums"]["loaded"],20)
            (root/"real_kit").symlink_to(project/"real_kit", target_is_directory=True)
            self.assertEqual(self.engine.lib.pepper_load_samples(self.engine.handle,str(root).encode()),1)
            self.assertEqual(self.engine.state()["drums"]["loaded"],20)

    def test_midi_clock_overrides_pattern_tempo_and_transport(self):
        self.source()
        self.engine.command({"type": "clock", "bpm": 90, "transport": "start"})
        self.render(44100)
        state = self.engine.state()
        self.assertTrue(state["drums"]["external"])
        self.assertTrue(state["leds"][6])
        self.assertAlmostEqual(state["drums"]["bpm"], 90, places=2)
        self.assertIn(state["drums"]["step"], range(16))
        self.engine.command({"type": "pot", "index": 0, "value": 1})
        self.render(44100)
        self.assertAlmostEqual(self.engine.state()["drums"]["bpm"], 90, places=2)
        self.engine.command({"type": "clock", "bpm": 180, "transport": "tempo"})
        self.render(44100)
        # Six-clock measurement windows alternate between integer control-frame lengths.
        self.assertAlmostEqual(self.engine.state()["drums"]["bpm"], 180, delta=.1)
        self.engine.command({"type": "clock", "bpm": 180, "transport": "stop"})
        count = self.engine.state()["drums"]["triggers"]
        self.render(44100)
        self.assertEqual(self.engine.state()["drums"]["triggers"], count)
        self.engine.command({"type": "clock", "bpm": 180, "transport": "continue"})
        self.render(44100)
        self.assertGreater(self.engine.state()["drums"]["triggers"][0], count[0])

    def test_button_three_cycles_seven_grooves_with_correct_meters(self):
        self.engine.command({"type":"clock","bpm":120,"transport":"stop"})
        seen=[]
        for pattern in range(7):
            state=self.engine.state()
            self.assertEqual(state["drums"]["patternIndex"],pattern)
            self.assertEqual(state["drums"]["kit"],"Lo-fi" if pattern<3 else "Lo-fi + Yamaha rim" if pattern<6 else "Real kit")
            steps = 12 if pattern==6 else 16
            self.assertEqual(state["drums"]["stepCount"], steps)
            self.assertEqual(state["drums"]["timeSignature"], "6/8" if pattern==6 else "4/4")
            self.assertTrue(all(len(row)==steps for row in state["drums"]["pattern"]))
            seen.append(str(state["drums"]["pattern"]))
            step=state["drums"]["step"]
            self.button(3,True); self.advance(220)
            self.assertEqual(self.engine.state()["drums"]["patternIndex"],pattern)
            self.advance(1); self.advance(44100)
            state=self.engine.state()
            self.assertEqual(state["drums"]["patternIndex"],(pattern+1)%7)
            self.assertEqual(state["drums"]["step"],step)
            self.button(3,False); self.advance(221)
        self.assertEqual(len(set(seen)),7)
        self.assertEqual(self.engine.state()["drums"]["patternIndex"],0)

    def test_real_kit_six_eight_fifth_eighth_twelve_steps_and_transport(self):
        self.engine.command({"type":"clock","bpm":120,"transport":"stop"})
        for _ in range(6):
            self.button(3,True); self.advance(221)
            self.button(3,False); self.advance(221)
        before = self.engine.state()["drums"]["triggers"]
        self.engine.command({"type":"clock","bpm":120,"transport":"start"})
        samples = self.render(66150)  # Exactly one bar of 6/8 at 120 quarter-note BPM.
        state = self.engine.state()
        self.assertGreater(max(map(abs,samples)), .001)
        self.assertEqual(state["drums"]["step"],11)
        self.assertEqual([a-b for a,b in zip(state["drums"]["triggers"],before)],[1,1,1,0,5,0,3])
        for step in range(12):
            active = [voice for voice,row in enumerate(state["drums"]["pattern"]) if row[step]>0]
            expected = [0,1] if step==8 else [4] if step%2==0 else []
            if step==0: expected.insert(0,2)
            if step%4==0: expected.append(6)
            self.assertEqual(active, expected)
        self.assertFalse(state["leds"][5])  # The real kit is not a Yamaha-rim variation.
        self.render(2)
        state = self.engine.state()
        self.assertEqual(state["drums"]["step"],0)
        self.assertEqual(state["drums"]["velocities"][:2],[0,0])
        self.assertGreater(state["drums"]["velocities"][4],0)
        self.engine.command({"type":"clock","bpm":120,"transport":"stop"})
        count = self.engine.state()["drums"]["triggers"]
        self.render(44100)
        self.assertEqual(self.engine.state()["drums"]["triggers"],count)
        self.engine.command({"type":"clock","bpm":120,"transport":"continue"})
        self.render(5513)
        self.assertEqual(self.engine.state()["drums"]["step"],1)
        self.engine.command({"type":"midi","bytes":[0xf2,23,0,0xf8]})
        self.advance(4)
        self.assertEqual(self.engine.state()["drums"]["step"],11)

    def test_clock_plays_one_bar_and_song_position_seeks(self):
        self.source()
        self.render(22050)
        self.engine.command({"type": "clock", "bpm": 120, "transport": "start"})
        before = self.engine.state()["drums"]["triggers"]
        self.render(88200)
        after = self.engine.state()["drums"]
        self.assertEqual([a-b for a,b in zip(after["triggers"], before)], [4,2,2,2,2,2,8])
        self.assertEqual(after["step"], 15)
        self.assertFalse(self.engine.state()["leds"][5])
        self.engine.command({"type": "midi", "bytes": [0xf2, 28, 0, 0xf8]})
        self.advance(4)
        self.assertEqual(self.engine.state()["drums"]["step"], 12)

    def test_raw_midi_clock_with_interleaved_notes(self):
        self.engine.command({"type": "midi", "bytes": [0xfa, 0x90, 60, 100]})
        for _ in range(25):
            self.engine.command({"type": "midi", "bytes": [0xf8]})
            self.advance(1225)  # 90 BPM.
        self.assertAlmostEqual(self.engine.state()["drums"]["bpm"], 90, places=2)
        self.engine.command({"type": "midi", "bytes": [64, 0xf8, 100, 0xfc]})
        self.assertTrue(self.engine.state()["drums"]["external"])

    def test_invalid_source_clock_and_midi_commands_are_atomic(self):
        original = self.engine.state()["source"]
        for command in [
            {"type": "source", "enabled": True, "frequency": 0, "level": .5, "eq": [0]*7},
            {"type": "source", "enabled": True, "frequency": 110, "level": .5, "eq": [0]*6},
            {"type": "source", "enabled": True, "frequency": 110, "level": .5, "eq": [0]*6+[25]},
            {"type": "source", "enabled": True, "frequency": 110, "level": .5, "eq": [0]*7, "waveform": "noise"},
            {"type": "source", "enabled": True, "frequency": 8001, "level": .5, "eq": [0]*7},
            {"type": "clock", "bpm": float("nan"), "transport": "start"},
            {"type": "clock", "bpm": 120, "transport": "restart"},
            {"type": "midi", "bytes": [248, 256]},
            {"type": "midi", "bytes": [True]},
        ]:
            with self.assertRaises(ValueError):
                self.engine.command(command)
        self.assertEqual(self.engine.state()["source"], original)


if __name__ == "__main__":
    unittest.main()
