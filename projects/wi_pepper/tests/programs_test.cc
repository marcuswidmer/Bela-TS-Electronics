#include "WiPepper.hpp"
#include "ProgramIndicator.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    WiPepper pepper;
    std::vector<uint8_t> midi;
    assert(pepper.setup(44100, 1000, [&](uint8_t byte) { midi.push_back(byte); }));
    assert(pepper.program() == WiPepper::PgmSequencer);
    std::array<float, 8> pots{{1, 1, 0, 0.5f, 0.5f, 0, 0, 0}};
    pepper.setPots(pots);
    float in[] = {0.25f, -0.5f}, out[2];
    pepper.process(in, out);
    assert(out[0] == 0 && out[1] == 0);
    bool sawGate = false, sawClock = false;
    for(int i = 0; i < 4000; ++i) {
        pepper.processControls();
        const auto& cv = pepper.cvOutputs();
        assert(cv[0] >= 0 && cv[0] <= 1);
        sawGate |= cv[1] > 0;
        assert(pepper.sequencerLed() == (cv[1] > 0));
    }
    for(auto byte : midi) sawClock |= byte == 0xF8;
    assert(sawGate && sawClock);
    pepper.pressButton(1);
    for(int i = 0; i < 3000; ++i) pepper.processControls();
    bool sawResync = false;
    for(unsigned int i = 0; i + 2 < midi.size(); ++i)
        sawResync |= midi[i] == 0xFC && midi[i+1] == 0xFA && midi[i+2] == 0xF8;
    assert(sawResync);

    // Replace a longer sequence with a single note without a stale step index.
    pepper.midiNoteOn(60, 100);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 21.0f/60) < 1e-6f);
    pepper.midiNoteOn(72, 0); // Velocity zero is note-off, not assignment.
    for(int i = 0; i < 1200; ++i) pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 21.0f/60) < 1e-6f);
    pepper.midiNoteOn(64, 100);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 25.0f/60) < 1e-6f);

    pepper.pressButton(0);
    assert(pepper.program() == WiPepper::PgmKarplusResonator);
    assert(midi.back() == 0xFC);
    assert(!pepper.sequencerLed());
    const auto midiCount = midi.size();
    pepper.midiNoteOn(80, 100); // Inactive sequencer ignores incoming notes.
    for(int i = 0; i < 4000; ++i) {
        pepper.processControls();
        pepper.process(in, out);
    }
    assert(midi.size() == midiCount);
    for(auto cv : pepper.cvOutputs()) assert(cv == 0);
    assert(std::abs(out[0] - 0.25f) < 1e-4f && out[0] == out[1]);
    pepper.pressButton(0);
    assert(pepper.program() == WiPepper::PgmSequencer);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 25.0f/60) < 1e-6f);
    pepper.process(in, out);
    assert(out[0] == 0 && out[1] == 0);

    // Optional MIDI callback and repeated setup must be safe.
    assert(pepper.setup(48000, 1000));
    pepper.pressButton(1);
    for(int i = 0; i < 4000; ++i) pepper.processControls();
    assert(pepper.program() == WiPepper::PgmSequencer);
    assert(!pepper.setup(0));

    // At 1 kHz, a step lights the LED for 10 ticks; resync priority lasts 1 s.
    assert(pepper.setup(44100, 1000));
    pots[0] = pots[2] = 0;
    pepper.setPots(pots);
    for(int i = 0; i < 10; ++i) {
        pepper.processControls();
        assert(pepper.sequencerLed());
    }
    pepper.processControls();
    assert(!pepper.sequencerLed());
    pepper.pressButton(1);
    for(int i = 0; i < 1000; ++i) {
        pepper.processControls();
        assert(pepper.sequencerLed());
        assert(pepper.cvOutputs()[1] == 0);
    }
    pepper.processControls();
    assert(!pepper.sequencerLed());
    pepper.pressButton(1);
    pepper.processControls();
    assert(pepper.sequencerLed());
    pepper.pressButton(0);
    assert(!pepper.sequencerLed());
    pepper.pressButton(0);
    assert(!pepper.sequencerLed());

    for(float rate : {22050.0f, 44100.0f, 48000.0f, 96000.0f}) {
        ProgramIndicator indicator;
        indicator.setup(rate);
        for(unsigned int program = 0; program < 2; ++program) {
            indicator.select(program);
            unsigned int flashes = 0, samples = 0;
            bool wasOn = false;
            while(indicator.active()) {
                const bool on = indicator.ledOn(program);
                if(on && !wasOn) ++flashes;
                for(unsigned int led = 0; led < 10; ++led)
                    if(led != program) assert(!indicator.ledOn(led));
                wasOn = on;
                indicator.advance();
                ++samples;
            }
            assert(flashes == 4);
            assert(samples == 8 * static_cast<unsigned int>(rate * 0.15f));
        }
        indicator.select(0);
        indicator.advance();
        indicator.select(1); // Switching mid-blink restarts the new indication.
        assert(!indicator.ledOn(0) && indicator.ledOn(1));
    }
    std::cout << "Program, MIDI, CV and indicator tests passed\n";
}
