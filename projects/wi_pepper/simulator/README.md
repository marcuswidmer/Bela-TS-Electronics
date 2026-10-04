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
continuously until the next selection, taking precedence over the program's
meter, countdown, and voice-count displays.
A disconnected browser leaves no button held after one second.

## Audio and CV: interfaces only

`PepperIO.hpp` defines zero-initialized stereo audio input/output frames, eight
CV inputs/outputs, and connection flags. Both connection flags remain false.
The native core processes silence; there is **no audio playback/capture,
file streaming, browser microphone, CV transport, or jack interaction**.
The GUI explicitly labels the ports as not connected. The sequencer still
runs its internal logic, but its CV values are not connected to external ports.
Future backends can supply/consume `PepperIO` frames inside the native host;
the panel's controls and the device's `render.cpp` do not need to change.
MIDI hardware is also not opened by the simulator. The JUNO program and its
controls are available in the panel, but playing it from a MIDI keyboard is
currently supported on the real Pepper only.

## Files and verification

- `run.py`: temporary build, native bridge, local HTTP API, simulation clock.
- `PepperSimulator.cc`: buttons, pots, program routing, LEDs, C interface.
- `PepperIO.hpp`: reserved audio/CV frame interface.
- `web/`: self-contained browser panel.

The `.cc` suffix keeps the simulator out of Bela's recursive `.cpp` build.
The real-device `render.cpp` is unchanged.

```sh
python3 -m unittest discover -s projects/wi_pepper/simulator -p 'test_*.py' -v
```

Tests exercise debounce, held-button behavior, program cycling, four-flash
startup indication, pot updates, countdown routing, and disconnected I/O.
