# Pepper programs

Button 0 (leftmost, digital pin 15) cycles between:

- Program 0: `PgmSequencer`, the default, copied from `wi_modular/WiSequencer`.
- Program 1: `PgmKarplusResonator`, the existing twelve-string resonator.
- Program 2: `PgmJuno`, a six-voice JUNO-inspired MIDI synthesizer.

LEDs 0–2 are reserved for program indication: the selected program LED stays
lit continuously and the other two stay off. LEDs 3–9 retain their normal
activity displays: input peak metering in program 1, sequencer step and priority
countdowns on LED 9 in program 0, and active-voice count in program 2. Button debouncing and program indication never block processing.

## Local GUI (no Bela required)

Run `python3 projects/wi_pepper/simulator/run.py` from the repository root,
then open **http://127.0.0.1:8765**. The frontplate provides interactive pots,
buttons, and live LEDs using the real C++ programs. Audio and CV jacks are
visible placeholders; their I/O is not implemented.
See [simulator/README.md](simulator/README.md) for controls and setup.

## Sequencer (program 0)

Pots 0–2 control speed, default sequence fraction / assigned-sequence substeps,
and random sequence, respectively.
Pot 3 controls MIDI output note length: 10 ms at minimum, one full step at
maximum, with a linear range between them. Duration is set at each Note On
using the current speed; pot changes affect subsequent notes. The next
step/substep retriggers the note if it arrives before the duration ends.
The second button (index 1) toggles direct MIDI-to-CV mode; LED 3 stays lit
while it is enabled. Each positive-velocity MIDI Note On sets CV0 to
`clamp((note - 39) / 60, 0, 1)` and triggers CV1 for 10 ms. The latest note
wins, and pitch remains after key release. Note Off and velocity-zero Note On
do not trigger. Incoming Note On and Note Off messages are also duplicated to
the computer, preserving pitch, velocity (including release velocity), and
channel; velocity-zero Note On is forwarded unchanged. CV2–3 stay zero.
Sequencing and outgoing MIDI Clock pause in
this mode; entering sends MIDI Stop. Toggling back resumes the saved sequence.
Program changes reset direct mode to off. Pots have no effect in direct mode.
The third button (index 2) toggles sequence freeze; LED 4 stays lit while locked.
Playback, gates and MIDI Clock continue, but MIDI note assignment and pattern,
variation changes are blocked. Speed, assigned-sequence substep division and
probability, and MIDI note length remain adjustable. The lock
applies immediately and survives direct mode and program changes; setup starts
unlocked. Press again to unlock. Direct MIDI-to-CV notes still work while locked.
The fourth button (index 3) toggles live MIDI note overlay in non-direct mode;
LED 5 stays lit while enabled. Incoming notes play over the running sequence
instead of assigning new steps, preserving their channel and velocity. Their
length follows incoming releases; pot 3 controls only sequence notes. The
sequence, CV outputs and MIDI Clock continue, including while locked. Shared
channel/pitch notes stay on until both the live input and sequence release them.
Turning overlay off releases its held notes. Direct mode temporarily suspends
overlay; program changes and setup reset it to off. This replaces Button 3's
MIDI resync action. Audio outputs are silent.

Analog output 0 carries pitch (note / 60, clamped to 0–1), output 1 carries
10 ms step/substep gates, and output 2 retains the source's sync output
(currently inactive because its sync callback is disabled). Other analog
outputs are zero. Sequencing runs at the actual analog sample rate, with
an audio-rate fallback if analog I/O is disabled.

MIDI input uses `hw:1,0,0`, a keyboard/interface attached to the USB-A host port.
Output uses `hw:0,0,0`, Bela's USB device connection to the computer (the USB
cable used for the IDE). Select Bela as a MIDI input device in your DAW to
receive its notes. Output is not sent to the USB-A host port. This mapping follows
[Bela's MIDI example](https://docs.bela.io/Communication_2MIDI_2render_8cpp-example.html);
`amidi -l` on Bela shows the actual device addresses.
With overlay and direct mode off, positive-velocity Note On messages
assign notes with an offset of 39. Note 0 ticks the sequencer externally;
regular incoming MIDI Clock is not used, matching the source. Output Clock
is six pulses per internal step. With direct mode off, each step/substep gate
sends a Note On on MIDI channel 1 at velocity 100, using the sequence pitch
plus 39 (clamped to 0–127), followed by Note Off after the duration set by pot 3.
CV gates remain 10 ms. Repeated pitches
retrigger; locked sequences keep sending notes. Mode/program changes, repeated
setup, and shutdown release held output notes.
Leaving the sequencer sends Stop, clears CV
outputs, and pauses sequence processing. Returning preserves assigned notes
and restores internal timing.
Missing MIDI hardware does not prevent the programs from running.

The local sequencer copy removes Bela logging, reserves space for up to 128
assigned steps during setup, and resets the step index when replacing a sequence.
Note assignment preserves the running step countdown and MIDI Clock phase;
new MIDI output notes start on the existing step/substep schedule.
Each replacement begins with the first assigned note on its next step, then
plays notes in the order received, including repeated pitches. Internal steps
and external ticks use the same ordering.
MIDI input is consumed on the render thread to avoid concurrent sequence edits.

## JUNO-inspired synthesizer (program 2)

Connect a class-compliant USB MIDI keyboard to Bela's USB-A host port (or use
a USB MIDI interface for a DIN keyboard), and listen to Pepper's stereo audio
outputs. Press Button 0 twice from startup to select program 2. MIDI input
uses `hw:1,0,0`, configured by `kMidiInputPort` in `render.cpp`; change that
constant if your interface enumerates at another ALSA address (`amidi -l`).
Synth operation requires MIDI input only; unavailable MIDI output is harmless.

The synth receives all 16 MIDI channels, tracks note ownership per channel,
and uses standard MIDI pitches (A4 = note 69 = 440 Hz; no sequencer offset).
Note On starts or retriggers a voice immediately. Note Off and zero-velocity
Note On release the matching note on its MIDI channel, following the Release
pot. Up to six notes can sound together; new notes prefer idle voices, then
quiet releasing voices, then steal the oldest voice.
CC1 adds PWM modulation, and pitch bend spans +/-2 semitones. CC64 holds
released notes while the sustain pedal is down. CC123 releases the channel's
voices; CC120 silences that channel and clears the shared chorus tail.
CC121 resets sustain, modulation and bend.
Button 3 is a global panic, useful if a keyboard disconnects with notes held.
Program changes also clear synth voices, pedal state, bend and chorus tails.

| Pot (zero-based) | Parameter |
|---|---|
| 0 | Master volume |
| 1 | Low-pass cutoff, 40 Hz–16 kHz before modulation/sample-rate limiting |
| 2 | Filter resonance |
| 3 | Filter envelope depth, 0–5 octaves |
| 4 | Attack, 2 ms–4 s |
| 5 | Decay, 20 ms–3 s |
| 6 | Sustain level |
| 7 | Release, 20 ms–8 s |

Decay/release times describe approximately 60 dB of exponential settling.
Filter cutoff also has fixed half keyboard tracking. Start with pots at
**70%, 60%, 20%, 30%, 5%, 35%, 70%, 30%**, respectively.

| Button | Action |
|---|---|
| 0 | Next program (returns to sequencer) |
| 1 | Cycle saw + pulse (default), saw, pulse |
| 2 | Cycle chorus I (default), II, off |
| 3 | Panic: silence all notes and clear the chorus |

The DSP combines anti-aliased saw/pulse oscillators, a prominent square sub oscillator one octave below the played note, light
noise, a four-pole low-pass filter, one ADSR controlling amplitude and filter,
a fixed high-pass stage, and stereo modulated-delay chorus. The chorus uses a 50% wet blend with
a widened side signal (1.5× in mode I, 1.8× in mode II); the dry signal stays centered. Parameters are
smoothed, processing uses fixed storage, and output is bounded to [-1, 1].
This is an original implementation inspired by the
[Roland JUNO-106 architecture](https://support.roland.com/hc/en-us/articles/201966419-Juno-106-Technical-Specifications),
not a circuit-accurate emulation or Roland software. The chorus is a clean
modulated delay, not a model of the original analog delay circuitry.

Audio input is unused in this program and CV outputs stay zero. The local
GUI shows the new program and controls, but desktop audio/MIDI transport
remains disconnected as before; keyboard playback is implemented on Bela.

## Resonator (program 1)

`KarplusResonator` is a self-contained adaptation of the twelve parallel
feedback delays in `../karplus_resonator`. Input 0 excites the strings; both
inputs contribute to the peak LED meter. Analog outputs are zero in this mode.

## Controls

Pots are numbered 1–8 here and use Bela analog channels 0–7 directly.

| Pot | Parameter | Range |
|---|---|---|
| 1 | Master volume | Silent to unity |
| 2 | External excitation | Off to 0.2 input gain into each string |
| 3 | Decay | Nominal 0.5–60 s, exponential |
| 4 | Brightness | Feedback low-pass cutoff 500 Hz–20 kHz (capped at 40% of sample rate) |
| 5 | Transpose | -12 to +12 semitones; A2 root at center |
| 6 | Dry/wet | Dry input to resonator only |
| 7 | Detune | No detune to +/-20 cents across strings |
| 8 | Stereo width | Mono to strings spread left/right |

| Button (zero-based, left to right) | Action |
|---|---|
| 0 | Select next program |
| 1 | Cycle chromatic, major, single note (unison), minor pentatonic note sets |
| 2 | Toggle sustain: disconnect external excitation and use long feedback |
| 3 | Clear all strings |

Buttons use active-high digital pins 15, 14, 13, 12 with 5 ms debouncing.
A held button triggers once; a button held at startup must first be released.
Sustain starts off and chromatic is the initial note set. Sustain is not an
infinite freeze: damping and soft saturation still dissipate energy.
Clear preserves the selected controls/mode. The former Button 0 pluck action
is replaced by program selection; Buttons 1–3 retain their existing actions.

Start with pot 1 halfway, pot 2 halfway, pot 3 halfway, pot 4 high,
pot 5 centered, pot 6 fully wet, and pots 7–8 low, then feed audio into input 0.
Button 3 can make a click because it immediately clears the delay lines.

## DSP details

All state is initialized in setup using the actual sample rate (22.05–96 kHz).
The fixed 2048-sample buffers cover the lowest A1 tuning at 96 kHz. Reads use
linear fractional-delay interpolation; parameters/delay lengths are smoothed
over approximately 10 ms. Pot targets update at roughly 1 kHz. The feedback
low-pass and interpolation affect pitch and actual decay, so these are musical
resonators rather than precisely calibrated oscillators or T60 controls.

Feedback stays below unity, each string has soft saturation above unity (quiet tails pass unchanged), the twelve
voices are averaged, and final outputs are clipped to [-1,1] as a guard.
At zero stereo width both outputs match. The linear pan law preserves the
sum of channel weights while spreading voices at higher width.

Processing allocates no memory and uses no file I/O, auxiliary task, or logging.
`render.cpp` owns Bela setup, audio I/O, control polling/debouncing, and LED
metering. `WiPepper` has a platform-independent interface for sample-rate setup,
pot values, button events, and per-sample audio processing.

## Verification

From the repository root:

```sh
g++ -std=c++14 -fsyntax-only -Iinclude -I. projects/wi_pepper/render.cpp projects/wi_pepper/WiPepper.cpp projects/wi_pepper/WiSequencer.cpp projects/wi_pepper/KarplusResonator.cpp projects/wi_pepper/JunoSynth.cpp
g++ -std=c++14 -O2 -Iprojects/wi_pepper projects/wi_pepper/tests/resonator_test.cc projects/wi_pepper/KarplusResonator.cpp -o /tmp/wi-pepper-resonator-test
/tmp/wi-pepper-resonator-test
g++ -std=c++14 -O2 -Iprojects/wi_pepper projects/wi_pepper/tests/programs_test.cc projects/wi_pepper/WiPepper.cpp projects/wi_pepper/WiSequencer.cpp projects/wi_pepper/KarplusResonator.cpp projects/wi_pepper/JunoSynth.cpp -o /tmp/wi-pepper-programs-test
/tmp/wi-pepper-programs-test
g++ -std=c++14 -O2 -Iprojects/wi_pepper projects/wi_pepper/tests/midi_output_test.cc projects/wi_pepper/WiPepper.cpp projects/wi_pepper/WiSequencer.cpp projects/wi_pepper/KarplusResonator.cpp projects/wi_pepper/JunoSynth.cpp -o /tmp/wi-pepper-midi-output-test
/tmp/wi-pepper-midi-output-test
g++ -std=c++14 -O2 -Iprojects/wi_pepper projects/wi_pepper/tests/juno_test.cc projects/wi_pepper/WiPepper.cpp projects/wi_pepper/WiSequencer.cpp projects/wi_pepper/KarplusResonator.cpp projects/wi_pepper/JunoSynth.cpp -o /tmp/wi-pepper-juno-test
/tmp/wi-pepper-juno-test
```

Tests cover initialization, silence, pluck energy, stereo spread, extreme
controls, changing note sets, sustain, clearing, and dry routing at four
sample rates. `.cc` keeps the test out of Bela's recursive `.cpp` build.
Program tests cover the default selection, switching, MIDI assignment and overlay,
CV clearing, audio routing, and continuous program indication at multiple sample rates.
MIDI output tests cover direct-mode forwarding, channel and velocity preservation,
sequence pitches, adjustable note lengths, retriggers, locked patterns and note cleanup
at multiple control sample rates.
They also compare CV gates and outgoing MIDI Clock against an unedited sequence
while notes are assigned between steps and at step boundaries.
Assignment-order tests cover internal steps, external ticks, repeated pitches,
and appending notes while a sequence is playing.
Synth tests cover pitch/bend, polyphony and sustain, clearing, release, chorus, extreme
controls, program routing, and allocation-free processing at four sample rates.
The keyboard test covers immediate note onset, channel-specific release,
sustain pedal, retriggering, and voice stealing at four sample rates.
Physical pot/button response, CV voltages, MIDI hardware, and Bela CPU/underruns
still require a board test.
