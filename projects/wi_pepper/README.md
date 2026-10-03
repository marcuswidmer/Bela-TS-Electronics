# Pepper programs

Button 0 (leftmost, digital pin 15) cycles between:

- Program 0: `PgmSequencer`, the default, copied from `wi_modular/WiSequencer`.
- Program 1: `PgmKarplusResonator`, the existing twelve-string resonator.

At startup and after each selection, only the corresponding LED (0 or 1)
flashes four times, with 150 ms on and 150 ms off. Afterward, program 1 shows
the input peak meter; program 0 shows the sequencer step and priority countdowns
on the last LED (index 9, digital pin 8), with the other LEDs off. Program-selection flashes take precedence. Button debouncing and program indication never block processing.

## Sequencer (program 0)

Pots 0–2 match `WiModular`'s `PgmSequencer` mapping: speed, default sequence
fraction / assigned-sequence substeps, and random sequence / assignment freeze.
Button 1 requests MIDI resync (Stop, Start, Clock at the next internal step).
Buttons 2–3 are unused in this program. Audio outputs are silent.

Analog output 0 carries pitch (note / 60, clamped to 0–1), output 1 carries
10 ms step/substep gates, and output 2 retains the source's sync output
(currently inactive because its sync callback is disabled). Other analog
outputs are zero. Sequencing runs at the actual analog sample rate, with
an audio-rate fallback if analog I/O is disabled.

MIDI uses `hw:1,0,0`, as in `WiModular.cpp`. Positive-velocity Note On messages
assign notes with an offset of 39. Note 0 ticks the sequencer externally;
regular incoming MIDI Clock is not used, matching the source. Output Clock
is six pulses per internal step. Leaving the sequencer sends Stop, clears CV
outputs, and pauses sequence processing. Returning preserves assigned notes
and restores internal timing; Button 1 resyncs connected equipment.
Missing MIDI hardware does not prevent the programs from running.

The local sequencer copy removes Bela logging, reserves space for up to 128
assigned steps during setup, and resets the step index when replacing a sequence.
MIDI input is consumed on the render thread to avoid concurrent sequence edits.

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
g++ -std=c++14 -fsyntax-only -Iinclude -I. projects/wi_pepper/render.cpp projects/wi_pepper/WiPepper.cpp projects/wi_pepper/WiSequencer.cpp projects/wi_pepper/KarplusResonator.cpp
g++ -std=c++14 -O2 -Iprojects/wi_pepper projects/wi_pepper/tests/resonator_test.cc projects/wi_pepper/KarplusResonator.cpp -o /tmp/wi-pepper-resonator-test
/tmp/wi-pepper-resonator-test
g++ -std=c++14 -O2 -Iprojects/wi_pepper projects/wi_pepper/tests/programs_test.cc projects/wi_pepper/WiPepper.cpp projects/wi_pepper/WiSequencer.cpp projects/wi_pepper/KarplusResonator.cpp -o /tmp/wi-pepper-programs-test
/tmp/wi-pepper-programs-test
```

Tests cover initialization, silence, pluck energy, stereo spread, extreme
controls, changing note sets, sustain, clearing, and dry routing at four
sample rates. `.cc` keeps the test out of Bela's recursive `.cpp` build.
Program tests cover the default selection, switching, MIDI assignment and resync,
CV clearing, audio routing, and four-flash indication at multiple sample rates.
Physical pot/button response, CV voltages, MIDI hardware, and Bela CPU/underruns
still require a board test.
