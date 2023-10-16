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

    int delayTime_;

private:
    float delayCnt_;
    float delayLevel_;
    float feedback_;
    float speed_;
    float line_[MAX_DELAY];
    float prevInSample_;

    Biquad lpFilter_;
};

#endif
