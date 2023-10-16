#ifndef KARPLUSRESONATOR_HPP
#define KARPLUSRESONATOR_HPP

#include <string>
#include <vector>

#include "../main_project/mainCommon.hpp"
#include <libraries/Biquad/Biquad.h>
#include "ResonatorPitchDetector.hpp"

#define MAX_DELAY 600
#define KR_LOOK_WINDOW 500

class KarplusResonator
{
public:
    KarplusResonator();
    ~KarplusResonator();
    float process(float inSamp);
    void setAnalogIns(struct AnalogIns ins);
    float rootMeanSquare(int note, int octave);
    float readAudioInput();

private:
    float gaussianNoise();
    int gNumFramesInFile;
    std::vector<float> gSampleData;
    float line[NUM_NOTES][NUM_OCTAVES][MAX_DELAY];
    int readCounter;
    float amplitude;
    int delayTimes[NUM_NOTES][NUM_OCTAVES];
    int delayCnt[NUM_NOTES][NUM_OCTAVES];
    float movingAverage[NUM_NOTES][NUM_OCTAVES];
    int maBuffCntr[NUM_NOTES][NUM_OCTAVES];

    float feedback;
    float alpha;
    //float f0[NUM_NOTES] = {n_A, n_As, n_B, n_C, n_Cs, n_D, n_Ds, n_E, n_F, n_Fs, n_G, n_Gs};
    //float f0[NUM_NOTES] = {n_G, n_A, n_B, n_C, n_D, n_E, n_Fs};
    //float f0[NUM_NOTES] = {n_E, n_G, n_A, n_B, n_D};
    //float f0[NUM_NOTES] = {n_E};

    Biquad filter;
    BiquadCoeff::Settings filterSettings;
};

#endif