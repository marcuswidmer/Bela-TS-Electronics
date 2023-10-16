#include "Delay.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

static void incrDelayLineCntr(int & cntr, float max_delay)
{
    cntr++;
    if (cntr >= max_delay) {
        //printf("Speed: %f\n", speed);
        cntr = 0;
    }
}

Delay::Delay()
{
    speed = 2.0f;

    hp_s = {
        .fs = AUDIOSAMPLERATE_HZ,
        .type = Biquad::highpass,
        .cutoff = 3000.0,  // Cut-off frequency for low-pass filter (Hz)
        .q = 0,
        .peakGainDb = 0};

    hpFilter.setup(hp_s);
}


float Delay::process(float inSamp)
{
    // If speed is so high that it skips over a sample then we need to "fill in" the gap that it makes

    int speedInt = speed;

    for (int i = 0; i < speedInt; i++) {
        float frac = i / (float)(speedInt - 1);
        float sample = (1 - frac) * prevInSample + frac * inSamp;

        line[delayCnt] = line[delayCnt] * feedback + sample;

        incrDelayLineCntr(delayCnt, delayTime);
    }

    prevInSample = inSamp;

    return hpFilter.process(line[delayCnt]) * delayLevel;
}

void Delay::setAnalogIns(AnalogIns ins)
{
    delayTime = 10 * 44100;
    delayLevel = map(ins.input_0, 0, 1, 0, 4.2);
    feedback = map(ins.input_1, 0, 1, 0, 1.3);
    speed = map(ins.input_2, 0, 1, 15, 100);
    hp_s.cutoff = 40 * exp(6.21 * ins.input_3); // 100 to 10000 exponentially
    hp_s.q = 1.7;
    delayLevel = delayLevel * map(ins.input_3, 0, 1, 1, 2);
    hpFilter.setup(hp_s);
}
