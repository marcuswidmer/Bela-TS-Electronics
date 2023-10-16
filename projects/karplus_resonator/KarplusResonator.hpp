#ifndef KARPLUSRESONATOR_HPP
#define KARPLUSRESONATOR_HPP

#include <string>
#include <vector>

#include "../main_project/mainCommon.hpp"
#include <libraries/Biquad/Biquad.h>

#define MAX_DELAY 600
#define NUM_NOTES 12
#define NUM_OCTAVES 1

#define n_A (440.00 / 4)
#define n_As (466.16 / 4)
#define n_B (493.88 / 4)
#define n_C (523.25 / 4)
#define n_Cs (554.37 / 4)
#define n_D (587.33 / 4)
#define n_Ds (622.25 / 4)
#define n_E (659.26 / 4)
#define n_F (698.46 / 4)
#define n_Fs (739.99 / 4)
#define n_G (783.99 / 4)
#define n_Gs (830.60 / 4)

class KarplusResonator
{
public:
    KarplusResonator();
    ~KarplusResonator();
    float process(float inSamp);
    void setAnalogIns(struct AnalogIns ins);

private:
    float readAudioInput();
    float gaussianNoise();
    int gNumFramesInFile;
    std::vector<float> gSampleData;
    float line[NUM_OCTAVES][NUM_NOTES][MAX_DELAY];
    float droneLine[MAX_DELAY];
    int readCounter;
    float amplitude;
    int delayTimes[NUM_OCTAVES][NUM_NOTES];
    int delayCnt[NUM_OCTAVES][NUM_NOTES];

    int delayTimeDrone;
    int delayCntDrone;

    float feedback;
    float f0[NUM_NOTES] = {n_A, n_As, n_B, n_C, n_C, n_D, n_D, n_E, n_F, n_F, n_G, n_Gs};
    //float f0[NUM_NOTES] = {n_G, n_A, n_B, n_C, n_D, n_E, n_Fs};
    //float f0[NUM_NOTES] = {n_E, n_G, n_A, n_B, n_D};
    //float f0[NUM_NOTES] = {n_E};

    float octaves[NUM_OCTAVES];
    float drone;

    Biquad filter[2];
    BiquadCoeff::Settings filterSettings;
};

#endif