#ifndef SINEGENERATOR_HPP
#define SINEGENERATOR_HPP

#include <string>
#include <vector>

#include "../main_project/mainCommon.hpp"
#include "ResonatorPitchDetector.hpp"

class SineGenerator
{
public:
    SineGenerator(int rl);
    ~SineGenerator();
    float process();
    void setAnalogIns(struct AnalogIns ins);
    void setNoteVelocity(int note, int octave, float rms);
    int rampCntr;
    bool notePlaying(int note, int octave);
    void setNotePlaying(int note, int octave, bool state);

private:
    float amplitude;
    float phase[NUM_NOTES][NUM_OCTAVES];
    float noteRms[NUM_NOTES][NUM_OCTAVES];
    float noteRmsPrev[NUM_NOTES][NUM_OCTAVES];
    bool playing[NUM_NOTES][NUM_OCTAVES];
    int rampLength;
    //float f0[NUM_NOTES] = {n_A, n_As, n_B, n_C, n_Cs, n_D, n_Ds, n_E, n_F, n_Fs, n_G, n_Gs};
    //float f0[NUM_NOTES] = {n_E, n_G, n_A, n_B, n_D};
};

#endif