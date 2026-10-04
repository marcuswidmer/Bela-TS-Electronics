#pragma once

#include <array>
#include <functional>
#include "KarplusResonator.hpp"
#include "JunoSynth.hpp"
#include "WiSequencer.hpp"

class WiPepper {
public:
    enum Program { PgmSequencer = 0, PgmKarplusResonator = 1, PgmJuno = 2, ProgramCount = 3 };
    float volume = 0.0f;

    bool setup(float sampleRate, float controlSampleRate = 0,
               std::function<void(uint8_t)> midiOutput = nullptr);
    void setPots(const std::array<float, 8>& pots);
    void pressButton(unsigned int button);
    void midiNoteOn(unsigned int note, unsigned int velocity, unsigned int channel = 0);
    void midiNoteOff(unsigned int note, unsigned int channel = 0);
    void midiControlChange(unsigned int controller, unsigned int value, unsigned int channel = 0);
    void midiPitchBend(unsigned int value, unsigned int channel = 0);
    unsigned int synthVoices() const { return program_ == PgmJuno ? juno.activeVoices() : 0; }
    void processControls(); // Call at controlSampleRate (Bela analog rate).
    void process(const float* in, float* out);
    bool sequencerLed() const { return program_ == PgmSequencer && sequencerLed_; }
    Program program() const { return program_; }
    const std::array<float, 4>& cvOutputs() const { return cvOutputs_; }

private:
    Program program_ = PgmSequencer;
    KarplusResonator resonator;
    JunoSynth juno;
    WiSequencer sequencer;
    std::array<float, 8> pots_{{0, 0.5f, 0.5f, 0.5f, 0.5f, 1, 0, 0}};
    std::array<float, 4> cvOutputs_{};
    std::function<void(uint8_t)> midiOutput_;
    float controlSampleRate_ = 0, speedSmoothed_ = 0;
    int stepCountdown_ = 0, priorityCountdown_ = 0, syncCountdown_ = 0;
    bool sequencerButton_ = false;
    bool sequencerLed_ = false;
};
