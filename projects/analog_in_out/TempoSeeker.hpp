#ifndef TEMPO_SEEKER_HPP
#define TEMPO_SEEKER_HPP

#include <string>
#include "../main_project/mainCommon.hpp"
#include "Ramp.h"


class TempoSeeker
{
public:
    TempoSeeker();
    void process();
    void setAnalogIns(struct AnalogIns ins);
    float tempoLed;
    int period;
    bool risingEdge;

private:
    float buttonSignal;
    float prevButtonSignal;
    int periodCntr;
    int ledOn;
    float ledOnSec;
    int sinceLastPushCntr;
    int risingEdgeCntr;


    Ramp periodLed;
    Ramp buttonPushLed;
};

#endif