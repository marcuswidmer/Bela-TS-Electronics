# Pepper resonator

`KarplusResonator` is a self-contained adaptation of the twelve parallel
feedback delays in `../karplus_resonator`. The original project is untouched.
No sample files, convolution, FFT worker, or `main_project` dependency is needed.
Input 0 excites the strings; both inputs still contribute to the peak LED meter.

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

| Button, left to right | Action |
|---|---|
| 1 | Pluck all strings with a short internal noise burst |
| 2 | Cycle chromatic, major, single note (unison), minor pentatonic note sets |
| 3 | Toggle sustain: disconnect external excitation and use long feedback |
| 4 | Clear all strings and cancel the pluck |

Buttons use active-high digital pins 15, 14, 13, 12 with 5 ms debouncing.
A held button triggers once; a button held at startup must first be released.
Sustain starts off and chromatic is the initial note set. Sustain is not an
infinite freeze: damping and soft saturation still dissipate energy. Plucking
works while sustain is enabled. Clear preserves the selected controls/mode.

Start with pot 1 halfway, pot 2 halfway, pot 3 halfway, pot 4 high,
pot 5 centered, pot 6 fully wet, and pots 7–8 low. Press button 1 to audition
without an input instrument. Button 4 can make a click because it immediately
clears the delay lines.

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
g++ -std=c++14 -fsyntax-only -Iinclude projects/wi_pepper/render.cpp projects/wi_pepper/WiPepper.cpp projects/wi_pepper/KarplusResonator.cpp
g++ -std=c++14 -O2 -Iprojects/wi_pepper projects/wi_pepper/tests/resonator_test.cc projects/wi_pepper/KarplusResonator.cpp -o /tmp/wi-pepper-resonator-test
/tmp/wi-pepper-resonator-test
```

Tests cover initialization, silence, pluck energy, stereo spread, extreme
controls, changing note sets, sustain, clearing, and dry routing at four
sample rates. `.cc` keeps the test out of Bela's recursive `.cpp` build.
Physical pot/button response and Bela CPU/underruns still require a board test.
