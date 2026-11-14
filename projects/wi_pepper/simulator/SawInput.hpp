#pragma once
#include "../AudioBiquad.hpp"
#include "../WiAudioDrivenDrumMachine.hpp"
#include <array>

// Simulator-only source, upstream of Pepper's real audio input/filter bank.
class SawInput {
public:
    bool enabled = false;
    bool sine = false;
    float frequency = 110, level = .5f;
    std::array<float, 7> eq{};
    void setup(float rate) { rate_ = rate; updateEq(); }
    void updateEq() {
        for(unsigned int i = 0; i < filters_.size(); ++i)
            filters_[i].peak(rate_, WiAudioDrivenDrumMachine::bandFrequency(i), 5.5f, eqSmoothed_[i]);
    }
    float process() {
        if(controlCountdown_-- == 0) {
            controlCountdown_ = 43;
            for(unsigned int i = 0; i < eq.size(); ++i)
                eqSmoothed_[i] += .025f * (eq[i] - eqSmoothed_[i]);
            updateEq();
        }
        const float dt = frequency / rate_;
        // PolyBLEP saw prevents oscillator aliasing from supplying false upper partials.
        float correction = 0;
        if(phase_ < dt) { const float t = phase_/dt; correction = 2*t-t*t-1; }
        else if(phase_ > 1-dt) { const float t = (phase_-1)/dt; correction = t*t+2*t+1; }
        float value = 2*phase_-1-correction;
        if(sine) value = std::sin(6.28318530718f * phase_);
        phase_ += dt;
        if(phase_ >= 1) phase_ -= 1;
        for(auto& filter : filters_) value = filter.process(value);
        gain_ += .001f * ((enabled ? level : 0) - gain_);
        return gain_ * value;
    }
private:
    float rate_ = 44100, phase_ = 0, gain_ = 0;
    unsigned int controlCountdown_ = 0;
    std::array<float, 7> eqSmoothed_{};
    std::array<AudioBiquad, 7> filters_;
};
