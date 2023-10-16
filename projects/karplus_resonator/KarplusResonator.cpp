#include "KarplusResonator.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>
#include <libraries/math_neon/math_neon.h>
#include <stdlib.h>
#include <array>
#include <string.h>

#include <libraries/AudioFile/AudioFile.h>
#include "../main_project/mainCommon.hpp"

KarplusResonator::KarplusResonator()
{
    std::string gFilename = "../low-fi-machine/jazz_lick.wav";
    gNumFramesInFile = AudioFileUtilities::getNumFrames(gFilename);
    gSampleData = AudioFileUtilities::load(gFilename, gNumFramesInFile, 0)[0];

    filterSettings = {
        .fs = AUDIOSAMPLERATE_HZ,
        .type = Biquad::lowpass,
        .cutoff = 3000.0,  // Cut-off frequency for low-pass filter (Hz)
        .q = 1,
        .peakGainDb = 0};

    for (int j = 0; j < NUM_OCTAVES; j++) {
        for (int i = 0; i < NUM_NOTES; i++) {
            delayTimes[j][i] = AUDIOSAMPLERATE_HZ / (f0[i] * (j + 1));
        }
    }

    delayTimeDrone = AUDIOSAMPLERATE_HZ / (f0[0] / 2);
}

KarplusResonator::~KarplusResonator()
{
}

float KarplusResonator::process(float inSamp)
{
    float in = amplitude * inSamp;
    //float in = amplitude * readAudioInput();
//    float noise = drone * filter[0].process(gaussianNoise());

    float outSum = 0;

    for (int j = 0; j < NUM_OCTAVES; j++) {
        for (int i = 0; i < NUM_NOTES; i++) {

            int * delayCntr = &delayCnt[j][i];

            float out = line[j][i][*delayCntr];
            line[j][i][*delayCntr] = out * feedback + (in);

            (*delayCntr)++;

            if (*delayCntr >= delayTimes[j][i])
                *delayCntr = 0;

            outSum += out;
        }
    }

    return 1 / ((float)NUM_OCTAVES) * outSum;
}

void KarplusResonator::setAnalogIns(AnalogIns ins)
{
    amplitude = map(ins.input_0, 0, 1, 0, 0.1);
    drone = map(ins.input_1, 0 ,1, 0, 0.2);
    feedback = map(ins.input_2, 0 ,1, 0, 1.21);

    // octaves[0] = map(ins.input_3, 0 ,1, 0, 1.21);
    // octaves[1] = map(ins.input_4, 0 ,1, 0, 1.21);
    // octaves[2] = map(ins.input_5, 0 ,1, 0, 1.21);
    // octaves[3] = map(ins.input_6, 0 ,1, 0, 1.21);
    //delayTime = map(ins.input_2, 0, 1, 200, 700);

    filterSettings.cutoff = 40 * exp(6.21 * ins.input_3); // 100 to 10000 exponentially
    filterSettings.q = map(ins.input_4, 0, 1, 0, 20);

    filter[0].setup(filterSettings);
    filter[1].setup(filterSettings);
}

float KarplusResonator::readAudioInput()
{
    float out = gSampleData[readCounter];
    readCounter++;
    if (readCounter >= gNumFramesInFile)
        readCounter = 0;

    return out;
}

float KarplusResonator::gaussianNoise()
{
    float random = rand()/(float)RAND_MAX;
    float s = 0;
    for (int c=0; c<16; c++) s += random;
    return (s-8)/4;
}
