#include "SineGenerator.hpp"
#include <math.h>
#include <Bela.h>

SineGenerator::SineGenerator(int rl)
	: rampLength(rl)
{
}

SineGenerator::~SineGenerator()
{
}

float SineGenerator::process()
{
	float out = 0.0f;

	float frac = (float)rampCntr / rampLength;

	rampCntr++;
	if (rampCntr >= rampLength)
		rampCntr = rampLength;

	for (int i = 0; i < NUM_NOTES; i++) {
		for (int j = 0; j < NUM_OCTAVES; j++) {
			out += amplitude * (frac * noteRms[i][j] + (1 - frac) * noteRmsPrev[i][j])  * sinf(phase[i][j]);

			phase[i][j] += 2.0f * (float)M_PI * 1 * (j+1) * f0[i] / (float)AUDIOSAMPLERATE_HZ;

			if (phase[i][j] > M_PI)
				phase[i][j] -= 2.0f * (float)M_PI;
		}
	}

	return out;
}

void SineGenerator::setAnalogIns(AnalogIns ins)
{
    amplitude = map(ins.input_1, 0, 1, 0, 3);
}

void SineGenerator::setNoteVelocity(int note, int octave, float rms)
{
	noteRmsPrev[note][octave] = noteRms[note][octave];
	noteRms[note][octave] = rms;
}

bool SineGenerator::notePlaying(int note, int octave)
{
	return playing[note][octave];
}

void SineGenerator::setNotePlaying(int note, int octave, bool state)
{
	playing[note][octave] = state;
}
