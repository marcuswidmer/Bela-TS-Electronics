#pragma once

#include <array>
#include "KarplusResonator.hpp"

class WiPepper {
public:
    float volume = 0.0f;

    bool setup(float sampleRate);
    void setPots(const std::array<float, 8>& pots);
    void pressButton(unsigned int button);
    void process(const float* in, float* out);

private:
    KarplusResonator resonator;
};
