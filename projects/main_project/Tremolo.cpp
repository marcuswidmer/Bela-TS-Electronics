#include "Tremolo.hpp"
#include "../main_project/mainCommon.hpp"

#include <stdio.h>


Tremolo::Tremolo()
{


// b = 10
// a = 1000
// numWaves = 10
// cont = np.zeros([a, b])

// allWaves = np.zeros([a,numWaves])
// for waveNum in range(numWaves):
//     sumWave = np.zeros(a)
//     for i in range(a):
//         for k in range(waveNum + 1):
//             float sample = 1 / ((2 * k) + 1) * math.sin(((2 * k) + 1) * 2 * np.pi * i / a);
//             sumWave[i] += sample;

//     float * wave = allWaves[]
//     for (int i = 0; i < a; i++)
//         allWaves[:,waveNum] = sumWave;

    //for (int n = 0; n < TREMOLO_LENGTH_WAVEFORM; n++) {
    //    for (int h = 0; h < TREMOLO_NUM_HARMONICS; h++) {
    //        float sample = 1 / ((2 * k) + 1) * math.sin(((2 * k) + 1) * 2 * M_PI * n / TREMOLO_LENGTH_WAVEFORM);
    //        env[]
    //    }
    //}

}

void Tremolo::init()
{
    float T = 1.0f;
    float A = 1.0f;

    for (int d = 0; d < TREMOLO_NUM_DUTY; d++) {
        float T_P = d / (float)(TREMOLO_NUM_DUTY - 1);

        for (int n = 0; n < TREMOLO_LENGTH_WAVEFORM; n++) {
            float sumHarmonics = A * T_P / T; // The sum up to "h" harmonics.
            envs[d][0][n] = sumHarmonics;

            for (int h = 1; h < TREMOLO_NUM_HARMONICS; h++) {
                float t = -T / 2 + n / (float)(TREMOLO_LENGTH_WAVEFORM - 1) * T;
                float sample = 2 * A / (h * M_PI) * sinf(h * M_PI * T_P / T) * cosf(h * 2.0f * M_PI / T * t);

                sumHarmonics += sample;
                envs[d][h][n] = sumHarmonics;
            }
        }
    }
}

void Tremolo::process(float in, float out[2])
{
    float env = envs[duty_][harmonic_][(int)globCounter_];
    //printf("Harmonic: %d, rate_: %f\n", harmonic_, rate_);
    out[0] = amplitude_ * env * in;
    out[1] = amplitude_ * env * in;

    incrGlobCounter();
}

void Tremolo::setAnalogIns(AnalogIns ins)
{
    amplitude_ = map(ins.input_1, 0, 1, 0, 1);
    rate_ = map(ins.input_2, 0, 1, 0, 1);
    harmonic_ = map(ins.input_3, 0, 1, 0, TREMOLO_NUM_HARMONICS);
    duty_ = map(ins.input_4, 0, 1, 0, TREMOLO_NUM_DUTY);
}

void Tremolo::incrGlobCounter()
{
    globCounter_ = globCounter_ + rate_;
    if (globCounter_ >= TREMOLO_LENGTH_WAVEFORM)
        globCounter_ = 0;
}



