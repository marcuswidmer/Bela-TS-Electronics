#ifndef DELAY_HPP
#define DELAY_HPP

#include <string>
#include "../main_project/mainCommon.hpp"
#include <libraries/Biquad/Biquad.h>
#include <functional>

#define MAX_DELAY (10 * 44100)

class Delay
{
public:
    Delay();
    float process(float inSamp);
    void setAnalogIns(struct AnalogIns ins);
    void setTempo(float period);
    void handleSpeedKnob(float knobSpeed);

    std::function<void(int)> setPeriodCallback;

private:
    int reelLength;
    int delayCnt;
    float delayLevel;
    float feedback;
    float speed;
    float line[MAX_DELAY];
    float prevInSample;
    float saturation;
    Biquad hpFilter;
    BiquadCoeff::Settings hp_s;

    float prevKnobSpeed;
    int speedKnobSamplerCntr;
};

#endif
