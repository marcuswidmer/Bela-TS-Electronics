#include "WiPepper.hpp"
#include "MidiInput.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

static void checkFullNoteRange(float rate)
{
    WiPepper pepper;
    MidiInput input;
    std::vector<uint8_t> output;
    assert(pepper.setup(44100, rate, [&](uint8_t byte) { output.push_back(byte); }));
    pepper.setPots({{0, 0, 0, 0, 0, 0, 0, 0}});

    // Assign every MIDI pitch using running status and interleaved clock.
    input.process(0xFA, pepper);
    input.process(0x90, pepper);
    for(unsigned int note = 0; note <= 127; ++note) {
        input.process(note, pepper);
        input.process(0xF8, pepper);
        input.process(100, pepper);
    }
    input.process(0xFA, pepper); // Clear queued clocks and start at the first note.
    assert(output.empty()); // Assigning notes never creates an extra gate or MIDI note.

    for(unsigned int note = 0; note <= 127; ++note) {
        for(int pulse = 0; pulse < 6; ++pulse) {
            input.process(0xF8, pepper);
            pepper.processControls();
        }
        const float expectedCv = std::max(0.0f, std::min(1.0f, (int(note) - 39) / 60.0f));
        assert(std::abs(pepper.cvOutputs()[0] - expectedCv) < 1e-6f);
        assert(pepper.cvOutputs()[1] == 1);
    }
    assert(output.size() % 3 == 0);
    unsigned int nextNote = 0;
    for(unsigned int i = 0; i < output.size(); i += 3) {
        if(output[i] == 0x90) {
            assert(output[i + 1] == nextNote++);
            assert(output[i + 2] == 100);
        } else assert(output[i] == 0x80);
    }
    assert(nextNote == 128);

    // Note zero also replaces an existing sequence while external clock is paused.
    for(int frame = 0; frame < rate + 1; ++frame) pepper.processControls();
    output.clear();
    const int drumStep = pepper.drums().currentStep();
    input.process(0x90, pepper);
    input.process(0, pepper);
    input.process(100, pepper);
    pepper.processControls();
    assert(pepper.cvOutputs()[0] == 0);
    assert(pepper.cvOutputs()[1] == 0);
    assert(pepper.drums().currentStep() == drumStep);
    assert(output.empty());
    input.process(0xF8, pepper);
    pepper.processControls();
    assert(pepper.cvOutputs()[1] == 1);
    assert(output == (std::vector<uint8_t>{0x90, 0, 100}));
}

int main()
{
    for(float rate : {1000.0f, 22050.0f, 44100.0f, 48000.0f}) checkFullNoteRange(rate);
    std::cout << "MIDI input tests passed: all pitches, CV gates, running status and interleaved clock\n";
}
