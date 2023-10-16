#include "Delay.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

static void incrDelayLineCntr(float & cntr, float max_delay, float speed)
{
    cntr = cntr + speed;
    if (cntr >= max_delay) {
        cntr = 0;
    }
}

Delay::Delay()
{
}

float Delay::process(float inSamp)
{
    float speed = 0.6;

    int delayNowCnt = delayNowCnt_;
    int floorThenCount = delayThenCnt_;

    if (!slow_) {
        float a = slowSpeedLine_[(((floorThenCount) % delayTime) + delayTime) % delayTime];
        float b = slowSpeedLine_[(((floorThenCount + 1) % delayTime) + delayTime) % delayTime];
        float frac = delayThenCnt_ - floorThenCount;
        float y = ((1 - frac) * a + frac * b);

        unitSpeedLine_[delayNowCnt] = (unitSpeedLine_[delayNowCnt] + y) * feedback_ + delayLevel_ * inSamp;
        //slowSpeedLine_[floorThenCount] = slowSpeedLine_[floorThenCount] * feedback_;

        incrDelayLineCntr(delayNowCnt_, delayTime, 1);
        incrDelayLineCntr(delayThenCnt_, delayTime, speed);
        return unitSpeedLine_[delayNowCnt];
    } else {
        float a = unitSpeedLine_[(((floorThenCount) % delayTime) + delayTime) % delayTime];
        float b = unitSpeedLine_[(((floorThenCount + 1) % delayTime) + delayTime) % delayTime];
        float frac = delayThenCnt_ - floorThenCount;
        float y = ((1 - frac) * a + frac * b);

        slowSpeedLine_[delayNowCnt] = (slowSpeedLine_[delayNowCnt] + y) * feedback_ + delayLevel_ * inSamp;
        //unitSpeedLine_[floorThenCount] = unitSpeedLine_[floorThenCount] * feedback_;

        incrDelayLineCntr(delayNowCnt_, delayTime, 1);
        incrDelayLineCntr(delayThenCnt_, delayTime, 1 / speed);
        return slowSpeedLine_[delayNowCnt];
    }

    // int floorReadCount = delayReadCnt_;

    // float speed = 0.3;

    // float a = line[(((floorReadCount) % delayTime) + delayTime) % delayTime];
    // float b = line[(((floorReadCount + 1) % delayTime) + delayTime) % delayTime];

    // float frac = delayReadCnt_ - floorReadCount;
    // float y = ((1 - frac) * a + frac * b);

    // incrDelayLineCntr(delayNowCnt_, delayTime, 1);
    // incrDelayLineCntr(delayThenCnt_, delayTime, speed);
}

void Delay::setAnalogIns(AnalogIns ins)
{
    delayTime = 2 * 44100 / 300 * (int)map(ins.input_1, 0, 1, 0, 300);
    delayLevel_ = map(ins.input_2, 0, 1, 0, 1.3);
    feedback_ = map(ins.input_3, 0, 1, 0, 1.3);
    slow_ = map(ins.input_4, 0, 1, 0, 1) > 0.5;
}
