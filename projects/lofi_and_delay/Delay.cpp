#include "Delay.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

static void incrDelayLineCntr(float & cntr, float max_delay, float speed)
{
    cntr = cntr + speed;
    if (cntr >= max_delay) {
        //printf("Speed: %f\n", speed);
        cntr = 0;
    }
}

Delay::Delay()
{
    speed_ = 2.0f;
    BiquadCoeff::Settings lp_s = {
        .fs = 44100,
        .type = Biquad::lowpass,
        .cutoff = 400.0,  // Cut-off frequency for low-pass filter (Hz)
        .q = 1,
        .peakGainDb = 0
    };

    lpFilter_.setup(lp_s);
}


float Delay::process(float inSamp)
{
    // If speed is so high that it skips over a sample then we need to "fill in" the gap that it makes

    int speed = speed_;

    for (int i = 0; i < speed; i++) {
        int delayCnt = delayCnt_;
        float frac = i / (float)(speed - 1);
        float sample = (1 - frac) * prevInSample_ + frac * inSamp;
        line_[delayCnt] = lpFilter_.process(line_[delayCnt] * feedback_ + delayLevel_ * sample);
        incrDelayLineCntr(delayCnt_, delayTime_, 1);
    }

    //line_[delayCnt] = lpFilter_.process(line_[delayCnt] * feedback_ + delayLevel_ * inSamp);


    prevInSample_ = inSamp;
    int readIndex = delayCnt_;
    return line_[readIndex];
}

void Delay::setAnalogIns(AnalogIns ins)
{
    delayTime_ = 4 * 44100;
    delayLevel_ = map(ins.input_5, 0, 1, 0, 2.3);
    feedback_ = map(ins.input_6, 0, 1, 0, 1.3);
    speed_ = map(ins.input_7, 0, 1, 6, 40);
}
