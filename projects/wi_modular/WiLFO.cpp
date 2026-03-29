#include "WiLFO.hpp"
#include <cmath>
#include <chrono>
#include <stdio.h>

WiLFO::WiLFO()
{
}

void WiLFO::init(float fs)
{
    fs_ = fs;
}

void WiLFO::process()
{
    int newSamplesPerCycle = (int)lroundf(period_ * fs_);
    if (newSamplesPerCycle < 1)
        newSamplesPerCycle = 1;
    samplesPerCycle_ = newSamplesPerCycle;

    lfo_ = amp_ * (sin(cycleCountdown_ / samplesPerCycle_ * 2 * M_PI) + 1.0f) / 2.0f;

    cycleCountdown_--;
    if (cycleCountdown_ <= 0)
        cycleCountdown_ = samplesPerCycle_;
}

void WiLFO::setPeriod(float period)
{
    period_ = period;
}

void WiLFO::setAmplitude(float amp)
{
    amp_ = amp;
}
