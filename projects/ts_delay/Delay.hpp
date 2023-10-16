#ifndef DELAY_HPP
#define DELAY_HPP

#include <string>
#include "../main_project/mainCommon.hpp"
#include <libraries/Biquad/Biquad.h>

#define MAX_DELAY (10 * 44100)

class Delay
{
public:
    Delay();
    float process(float inSamp);
    void setAnalogIns(struct AnalogIns ins);

private:
    int delayTime;
    int delayCnt;
    float delayLevel;
    float feedback;
    float speed;
    float line[MAX_DELAY];
    float prevInSample;
    Biquad hpFilter;
    BiquadCoeff::Settings hp_s;
};

#endif
