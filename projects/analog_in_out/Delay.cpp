#include "Delay.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

#define SPEED_KNOB_FS 441

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
    reelLength = 10 * 44100;
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

        incrDelayLineCntr(delayCnt, reelLength);
    }

    prevInSample = inSamp;

    float out = line[delayCnt];
    out =  (1 + 0.1 * saturation) * out / (1 + saturation * fabs(out));
    return hpFilter.process(out) * delayLevel;
}

void Delay::setAnalogIns(AnalogIns ins)
{
    delayLevel = map(ins.input_1, 0, 1, 0, 4.2);

    if (ins.input_4 < 0.4) {
        feedback = map(ins.input_2, 0, 1, 0, 1.3);
        saturation = map(ins.input_7, 0, 1, 0, 40);
    } else {
        feedback = 1.04;
        saturation = 20;
    }

    hp_s.cutoff = 40 * exp(6.21 * ins.input_6); // 100 to 10000 exponentially
    hp_s.q = 1.7;
    delayLevel = delayLevel * map(ins.input_6, 0, 1, 1, 2);

    hpFilter.setup(hp_s);

    handleSpeedKnob(ins.input_3);
}

void Delay::setTempo(float period)
{
    speed = reelLength / period;
}

void Delay::handleSpeedKnob(float knobSpeed)
{
    if (speedKnobSamplerCntr == SPEED_KNOB_FS - 1) {
        speedKnobSamplerCntr = 0;

        if (fabs(prevKnobSpeed - knobSpeed) > 0.01)
            speed = map(knobSpeed, 0, 1, 15, 160);
            setPeriodCallback(reelLength / speed);

        prevKnobSpeed = knobSpeed;
    }
    speedKnobSamplerCntr++;
}
