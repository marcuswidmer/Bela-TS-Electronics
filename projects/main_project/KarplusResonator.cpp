#include "KarplusResonator.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>
#include <libraries/math_neon/math_neon.h>
#include <stdlib.h>
#include <array>
#include <string.h>

#include <libraries/AudioFile/AudioFile.h>
#include "mainCommon.hpp"

KarplusResonator::KarplusResonator()
{
    for (int j = 0; j < KARPLUSRESONATOR_NUM_OCTAVES; j++) {
        for (int i = 0; i < KARPLUSRESONATOR_NUM_NOTES; i++) {
            delayTimes[j][i] = AUDIOSAMPLERATE_HZ / (f0[i] * (j + 1));
        }
    }
}

KarplusResonator::~KarplusResonator()
{
}

void KarplusResonator::process(float in, float out[2])
{
    in *= amplitude;
    float outSum = 0;

    for (int j = 0; j < num_octaves; j++) {
        for (int i = 0; i < num_notes; i++) {

            int * delayCntr = &delayCnt[j][i];

            float out = line[j][i][*delayCntr];
            line[j][i][*delayCntr] = out * feedback + (in);

            (*delayCntr)++;

            int delay_times_index = f0_indecies[i];
            if (*delayCntr >= delayTimes[j][(delay_times_index + offset) % KARPLUSRESONATOR_NUM_NOTES])
                *delayCntr = 0;

            outSum += out;
        }
    }

    float out_mono = 1 / ((float)num_octaves) * outSum;

    out[0] = out_mono;
    out[1] = out_mono;
}

void KarplusResonator::setAnalogIns(AnalogIns ins)
{
    amplitude = map(ins.input_1, 0, 1, 0, 0.1);
    feedback = map(ins.input_2, 0 ,1, 0, 1.21);
    num_notes = map(ins.input_3, 0 ,1, 1, 15);
    offset = map(ins.input_4, 0 ,1, 0, 14);
    num_octaves = map(ins.input_5, 0 ,1, 1, 10);
}

