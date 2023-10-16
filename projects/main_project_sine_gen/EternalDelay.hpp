#ifndef ETERNAL_DELAY_HPP
#define ETERNAL_DELAY_HPP

#include <string>
#include "mainCommon.hpp"

#define MODULATION_MAX_DELAY (10 * 44100)
#define MAX_DELAY (1.0 * 44100)

class EternalDelay
{
public:
    EternalDelay();
    float process(float inSamp);
    void setAnalogIns(struct AnalogIns ins);
    float modulation(float inSamp);
    float processEnvelope();

private:
    float modDelayLine_[MODULATION_MAX_DELAY];
    float delayLine_[MODULATION_MAX_DELAY];
    int modDelayWriteCnt_;
    int delayWriteCnt_;
    float vibDepth_;
    float vibSpeed_;
    float blend_;
    float delayLevel_;
    int delayTime_;
    float feedback_;
    float gPhase_;
    float envPower_;
};

#endif
