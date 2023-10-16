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

}

void Tremolo::process(float in, float out[2])
{
    float ampl = squareWave[(int)globCounter_];

    out[0] = amplitude * (ampl + 1) * in;
    out[1] = amplitude * (ampl + 1) * in;

    incrGlobCounter();
}

void Tremolo::setAnalogIns(AnalogIns ins)
{
    amplitude = map(ins.input_1, 0, 1, 0, 1);
    rate_ = map(ins.input_2, 0, 1, 0, 1);
}

void Tremolo::incrGlobCounter()
{
    globCounter_ = globCounter_ + rate_;
    if (globCounter_ >= LENGTH_WAVEFORM)
        globCounter_ = 0;
}
