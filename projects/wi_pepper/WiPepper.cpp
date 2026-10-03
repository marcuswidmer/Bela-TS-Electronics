#include "WiPepper.hpp"

bool WiPepper::setup(float sampleRate)
{
    return resonator.setup(sampleRate);
}

void WiPepper::setPots(const std::array<float, 8>& pots)
{
    volume = pots[0];
    resonator.setPots(pots);
}

void WiPepper::pressButton(unsigned int button)
{
    resonator.pressButton(button);
}

void WiPepper::process(const float* in, float* out)
{
    resonator.process(in[0], out[0], out[1]);
}
