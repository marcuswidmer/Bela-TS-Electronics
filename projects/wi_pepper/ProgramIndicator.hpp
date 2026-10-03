#pragma once

#include <algorithm>

// Four 150 ms flashes. Advance once per digital frame; no blocking delays.
class ProgramIndicator {
public:
    void setup(float sampleRate) {
        halfPeriod_ = std::max(1u, static_cast<unsigned int>(sampleRate * 0.15f));
        phase_ = 8;
        remaining_ = 0;
    }
    void select(unsigned int program) {
        program_ = program;
        phase_ = 0;
        remaining_ = halfPeriod_;
    }
    bool active() const { return phase_ < 8; }
    bool ledOn(unsigned int led) const {
        return active() && phase_ % 2 == 0 && led == program_;
    }
    void advance() {
        if(active() && --remaining_ == 0) {
            ++phase_;
            remaining_ = halfPeriod_;
        }
    }
private:
    unsigned int program_ = 0, phase_ = 8, remaining_ = 0, halfPeriod_ = 1;
};
