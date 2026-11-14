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
In sequencer mode, Button 2 toggles sequence freeze and LED 4 shows the lock.
Button 1 toggles direct MIDI mode (LED 3); Button 3 cycles seven drum patterns. LED 5 indicates the Yamaha-rim variations (patterns 4–6).
Pot 3 controls sequencer MIDI note length from 10 ms to one full step.

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

## Drum machine

Stay in **Sequencer + drums** and click **Start drums**. This enables stereo
browser audio, sets approximately 120 BPM, raises pot 4 (volume) and opens
pot 5 (filter). No audio input is required. All seven selected lo-fi samples, the Yamaha rim and twelve real-kit samples are
loaded at startup; missing/invalid selected WAVs produce a clear startup error.

The fourth panel button, **3 / Next pattern**, cycles:

1. Lo-fi Pocket — syncopated kick, backbeat and eighth-note hats
2. Lo-fi Four — four-on-the-floor kick and offbeat hats
3. Lo-fi Broken — displaced kicks, snare ghosts and hat pickups
4. Rim Pocket — pocket groove with rim backbeats and a hat pickup
5. Rim Four — quarter-note kick, rim backbeats and a kick pickup
6. Rim Broken — broken kick/hat variation with rim ghosts
7. Real Kit 6/8 — kick on 1; snare and tambourine hit on eighth note 5; shakes on 1, 2, 3, 4 and 6; triangle on 1, 3 and 5

These are the original three lo-fi patterns from the former lo-fi/Yamaha
bank, with their original note positions and accents. Three similar variations
follow, replacing the snare with Yamaha SnareDrum Rim. Patterns 1–6 are one bar
of 4/4 (16 sixteenths). Pattern 7 is one bar of 6/8 (12 sixteenths): count
six eighth notes, with snare and tambourine hit on 5, and only a shake on
1, 2, 3, 4 and 6. Triangle also plays on 1, 3 and 5, and kick plays on 1.
Intervening sixteenths are rests. All three hit and six
shake variations cycle across bars, repeating together every six bars. MIDI Start resets the variation
cycle; Song Position Pointer selects the matching bar and variation.
The display follows the selected meter and bar length. Changes preserve beat
position modulo the new bar length, and a held button advances only once.

Pot 5 progressively admits brighter voices. Closed gives kick only; opening
adds percussion and snare before cymbals and hi-hats. **Close filter · kick only**
and **Open filter · full kit** preview the endpoints. Voice meters show the
filter gains, and flashes show triggered notes. In the 6/8 groove, the closed
fader leaves tambourine hits and shakes only. Opening gradually adds triangle
(4–32%), kick (36–64%), and finally snare (68–96%). **Close filter · tambourine only**
previews its minimum. Pot 4 controls drum volume;
pots 6–7 are unused in the sequencer. The existing MIDI/CV controls on pots
0–3 retain their functions. LED 5 is lit for the rim variations.

The grooves share seven `use_`-prefixed samples in
`minimal_lofi_sounds/`: kick, tom, snare, conga, claves, percussion stab and
closed hats. Optional parts have rests; not every pattern plays every sample.
The three rim variations replace only the snare with
`Yamaha_Electone_HS5/use_HS5 - SnareDrum Rim.wav`. The 6/8 groove uses `real_kit/snare.wav`
and every `EAR_CANDY_tamourine_hit_var_*` and `EAR_CANDY_tamourine_shake_var_*` WAV,
plus `EAR_CANDY_light_triangle.wav` and `kick_marius.wav`, in `real_kit/`.
All three folders are required; 20 WAVs are loaded once,
and ringing tails retain their sample identity across pattern changes.
Closed filter leaves only the kick in patterns 1–6, or tambourine in pattern 7.

The saw/sine/EQ test source is now shown only in **KarplusResonator**, where
it excites audio input. It does not control the drum machine. No microphone
is opened; CV jacks remain placeholders. Use a standalone browser such as
Chrome; VS Code's embedded browser may not play audio.

### MIDI clock

The **Test MIDI BPM** control plus Start/Stop/Continue generates 24 PPQN inside
the native simulator, using the same raw-byte parser and tempo override as
Bela. Every sixth pulse advances one programmed sixteenth-note drum step.
Start restarts the bar; Continue resumes; Song Position Pointer seeks within
the pattern. LED 6 indicates external takeover. Stop freezes new hits and
allows tails to finish. The external override remains selected until restart;
the speed pot does not reclaim tempo when clock stops. Missing clock holds
new hits immediately. The filter changes voice intensity without changing step timing.

To use an actual Ableton MIDI output, open the simulator in a Web MIDI browser
(for example Chrome on localhost), click **Connect MIDI input**, grant browser
access, and choose the input port. Route Ableton to that input using your MIDI
interface or a virtual MIDI port and enable Sync on the corresponding Live
output. Incoming clock/transport takes over from the test-clock generator.
Web MIDI also accepts keyboard messages. HTTP/browser scheduling adds jitter;
this is a functional clock test, not a sample-accurate hardware timing test.

All tabs share pots, pattern selection, clock and optional source controls.
Browser audio has its own playback buffer. Bela audio input is unused by
the drum machine; the resonator still accepts external input.

## Files and verification

- `run.py`: temporary build, native bridge, local HTTP API, simulation clock, bounded stereo stream.
- `PepperSimulator.cc`: controls, program routing, LEDs, raw MIDI/clock, source input and stereo rendering.
- `SawInput.hpp`: saw/sine source and smoothed seven-band EQ, upstream of Pepper.
- `PepperIO.hpp`: audio/CV frame interface.
- `web/`: self-contained browser panel.

The `.cc` suffix keeps the simulator out of Bela's recursive `.cpp` build.
The real-device `render.cpp` loads the same lo-fi sample pool and routes the
processed stereo signal to Bela's audio outputs.

```sh
python3 -m unittest discover -s projects/wi_pepper/simulator -p 'test_*.py' -v
```

Tests exercise debounce, held-button behavior, program cycling, steady startup
indication, pot updates, countdown routing, MIDI validation, JUNO stereo output,
note release, Panic, audio stream cursors, drum playback, source/EQ sculpting,
seven pattern selections, 6/8 bar length and fifth-eighth accent, lo-fi/rim/real-kit loading, filter endpoints, MIDI clock override and
transport, song positions, and invalid source/clock commands.
