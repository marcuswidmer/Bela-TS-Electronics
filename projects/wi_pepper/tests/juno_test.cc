#include "JunoSynth.hpp"
#include "WiPepper.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>

static bool countAllocations = false;
static unsigned int allocations = 0;
void* operator new(std::size_t size) {
    if(countAllocations) ++allocations;
    if(void* p = std::malloc(size)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

const std::array<float, 8> patch{{.7f, .6f, .25f, .3f, 0, .2f, .7f, .1f}};

double run(JunoSynth& synth, unsigned int frames, double* stereo = nullptr)
{
    double energy = 0;
    for(unsigned int i = 0; i < frames; ++i) {
        float l, r;
        synth.process(l, r);
        assert(std::isfinite(l) && std::isfinite(r));
        assert(std::abs(l) <= 1 && std::abs(r) <= 1);
        energy += l*l + r*r;
        if(stereo) *stereo += (l-r)*(l-r);
    }
    return energy;
}

int main()
{
    JunoSynth synth;
    assert(!synth.setup(0));
    assert(!synth.setup(std::numeric_limits<float>::quiet_NaN()));
    for(float fs : {22050.0f, 44100.0f, 48000.0f, 96000.0f}) {
        assert(synth.setup(fs));
        synth.setPots(patch);
        assert(run(synth, 1000) == 0);
        synth.noteOn(60, 100);
        double stereo = 0;
        assert(run(synth, fs / 2, &stereo) > .01);
        assert(stereo > .001);
        assert(synth.activeVoices() == 1);
        synth.noteOff(60);
        run(synth, fs);
        assert(synth.activeVoices() == 0);
        assert(run(synth, 1000) < 1e-12);

        // Channel-specific sustain and Note On velocity zero as Note Off.
        synth.noteOn(60, 100, 2);
        run(synth, 1000);
        synth.controlChange(64, 127, 2);
        synth.noteOn(60, 0, 2);
        run(synth, fs / 2);
        assert(synth.activeVoices() == 1);
        synth.controlChange(64, 0, 1); // Other channel must not release it.
        run(synth, 1000);
        assert(synth.activeVoices() == 1);
        synth.controlChange(64, 0, 2);
        run(synth, fs);
        assert(synth.activeVoices() == 0);

        // Retriggering and stealing never exceed the six voice limit.
        for(unsigned int n = 48; n < 60; ++n) {
            synth.noteOn(n, 127);
            run(synth, 100);
            assert(synth.activeVoices() <= 6);
        }
        assert(synth.activeVoices() == 6);
        synth.noteOff(48); // Stolen note must not release its replacement.
        assert(synth.activeVoices() == 6);
        synth.noteOn(59, 100);
        assert(synth.activeVoices() == 6);
        synth.controlChange(123, 0);
        run(synth, fs);
        assert(synth.activeVoices() == 0);
        synth.noteOn(64, 127);
        run(synth, 1000);
        synth.controlChange(120, 0);
        assert(synth.activeVoices() == 0);
        assert(run(synth, 1000) == 0);

        // Sweep extreme parameters, MIDI range and all oscillator/chorus modes.
        for(unsigned int mode = 0; mode < 3; ++mode) {
            synth.pressButton(1); synth.pressButton(2);
            for(unsigned int note : {0u, 36u, 69u, 100u, 127u}) {
                std::array<float, 8> extremes{{1, 1, 1, 1, 0, 0, 1, 0}};
                synth.setPots(extremes);
                synth.noteOn(note, 127);
                synth.pitchBend(16383);
                synth.controlChange(1, 127);
                run(synth, 4000);
                synth.pitchBend(0);
                extremes[1] = 0;
                synth.setPots(extremes);
                run(synth, 4000);
            }
        }
        synth.pressButton(3);
        assert(synth.activeVoices() == 0);
        assert(run(synth, 1000) == 0);
    }

    // Measure the A4 spectral peak and the +2-semitone bend. Oscillator noise
    // and the sub oscillator make raw zero-crossing counts unreliable.
    assert(synth.setup(44100));
    auto bright = patch; bright[1] = 1; bright[2] = bright[3] = 0; bright[6] = 1;
    synth.setPots(bright);
    synth.pressButton(1); // Saw.
    synth.pressButton(2); synth.pressButton(2); // Chorus off.
    synth.noteOn(69, 127);
    run(synth, 44100);
    auto spectralPeak = [&]() {
        std::array<float, 44100> samples{};
        for(auto& l : samples) { float r; synth.process(l, r); }
        unsigned int peak = 0;
        double largest = 0;
        for(unsigned int hz = 420; hz <= 510; ++hz) {
            const double coefficient = 2 * std::cos(2 * 3.141592653589793 * hz / 44100);
            double a = 0, b = 0;
            for(float sample : samples) {
                const double next = sample + coefficient * a - b;
                b = a; a = next;
            }
            const double power = a*a + b*b - coefficient*a*b;
            if(power > largest) { largest = power; peak = hz; }
        }
        return peak;
    };
    assert(spectralPeak() == 440);
    // The one-octave-down sub must be audible, not just present in the mix.
    {
        double subReal = 0, subImag = 0, mainReal = 0, mainImag = 0;
        for(unsigned int i = 0; i < 44100; ++i) {
            float l, r; synth.process(l, r);
            const double phase = 2 * 3.141592653589793 * i / 44100;
            subReal += l * std::cos(220 * phase); subImag += l * std::sin(220 * phase);
            mainReal += l * std::cos(440 * phase); mainImag += l * std::sin(440 * phase);
        }
        assert(std::hypot(subReal, subImag) > .5 * std::hypot(mainReal, mainImag));
    }
    // Chorus supplies a substantial side signal; bypass remains mono.
    {
        double monoSide = 0; run(synth, 44100, &monoSide);
        assert(monoSide < 1e-10);
        synth.pressButton(2); run(synth, 44100);
        double side = 0;
        const double energy = run(synth, 44100, &side);
        assert(side > .1 * energy);
        synth.pressButton(2); synth.pressButton(2); run(synth, 44100);
    }

    synth.pitchBend(16383); run(synth, 1000);
    const auto bent = spectralPeak();
    assert(bent >= 493 && bent <= 494);

    // Audio processing, controls and MIDI handling allocate no memory.
    countAllocations = true;
    for(int i = 0; i < 100; ++i) {
        synth.setPots(patch);
        synth.noteOn(48 + i % 24, 100);
        synth.pitchBend(i * 100);
        run(synth, 32);
        synth.noteOff(48 + i % 24);
    }
    countAllocations = false;
    assert(allocations == 0);

    // Full program routing: raw MIDI pitch, audio output, CV isolation, no stuck notes.
    WiPepper pepper;
    assert(pepper.setup(44100));
    pepper.setPots(patch);
    pepper.pressButton(0); pepper.pressButton(0);
    assert(pepper.program() == WiPepper::PgmJuno);
    pepper.midiNoteOn(60, 100);
    float input[2] = {}, output[2];
    double energy = 0;
    for(int i = 0; i < 44100; ++i) {
        pepper.process(input, output);
        energy += output[0]*output[0];
    }
    assert(energy > .01 && pepper.synthVoices() == 1);
    for(float cv : pepper.cvOutputs()) assert(cv == 0);
    pepper.midiControlChange(64, 127);
    pepper.midiNoteOff(60);
    pepper.pressButton(0);
    assert(pepper.program() == WiPepper::PgmSequencer);
    pepper.pressButton(0); pepper.pressButton(0);
    assert(pepper.synthVoices() == 0);
    pepper.process(input, output);
    assert(output[0] == 0 && output[1] == 0);
    std::cout << "JUNO tests passed: pitch, polyphony, MIDI, envelopes, chorus, limits, routing and allocations\n";
}
