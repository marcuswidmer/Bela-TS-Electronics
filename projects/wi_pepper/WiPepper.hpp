#pragma once

#include <array>
#include <functional>
#include "KarplusResonator.hpp"
#include "WiSequencer.hpp"

class WiPepper {
public:
    enum Program { PgmSequencer = 0, PgmKarplusResonator = 1 };
    float volume = 0.0f;

    bool setup(float sampleRate, float controlSampleRate = 0,
               std::function<void(uint8_t)> midiOutput = nullptr);
    void setPots(const std::array<float, 8>& pots);
    void pressButton(unsigned int button);
    void midiNoteOn(unsigned int note, unsigned int velocity);
    void processControls(); // Call at controlSampleRate (Bela analog rate).
    void process(const float* in, float* out);
    bool sequencerLed() const { return program_ == PgmSequencer && sequencerLed_; }
    Program program() const { return program_; }
    const std::array<float, 4>& cvOutputs() const { return cvOutputs_; }

private:
    Program program_ = PgmSequencer;
    KarplusResonator resonator;
    WiSequencer sequencer;
    std::array<float, 8> pots_{{0, 0.5f, 0.5f, 0.5f, 0.5f, 1, 0, 0}};
    std::array<float, 4> cvOutputs_{};
    std::function<void(uint8_t)> midiOutput_;
    float controlSampleRate_ = 0, speedSmoothed_ = 0;
    int stepCountdown_ = 0, priorityCountdown_ = 0, syncCountdown_ = 0;
    bool sequencerButton_ = false;
    bool sequencerLed_ = false;
};
