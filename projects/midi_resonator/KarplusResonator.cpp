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


    for (int i = 0; i < NUM_NOTES; i++) {
        for (int j = 0; j <= NUM_OCTAVES; j++) {
            delayTimes[i][j] = AUDIOSAMPLERATE_HZ / (f0[i] * (j + 1));
        }
    }
}

KarplusResonator::~KarplusResonator()
{
}

float KarplusResonator::rootMeanSquare(int note, int octave)
{
    return sqrtf(movingAverage[note][octave]);
}

static int modulo(int n, int M)
{
    return ((n % M) + M) % M;
}

float KarplusResonator::process(float inSamp)
{
    float inRaw = inSamp;
    //float inRaw = readAudioInput();
    float in = amplitude * filter.process(inRaw);

    //float outSum = 0;

    for (int i = 0; i < NUM_NOTES; i++) {
        for (int j = 0; j < NUM_OCTAVES; j++) {

            int * delayCntr = &delayCnt[i][j];

            float out = line[i][j][*delayCntr];
            line[i][j][*delayCntr] = out * feedback + (in);

            float * movingAv = &movingAverage[i][j];
            *movingAv = alpha * (*movingAv) + (1 - alpha) * (out * out);

            (*delayCntr)++;
            if (*delayCntr >= delayTimes[i][j])
                *delayCntr = 0;

            //outSum += out;
        }
    }

    //return 1 / ((float)NUM_OCTAVES) * outSum;
    return in;
    //return 0.0f;
}

void KarplusResonator::setAnalogIns(AnalogIns ins)
{
    amplitude = map(ins.input_0, 0, 1, 0, 0.1);
    feedback = map(ins.input_3, 0 ,1, 0, 1.21);
    alpha = log10(map(ins.input_2, 0 ,0.826, 1, 10000)) / 4;
    //printf("%f\n", log10(map(ins.input_1, 0 ,0.8255, 1, 10000)) / 4);
    // filterSettings.cutoff = 40 * exp(6.21 * ins.input_5); // 100 to 10000 exponentiallys
    // filterSettings.q = map(ins.input_6, 0, 0.826, 0, 20);
    //printf("f: %f, q: %f\n",filterSettings.cutoff, filterSettings.q);

    filter.setup(filterSettings);
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
