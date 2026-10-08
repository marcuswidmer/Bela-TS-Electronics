# Pepper local simulator

Run from the repository root:

```sh
python3 projects/wi_pepper/simulator/run.py
```

Open **http://127.0.0.1:8765**. Stop with Ctrl+C. Use `--port 8766` if the port
is occupied. Requires Python 3, g++, and a modern browser; no Python packages,
JavaScript dependencies, Bela hardware, or internet connection are required.
The launcher builds a temporary shared library and removes it on exit.

The panel follows the Pepper frontplate: ten LEDs at the top, eight pots in
two columns (numbered across each row), four buttons along the bottom, and
input/output jacks at the sides. Drag knobs vertically (Shift for fine control),
use their sliders, or focus a knob and use arrows, Home or End. Buttons support
pointer, touch, Space and Enter. Hold Button 0 to verify that it switches only
once; release and press again to advance through sequencer, resonator, and JUNO. The page shows the current program's
pot and button functions. All open tabs share one instrument.

This is a native simulation of `WiPepper`, `WiSequencer`, `KarplusResonator`, `JunoSynth`
and `ProgramIndicator`, not a JavaScript imitation. The host advances a 44.1 kHz
sample clock, runs sequencer controls at 22.05 kHz, and debounces buttons for
5 ms. Program 0 is the default. Each selection keeps its numbered LED lit
continuously until the next selection. LEDs 0–2 are reserved for program
indication; LEDs 3–9 retain the meter, countdown, and voice-count displays.
A disconnected browser leaves no button held after one second.

## Audio playback and MIDI keyboard

Select **JUNO** by pressing and releasing Button 0 twice. Click **Enable audio**,
raise **Volume (pot 0)**, and play the C4–C5 keyboard with mouse, touch, or
**A W S E D F T G Y H U J K**. Multiple keys play chords; Space/Enter plays a
focused key. Use **− Octave / + Octave** or **Z / X** to transpose the
keyboard from C−1–C0 up to C8–C9. Changing octave releases held keys. Notes use MIDI channel 1 and velocity 100. Release a key to send
note-off and starts the release envelope. Leaving the window sends note-off for
held keys; losing the browser connection clears sound after one second.
Button 3 (Panic) immediately stops all notes.
Volume starts at zero, matching the hardware. Attack and sustain also affect
how quickly and how long a held note sounds.

The host renders the real synth at 44.1 kHz. The browser plays its stereo float
samples through Web Audio, with a small playback buffer (roughly 70–100 ms
under normal local conditions). Click **Disable audio** to stop playback.
Each browser tab has its own audio cursor; all tabs still share the instrument
and controls. Enable audio in one tab to avoid doubled playback. Stalls discard
old samples rather than accumulating a long backlog.

Audio input and CV remain disconnected, and no microphone or hardware MIDI
port is opened. `PepperIO.hpp` supplies zero-initialized input frames; the native
host now exports processed stereo output for browser playback. The sequencer
has no internal audio source, and the resonator receives silence.

## Files and verification

- `run.py`: temporary build, native bridge, local HTTP API, simulation clock, bounded stereo stream.
- `PepperSimulator.cc`: buttons, pots, program routing, LEDs, MIDI notes, stereo rendering, C interface.
- `PepperIO.hpp`: reserved audio/CV frame interface.
- `web/`: self-contained browser panel.

The `.cc` suffix keeps the simulator out of Bela's recursive `.cpp` build.
The real-device `render.cpp` is unchanged.

```sh
python3 -m unittest discover -s projects/wi_pepper/simulator -p 'test_*.py' -v
```

Tests exercise debounce, held-button behavior, program cycling, four-flash
startup indication, pot updates, countdown routing, MIDI validation, JUNO stereo output, note release, Panic, and audio stream cursors.
