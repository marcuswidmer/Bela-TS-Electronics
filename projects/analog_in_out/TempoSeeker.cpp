#include "TempoSeeker.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

#define MAX_PERIOD (2 * 44100)

const float minimumAmplitude = (1.5 / 5.0);
const float analogInScaler = 0.85f;

TempoSeeker::TempoSeeker()
{
    tempoLed = 0;
    ledOn = 750;
    ledOnSec = (float)ledOn / AUDIOSAMPLERATE_HZ;
    period = 11011;

    periodLed.setSampleRate(AUDIOSAMPLERATE_HZ);
    buttonPushLed.setSampleRate(AUDIOSAMPLERATE_HZ);
}

void TempoSeeker::process()
{
	if ((prevButtonSignal < analogInScaler / 2.0f && buttonSignal > analogInScaler / 2.0f)) {

		if (sinceLastPushCntr != MAX_PERIOD) {
			period = sinceLastPushCntr;
		}

		sinceLastPushCntr = 0;
		risingEdge = true;
		risingEdgeCntr = ledOn;
	}

	if (periodCntr == 0)
		periodLed.rampTo(1.0, ledOnSec / 4);

	if (periodCntr == ledOn / 2)
		periodLed.rampTo(0.0, ledOnSec / 4);

	if (risingEdgeCntr == ledOn)
		buttonPushLed.rampTo(1.0, ledOnSec / 4);

	if (risingEdgeCntr == ledOn / 2)
		buttonPushLed.rampTo(0.0, ledOnSec / 4);

	float led = periodLed.process() + buttonPushLed.process();
	tempoLed = led > 1.0 ? 1.0 : led;

	if (periodCntr++ >= period)
		periodCntr = 0;

	if (risingEdgeCntr-- < 0)
		risingEdge = false;

	if (sinceLastPushCntr++ >= MAX_PERIOD)
		sinceLastPushCntr = MAX_PERIOD;

	prevButtonSignal = buttonSignal;
}

void TempoSeeker::setAnalogIns(AnalogIns ins)
{
	buttonSignal = ins.input_0;
}
