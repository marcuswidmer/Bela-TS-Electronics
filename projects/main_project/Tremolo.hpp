#ifndef TREMOLO_HPP
#define TREMOLO_HPP

#include <Bela.h>
#include <cmath>
#define TREMOLO_LENGTH_WAVEFORM 1000
#define TREMOLO_NUM_DUTY 10
#define TREMOLO_NUM_HARMONICS 50

struct AnalogIns;

class Tremolo
{
public:
    Tremolo();

    void process(float in, float out[2]);
    void setAnalogIns(AnalogIns ins);
    void incrGlobCounter();
    void init();


private:
	float amplitude_ = 0;
	float rate_ = 1.0f;
	int harmonic_ = 0;
	int duty_ = 0;
	float globCounter_ = 0.0f;
	float envs[TREMOLO_NUM_DUTY][TREMOLO_NUM_HARMONICS][TREMOLO_LENGTH_WAVEFORM] = {};
};

#endif