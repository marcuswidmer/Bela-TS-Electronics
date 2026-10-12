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
    pepper.pressButton(3);
    assert(pepper.noteOverlay());
    const auto beforeOverlay = midi.size();
    pepper.midiNoteOn(76, 80, 2);
    pepper.midiNoteOff(76, 2, 30);
    const std::vector<uint8_t> overlayBytes(midi.begin() + beforeOverlay, midi.end());
    assert(overlayBytes == (std::vector<uint8_t>{0x92, 76, 80, 0x82, 76, 30}));
    pepper.pressButton(3);
    assert(!pepper.noteOverlay());

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
    assert(pepper.program() == WiPepper::PgmJuno);
    pepper.pressButton(0);
    assert(pepper.program() == WiPepper::PgmSequencer);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 25.0f/60) < 1e-6f);
    pepper.process(in, out);
    assert(out[0] == 0 && out[1] == 0);

    // Optional MIDI callback and repeated setup must be safe.
    assert(pepper.setup(48000, 1000));
    pepper.pressButton(3);
    for(int i = 0; i < 4000; ++i) pepper.processControls();
    assert(pepper.program() == WiPepper::PgmSequencer);
    assert(!pepper.setup(0));

    // At 1 kHz, a step lights the LED for 10 ticks; overlay does not alter timing.
    assert(pepper.setup(44100, 1000));
    pots[0] = pots[2] = 0;
    pepper.setPots(pots);
    for(int i = 0; i < 10; ++i) {
        pepper.processControls();
        assert(pepper.sequencerLed());
    }
    pepper.processControls();
    assert(!pepper.sequencerLed());
    pepper.pressButton(3);
    assert(pepper.noteOverlay());
    for(int i = 0; i < 1000; ++i) {
        pepper.processControls();
        assert(!pepper.sequencerLed());
        assert(pepper.cvOutputs()[1] == 0);
    }
    pepper.processControls();
    assert(!pepper.sequencerLed());
    pepper.pressButton(3);
    assert(!pepper.noteOverlay());
    pepper.processControls();
    assert(!pepper.sequencerLed());
    pepper.pressButton(0);
    assert(!pepper.sequencerLed());
    pepper.pressButton(0);
    assert(!pepper.sequencerLed());

    // Direct MIDI mode pauses sequencing and preserves the assigned sequence.
    assert(pepper.setup(44100, 1000, [&](uint8_t b) { midi.push_back(b); }));
    pots[2] = 0;
    pepper.setPots(pots);
    pepper.midiNoteOn(60, 100);
    pepper.processControls();
    pepper.pressButton(1);
    assert(pepper.directMidiMode());
    const auto clocksBefore = midi.size();
    pepper.midiNoteOn(72, 100);
    for(int i = 0; i < 10; ++i) {
        pepper.processControls();
        assert(std::abs(pepper.cvOutputs()[0] - 33.0f/60) < 1e-6f);
        assert(pepper.cvOutputs()[1] == 1);
        assert(!pepper.sequencerLed());
    }
    pepper.processControls();
    assert(pepper.cvOutputs()[1] == 0);
    pepper.midiNoteOff(72);
    pepper.midiNoteOn(75, 0);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 33.0f/60) < 1e-6f);
    assert(pepper.cvOutputs()[1] == 0);
    pepper.midiNoteOn(127, 100);
    pepper.processControls();
    assert(pepper.cvOutputs()[0] == 1);
    pepper.midiNoteOn(0, 100);
    pepper.processControls();
    assert(pepper.cvOutputs()[0] == 0);
    assert(pepper.cvOutputs()[1] == 1);
    assert(midi.size() == clocksBefore + 15); // Five forwarded note messages, no clock.
    pepper.pressButton(1);
    assert(!pepper.directMidiMode());
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 21.0f/60) < 1e-6f);
    pepper.pressButton(1);
    pepper.midiNoteOn(64, 100);
    pepper.pressButton(0);
    assert(!pepper.directMidiMode());
    for(float cv : pepper.cvOutputs()) assert(cv == 0);

    // Freeze keeps the full default pattern playing despite pot/MIDI edits.
    assert(pepper.setup(44100, 1000, [&](uint8_t b) { midi.push_back(b); }));
    pots[0] = pots[1] = 1;
    pots[2] = 0;
    pepper.setPots(pots);
    pepper.processControls();
    assert(!pepper.sequenceLocked());
    pepper.pressButton(2);
    assert(pepper.sequenceLocked());
    const int defaultPattern[] = {8, 13, 16, 20, 1, 4};
    const auto midiBeforeLock = midi.size();
    pots[1] = 0; // Would collapse the default pattern to one note.
    pots[2] = 1; // Would randomize it and previously froze assignment.
    pepper.setPots(pots);
    pepper.midiNoteOn(90, 100); // Rejected immediately, before control processing.
    for(int cycle = 0; cycle < 3; ++cycle) {
        for(int note : defaultPattern) {
            pepper.midiNoteOn(0, 100); // External ticks remain available while locked.
            pepper.processControls();
            assert(std::abs(pepper.cvOutputs()[0] - note/60.0f) < 1e-6f);
            assert(pepper.cvOutputs()[1] == 1);
        }
    }
    for(int i = 0; i < 1200; ++i) pepper.processControls();
    bool clockWhileLocked = false;
    for(auto i = midiBeforeLock; i < midi.size(); ++i) {
        assert(midi[i] != 0xFC && midi[i] != 0xFA); // Notes and clock continue without Stop/Start.
        clockWhileLocked |= midi[i] == 0xF8;
    }
    assert(clockWhileLocked);
    pepper.pressButton(2);
    assert(!pepper.sequenceLocked());
    pepper.processControls();
    pepper.midiNoteOn(0, 100);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 1.0f/60) < 1e-6f);

    // Assigned notes are protected too; pot 2 no longer locks assignment.
    pepper.midiNoteOn(60, 100);
    pepper.processControls();
    pepper.pressButton(2);
    pepper.midiNoteOn(64, 100);
    pepper.midiNoteOn(0, 100);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 21.0f/60) < 1e-6f);
    pepper.pressButton(1);
    assert(pepper.sequenceLocked() && pepper.directMidiMode());
    pepper.midiNoteOn(72, 100);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 33.0f/60) < 1e-6f);
    pepper.pressButton(1);
    pepper.pressButton(0);
    assert(!pepper.sequenceLocked()); // Only display the lock in sequencer mode.
    pepper.pressButton(0);
    pepper.pressButton(0);
    assert(pepper.sequenceLocked());
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 21.0f/60) < 1e-6f);
    for(int i = 0; i < 1200; ++i) pepper.processControls();
    pepper.pressButton(2);
    pepper.midiNoteOn(64, 100);
    pepper.processControls();
    assert(std::abs(pepper.cvOutputs()[0] - 25.0f/60) < 1e-6f);
    pepper.pressButton(2);
    assert(pepper.setup(44100, 1000));
    assert(!pepper.sequenceLocked());

    for(float rate : {22050.0f, 44100.0f, 48000.0f, 96000.0f}) {
        ProgramIndicator indicator;
        indicator.setup(rate);
        for(unsigned int program = 0; program < WiPepper::ProgramCount; ++program) {
            indicator.select(program);
            for(unsigned int sample = 0; sample < static_cast<unsigned int>(rate * 2); ++sample) {
                assert(indicator.active());
                for(unsigned int led = 0; led < 10; ++led)
                    assert(indicator.ledOn(led) == (led == program));
                indicator.advance();
            }
        }
        indicator.select(0);
        indicator.advance();
        indicator.select(1); // Switching immediately moves the steady indication.
        assert(!indicator.ledOn(0) && indicator.ledOn(1));
    }
    std::cout << "Program, MIDI, CV and indicator tests passed\n";
}
