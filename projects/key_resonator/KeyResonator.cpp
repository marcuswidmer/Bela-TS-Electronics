#include "KeyResonator.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

#include <libraries/AudioFile/AudioFile.h>
#include <transfer_function_partitioned.h>
#include "../main_project/mainCommon.hpp"

KeyResonator::KeyResonator()
{
    std::string gFilename = "../low-fi-machine/jazz_lick.wav";
    gNumFramesInFile = AudioFileUtilities::getNumFrames(gFilename);
    gSampleData = AudioFileUtilities::load(gFilename, gNumFramesInFile, 0)[0];

    timeDomainIn[0] = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESONATOR_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    timeDomainIn[1] = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESONATOR_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    timeDomainOut[0] = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESONATOR_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    timeDomainOut[1] = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESONATOR_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    frequencyDomain = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESONATOR_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    cfg = ne10_fft_alloc_c2c_float32_neon (RESONATOR_FFTSIZE);

    for (int i = 0; i < NUM_FRAMES_IN_H; i++) {
        D[i] = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESONATOR_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
        TF[i] = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESONATOR_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    }

    C = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESONATOR_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));

    initializeNE10Buffers();
    loadTransferFunction();
}

KeyResonator::~KeyResonator()
{
    NE10_FREE(frequencyDomain);
    NE10_FREE(cfg);
}

float KeyResonator::process(float inSamp)
{
    float out = readAudioInput();

    timeDomainIn[current][inCounter].r = out;
    timeDomainIn[current][inCounter].i = 0.0f;

    float outSamp = timeDomainOut[current][inCounter].r + timeDomainOut[!current][inCounter].r;

    inCounter++;
    if (inCounter >= RESONATOR_FFTSIZE / 2) {
        inCounter = 0;

        partialConvolution();

        delLineCounter++;
        if (delLineCounter >= NUM_FRAMES_IN_H)
            delLineCounter = 0;

        current = !current;// == 0 ? 1 : 0
    }

    return amplitude * outSamp;
}

void KeyResonator::setAnalogIns(AnalogIns ins)
{
    amplitude = ins.input_0;
}

float KeyResonator::readAudioInput()
{
    float out = gSampleData[readCounter];
    readCounter++;
    if (readCounter >= gNumFramesInFile)
        readCounter = 0;

    return out;
}

void KeyResonator::partialConvolution()
{
    // FFT
    ne10_fft_c2c_1d_float32_neon(frequencyDomain, timeDomainIn[current], cfg, 0);

    copyInBufferToDelayMatrix();

    for (int i = 0; i < NUM_FRAMES_IN_H; i++)
        complexMultiplication(i);

    // iFFT
    ne10_fft_c2c_1d_float32_neon(timeDomainOut[current], C, cfg, 1);

}

void KeyResonator::initializeNE10Buffers()
{
    for (int n = 0; n < RESONATOR_FFTSIZE; n++) {
        timeDomainIn[0][n].r = 0.0f;
        timeDomainIn[0][n].i = 0.0f;
        timeDomainIn[1][n].r = 0.0f;
        timeDomainIn[1][n].i = 0.0f;
        timeDomainOut[0][n].r = 0.0f;
        timeDomainOut[0][n].i = 0.0f;
        timeDomainOut[1][n].r = 0.0f;
        timeDomainOut[1][n].i = 0.0f;
        frequencyDomain[n].r = 0.0f;
        frequencyDomain[n].i = 0.0f;
     }

    for (int n = 0; n < RESONATOR_FFTSIZE; n++) {
        for (int k = 0; k < NUM_FRAMES_IN_H; k++) {
            D[k][n].r = 0.0f;
            D[k][n].i = 0.0f;
        }
    }
}

void KeyResonator::copyInBufferToDelayMatrix()
{
    for (int n = 0; n < RESONATOR_FFTSIZE; n++) {
        D[delLineCounter][n].r = frequencyDomain[n].r;
        D[delLineCounter][n].i = frequencyDomain[n].i;
    }
}

void KeyResonator::complexMultiplication(int index)
{
    int delIndex = PYTHON_MOD(delLineCounter - index, NUM_FRAMES_IN_H);
    float multi = (index == 0) ? 0.0f : 1.0f;

    for (int n = 0; n < RESONATOR_FFTSIZE; n++) {
        C[n].r = multi * C[n].r + D[delIndex][n].r * TF[index][n].r - D[delIndex][n].i * TF[index][n].i;
        C[n].i = multi * C[n].i + D[delIndex][n].r * TF[index][n].i + D[delIndex][n].i * TF[index][n].r;
    }
}
void KeyResonator::loadTransferFunction()
{
    for (int n = 0; n < NUM_FRAMES_IN_H; n++) {
        for (int k = 0; k < RESONATOR_FFTSIZE; k++) {
            TF[n][k].r = H[n][2 * k];
            TF[n][k].i = H[n][2 * k + 1];
        }
    }
}
