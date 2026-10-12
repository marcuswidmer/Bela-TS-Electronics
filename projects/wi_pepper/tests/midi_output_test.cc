#include "WiPepper.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

// Realtime bytes may appear between channel messages.
static std::vector<uint8_t> notesOnly(const std::vector<uint8_t>& bytes)
{
    std::vector<uint8_t> notes;
    for(auto byte : bytes)
        if(byte < 0xF8) notes.push_back(byte);
    assert(notes.size() % 3 == 0);
    return notes;
}

static void checkAssignmentTiming(float rate)
{
    WiPepper reference, edited;
    int frame = 0;
    std::vector<int> referenceClocks, editedClocks, noteStarts;
    assert(reference.setup(44100, rate, [&](uint8_t byte) {
        if(byte == 0xF8) referenceClocks.push_back(frame);
    }));
    assert(edited.setup(44100, rate, [&](uint8_t byte) {
        if(byte == 0xF8) editedClocks.push_back(frame);
        if(byte == 0x90) noteStarts.push_back(frame);
    }));
    const std::array<float, 8> pots{{0, .1f, 0, 0, 0, 0, 0, 0}};
    reference.setPots(pots);
    edited.setPots(pots); // Fixed 2.5-second steps; substeps disabled.
    const int step = std::lround(rate * 2.5f);
    const std::vector<long> assignmentTimes{
        std::lround(rate * .1f), std::lround(rate * .2f), // Replace, then append.
        std::lround(rate * 1.4f), std::lround(rate * 1.5f), // Expired assignment window.
        step - 1, step, // Replace just before a step, append exactly on it.
        2 * step + 1, 3 * step - 1
    };
    unsigned int nextAssignment = 0;
    for(frame = 0; frame <= 4 * step; ++frame) {
        if(nextAssignment < assignmentTimes.size() && frame == assignmentTimes[nextAssignment]) {
            edited.midiNoteOn(60 + nextAssignment, 100);
            ++nextAssignment;
        }
        reference.processControls();
        edited.processControls();
        assert(edited.cvOutputs()[1] == reference.cvOutputs()[1]);
    }
    assert(nextAssignment == assignmentTimes.size());
    assert(noteStarts == (std::vector<int>{0, step, 2 * step, 3 * step, 4 * step}));
    assert(editedClocks == referenceClocks);
}

static void checkAssignmentOrder(float rate)
{
    WiPepper pepper;
    int frame = 0;
    std::vector<uint8_t> bytes;
    std::vector<int> starts;
    assert(pepper.setup(44100, rate, [&](uint8_t byte) {
        bytes.push_back(byte);
        if(byte == 0x90) starts.push_back(frame);
    }));
    std::array<float, 8> pots{{0, .1f, 0, 0, 0, 0, 0, 0}};
    pepper.setPots(pots);
    const int step = std::lround(rate * 2.5f);
    for(; frame < step + 101; ++frame) pepper.processControls();
    bytes.clear();
    starts.clear();
    for(auto pitch : {67u, 60u, 64u, 60u}) pepper.midiNoteOn(pitch, 100);
    for(; frame < 10 * step; ++frame) pepper.processControls();
    std::vector<uint8_t> pitches;
    const auto messages = notesOnly(bytes);
    for(unsigned int i = 0; i < messages.size(); i += 3)
        if(messages[i] == 0x90) pitches.push_back(messages[i+1]);
    assert(pitches == (std::vector<uint8_t>{67, 60, 64, 60, 67, 60, 64, 60}));
    assert(starts == (std::vector<int>{2*step, 3*step, 4*step, 5*step,
                                     6*step, 7*step, 8*step, 9*step}));

    // External ticks follow the same order, including notes appended while playing.
    assert(pepper.setup(44100, rate, [&](uint8_t byte) { bytes.push_back(byte); }));
    pots[1] = 1;
    pepper.setPots(pots);
    pepper.processControls();
    for(int i = 0; i < 2; ++i) {
        pepper.midiNoteOn(0, 100);
        pepper.processControls();
    }
    pepper.midiNoteOn(67, 100);
    pepper.processControls(); // Assignment itself does not consume the first step.
    pepper.midiNoteOn(60, 100);
    pepper.midiNoteOn(64, 100);
    bytes.clear();
    bool appended = false;
    for(int pitch : {67, 60, 64, 60, 67}) {
        pepper.midiNoteOn(0, 100);
        pepper.processControls();
        assert(std::abs(pepper.cvOutputs()[0] - (pitch - 39)/60.0f) < 1e-6f);
        if(!appended) {
            pepper.midiNoteOn(60, 100); // Append fourth note after first playback step.
            appended = true;
        }
    }
    pitches.clear();
    const auto externalMessages = notesOnly(bytes);
    for(unsigned int i = 0; i < externalMessages.size(); i += 3)
        if(externalMessages[i] == 0x90) pitches.push_back(externalMessages[i+1]);
    assert(pitches == (std::vector<uint8_t>{67, 60, 64, 60, 67}));
    pepper.stopMidiNotes();
}

int main()
{
    for(float rate : {1000.0f, 22050.0f, 44100.0f, 48000.0f}) {
        checkAssignmentOrder(rate);
        checkAssignmentTiming(rate);
        // Pot 3 spans 10 ms, 1.255 s at halfway, and a 2.5 s slow step.
        // Locking the pattern still allows this performance control.
        for(float length : {0.0f, .5f, 1.0f}) {
            WiPepper timed;
            std::vector<uint8_t> output;
            assert(timed.setup(44100, rate, [&](uint8_t byte) { output.push_back(byte); }));
            std::array<float, 8> controls{{0, .1f, 0, 0, 0, 0, 0, 0}};
            timed.setPots(controls);
            timed.processControls();
            timed.stopMidiNotes();
            timed.pressButton(2);
            controls[3] = length;
            timed.setPots(controls);
            output.clear();
            timed.midiNoteOn(0, 100); // One external tick; no intervening steps.
            timed.processControls();
            assert(notesOnly(output) == (std::vector<uint8_t>{0x90, 40, 100}));
            const float seconds = length == 0 ? .01f : length == .5f ? 1.255f : 2.5f;
            const int ticks = std::lround(rate * seconds);
            controls[3] = 1 - length; // Current note keeps its original duration.
            timed.setPots(controls);
            for(int i = 1; i < ticks; ++i) timed.processControls();
            assert(notesOnly(output) == (std::vector<uint8_t>{0x90, 40, 100}));
            timed.processControls();
            assert(notesOnly(output) == (std::vector<uint8_t>{0x90, 40, 100, 0x80, 40, 0}));
            assert(timed.cvOutputs()[1] == 0); // CV gate stays 10 ms.
        }

        // Direct-mode held notes follow input releases at both pot endpoints.
        for(float length : {0.0f, 1.0f}) {
            WiPepper direct;
            std::vector<uint8_t> output;
            assert(direct.setup(44100, rate, [&](uint8_t byte) { output.push_back(byte); }));
            std::array<float, 8> controls{{0, 0, 0, length, 0, 0, 0, 0}};
            direct.setPots(controls);
            direct.pressButton(1);
            output.clear();
            direct.midiNoteOn(60, 100, 2);
            assert(output == (std::vector<uint8_t>{0x92, 60, 100}));
            output.clear();
            for(int i = 0; i < std::lround(rate * 2.6f); ++i) direct.processControls();
            assert(output.empty());
            direct.midiNoteOff(60, 2, 30);
            assert(output == (std::vector<uint8_t>{0x82, 60, 30}));
        }

        WiPepper pepper;
        std::vector<uint8_t> bytes;
        assert(pepper.setup(44100, rate, [&](uint8_t byte) { bytes.push_back(byte); }));
        std::array<float, 8> pots{{0, .1f, 0, 0, 0, 0, 0, 0}};
        pepper.setPots(pots);

        // Internal steps send standard MIDI pitches and a 10 ms gate.
        pepper.processControls();
        assert(notesOnly(bytes) == (std::vector<uint8_t>{0x90, 40, 100}));
        const int duration = std::lround(rate * .01f);
        for(int i = 1; i < duration; ++i) pepper.processControls();
        assert(notesOnly(bytes) == (std::vector<uint8_t>{0x90, 40, 100}));
        pepper.processControls();
        assert(notesOnly(bytes) == (std::vector<uint8_t>{0x90, 40, 100, 0x80, 40, 0}));

        // Assignment restores the input pitch; external ticks retrigger it.
        bytes.clear();
        pepper.midiNoteOn(127, 70, 4);
        pepper.processControls();
        assert(notesOnly(bytes).empty()); // Assignment waits for the existing step deadline.
        for(int i = duration + 1; i < std::lround(rate * 2.5f); ++i)
            pepper.processControls();
        assert(notesOnly(bytes) == (std::vector<uint8_t>{0x90, 127, 100}));
        bytes.clear();
        pepper.midiNoteOn(0, 100);
        pepper.processControls();
        assert(notesOnly(bytes) == (std::vector<uint8_t>{0x80, 127, 0, 0x90, 127, 100}));

        // Switching to direct mode releases the currently sounding step.
        bytes.clear();
        pepper.pressButton(1);
        assert(bytes == (std::vector<uint8_t>{0x80, 127, 0, 0xFC}));
        bytes.clear();
        pepper.midiNoteOn(60, 73, 2);
        pepper.midiNoteOn(60, 90, 15);
        pepper.midiNoteOff(60, 2, 42);
        pepper.midiNoteOn(60, 0, 15);
        assert(bytes == (std::vector<uint8_t>{0x92, 60, 73, 0x9F, 60, 90,
                                             0x82, 60, 42, 0x9F, 60, 0}));
        bytes.clear();
        for(int i = 0; i < duration * 2; ++i) pepper.processControls();
        assert(bytes.empty()); // No sequenced notes, automatic releases or clock.
        pepper.midiNoteOn(128, 100);
        pepper.midiNoteOn(60, 128);
        pepper.midiNoteOn(60, 100, 16);
        pepper.midiNoteOff(128);
        pepper.midiNoteOff(60, 16);
        pepper.midiNoteOff(60, 0, 128);
        assert(bytes.empty());

        // Returning to sequencing releases every held channel/pitch.
        pepper.midiNoteOn(64, 91, 0);
        pepper.midiNoteOn(64, 92, 5);
        bytes.clear();
        pepper.pressButton(1);
        assert(bytes == (std::vector<uint8_t>{0x80, 64, 0, 0x85, 64, 0}));
        bytes.clear();
        pepper.processControls();
        assert(notesOnly(bytes).empty()); // No replay of direct-mode notes.
        pepper.midiNoteOn(0, 100);
        pepper.processControls();
        assert(notesOnly(bytes) == (std::vector<uint8_t>{0x90, 127, 100}));

        bytes.clear();
        pepper.pressButton(0);
        assert(bytes == (std::vector<uint8_t>{0x80, 127, 0, 0xFC}));
        bytes.clear();
        pepper.midiNoteOn(60, 100);
        pepper.midiNoteOff(60);
        pepper.processControls();
        pepper.pressButton(0);
        pepper.midiNoteOn(60, 100);
        pepper.midiNoteOff(60);
        assert(bytes.empty()); // Other programs don't duplicate notes.

        pepper.pressButton(0);
        pepper.pressButton(1);
        pepper.midiNoteOn(67, 100, 3);
        bytes.clear();
        pepper.stopMidiNotes();
        assert(bytes == (std::vector<uint8_t>{0x83, 67, 0}));
        bytes.clear();
        pepper.stopMidiNotes();
        assert(bytes.empty());
        pepper.midiNoteOn(69, 100, 1);
        bytes.clear();
        assert(pepper.setup(44100, rate)); // Repeated setup releases old output notes.
        assert(bytes == (std::vector<uint8_t>{0x81, 69, 0}));

        // Each default step uses its new pitch, including when locked.
        assert(pepper.setup(44100, rate, [&](uint8_t byte) { bytes.push_back(byte); }));
        pots[1] = 1;
        pepper.setPots(pots);
        bytes.clear();
        pepper.processControls();
        assert(notesOnly(bytes) == (std::vector<uint8_t>{0x90, 43, 100}));
        pepper.pressButton(2);
        pots[1] = 0;
        pots[2] = 1;
        pepper.setPots(pots);
        int previous = 43;
        for(int pitch : {47, 52, 55, 59, 40, 43}) {
            bytes.clear();
            pepper.midiNoteOn(0, 100);
            pepper.processControls();
            assert(notesOnly(bytes) == (std::vector<uint8_t>{0x80, static_cast<uint8_t>(previous), 0,
                                                            0x90, static_cast<uint8_t>(pitch), 100}));
            previous = pitch;
        }
        pepper.stopMidiNotes();
    }
    std::cout << "MIDI output tests passed: sequence, gates, direct notes and cleanup\n";
}
