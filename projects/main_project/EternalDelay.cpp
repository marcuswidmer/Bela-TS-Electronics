#include "EternalDelay.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

static void incrDelayLineCntr(int & cntr, int max_delay)
{
    cntr++;
    if (cntr >= max_delay)
        cntr = 0;
}

EternalDelay::EternalDelay()
{
}

float EternalDelay::process(float inSamp)
{
    float y = modulation(inSamp);
    float envelope = processEnvelope();
    delayLine_[delayWriteCnt_] = delayLine_[delayWriteCnt_] * feedback_ + delayLevel_ * y * envelope;

    incrDelayLineCntr(delayWriteCnt_, delayTime_);

    return delayLine_[delayWriteCnt_] + inSamp;
    //return delayLine_[delayWriteCnt_];
    //return envelope * inSamp;
}

float EternalDelay::modulation(float inSamp)
{
    modDelayLine_[modDelayWriteCnt_] = inSamp;

    float modulator_osc = vibDepth_ * (1 + sinf(gPhase_)) / 2;
    int k0 = modulator_osc;
    float frac = modulator_osc - k0;

    float a = modDelayLine_[(((modDelayWriteCnt_ - k0) % MODULATION_MAX_DELAY) + MODULATION_MAX_DELAY) % MODULATION_MAX_DELAY];
    float b = modDelayLine_[(((modDelayWriteCnt_ - k0 - 1) % MODULATION_MAX_DELAY) + MODULATION_MAX_DELAY) % MODULATION_MAX_DELAY];

    float y = ((1 - frac) * a + frac * b) + blend_ * inSamp;

    gPhase_ += 2.0f * (float)M_PI * vibSpeed_ * 1.0f / 44100.0f;
    if(gPhase_ > M_PI)
        gPhase_ -= 2.0f * (float)M_PI;

    incrDelayLineCntr(modDelayWriteCnt_, MODULATION_MAX_DELAY);

    return y;
}

void EternalDelay::setAnalogIns(AnalogIns ins)
{
    vibDepth_ = map(ins.input_1, 0, 1, 0, 200);
    vibSpeed_ = map(ins.input_2, 0, 1, 0, 20);
    delayTime_ = 2 * 44100 / 100 * (int)map(ins.input_3, 0, 1, 0, 100);
    delayLevel_ = map(ins.input_4, 0, 1, 0, 1.3);
    feedback_ = map(ins.input_5, 0, 1, 0, 1.3);
    envPower_ = map(ins.input_6, 0, 1, 1, 200);

}

float EternalDelay::processEnvelope()
{
    float env = pow((1 + sinf(gPhase_)) / 2, envPower_);
    return env;
}




