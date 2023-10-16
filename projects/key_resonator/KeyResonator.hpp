#ifndef KEYRESONATOR_HPP
#define KEYRESONATOR_HPP

#include <string>
#include <vector>
#include "../main_project/mainCommon.hpp"
#include <libraries/ne10/NE10.h>

#define RESONATOR_FFTSIZE (1024)
#define NUM_FRAMES_IN_H (1)

class KeyResonator
{
public:
    KeyResonator();
    ~KeyResonator();
    float process(float inSamp);
    void setAnalogIns(struct AnalogIns ins);

private:
    float readAudioInput();
    void initializeNE10Buffers();
    void partialConvolution();
    void copyInBufferToDelayMatrix();
    void complexMultiplication(int index);
    void loadTransferFunction();
    int gNumFramesInFile;
    std::vector<float> gSampleData;
    int readCounter;
    int inCounter;
    int delLineCounter;
    float amplitude;
    int current;

    ne10_fft_cpx_float32_t* timeDomainIn[2];
    ne10_fft_cpx_float32_t* timeDomainOut[2];
    ne10_fft_cpx_float32_t* frequencyDomain;
    ne10_fft_cfg_float32_t cfg;

    ne10_fft_cpx_float32_t* D[NUM_FRAMES_IN_H];
    ne10_fft_cpx_float32_t* TF[NUM_FRAMES_IN_H];
    ne10_fft_cpx_float32_t* C;

};

#endif