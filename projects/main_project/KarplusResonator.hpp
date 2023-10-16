#ifndef KARPLUSRESONATOR_HPP
#define KARPLUSRESONATOR_HPP

#include <string>
#include <vector>

#include "../main_project/mainCommon.hpp"

#define KARPLUSRESONATOR_MAX_DELAY 600
#define KARPLUSRESONATOR_NUM_NOTES 12
#define KARPLUSRESONATOR_NUM_OCTAVES 12

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
    void process(float in, float out[2]);
    void setAnalogIns(AnalogIns ins);

private:
    float line[KARPLUSRESONATOR_NUM_OCTAVES][KARPLUSRESONATOR_NUM_NOTES][KARPLUSRESONATOR_MAX_DELAY] = {};
    int delayTimes[KARPLUSRESONATOR_NUM_OCTAVES][KARPLUSRESONATOR_NUM_NOTES] = {};
    int delayCnt[KARPLUSRESONATOR_NUM_OCTAVES][KARPLUSRESONATOR_NUM_NOTES] = {};

    float amplitude;
    float feedback;
    int num_notes;
    int num_octaves;
    int offset;

    //float f0[KARPLUSRESONATOR_NUM_NOTES] = {n_A, n_As, n_B, n_C, n_Cs, n_D, n_Ds, n_E, n_F, n_Fs, n_G, n_Gs};
    //float f0[KARPLUSRESONATOR_NUM_NOTES] = {n_G, n_A, n_B, n_C, n_D, n_E, n_Fs};
    //float f0[KARPLUSRESONATOR_NUM_NOTES] = {n_E, n_G, n_A, n_B, n_D};
    float f0[KARPLUSRESONATOR_NUM_NOTES] = {n_A, n_As, n_B, n_C, n_Cs, n_D, n_Ds, n_E, n_F, n_Fs, n_G, n_Gs};
    //float f0_indecies[KARPLUSRESONATOR_NUM_NOTES] = {n_E, n_G, n_A, n_B, n_D, n_C, n_Fs, n_As, n_Cs, n_Ds, n_F, n_Gs};
    float f0_indecies[KARPLUSRESONATOR_NUM_NOTES] = {7, 10, 0, 2, 5, 3, 9, 1, 4, 6, 8, 11};
    //float f0[KARPLUSRESONATOR_NUM_NOTES_NUM_NOTES] = {n_E};
};

#endif