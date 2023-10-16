#ifndef DELAY_HPP
#define DELAY_HPP

#include <string>
#include "../main_project/mainCommon.hpp"

#define MAX_DELAY (10 * 44100)

class Delay
{
public:
    Delay();
    float process(float inSamp);
    void setAnalogIns(struct AnalogIns ins);

    float line[MAX_DELAY];
    int delayTime;

private:
    float delayThenCnt_;
    float delayNowCnt_;
    float delayLevel_;
    float feedback_;
    bool slow_;
    float unitSpeedLine_[MAX_DELAY];
    float slowSpeedLine_[MAX_DELAY];
};

#endif
