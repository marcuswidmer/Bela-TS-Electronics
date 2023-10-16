#include <Bela.h>
#include <libraries/Midi/Midi.h>
#include <cmath>
#include "Wavetable.h"
#include "Ramp.h"
#include "ADSR.h"
#include <algorithm>
#include <vector>

#define NUM_VOICES 10
Wavetable gOscillator[NUM_VOICES];

ADSR gAmplitudeADSR[NUM_VOICES];

Midi midi;
Ramp velocityRamp[NUM_VOICES];
int voice = 0;
std::vector<int> notes(NUM_VOICES, 0);
float velocityRampTime = 0.1;

const char* gMidiPort0 = "hw:1,0,0";

bool noteOn[NUM_VOICES] = {};
bool noteOff[NUM_VOICES] = {};
float gFreq[NUM_VOICES] = {};
float gVelocity[NUM_VOICES] = {};
float gSamplingPeriod = 0;
float masterVolume = 0.5f;


void midiMessageCallback(MidiChannelMessage message, void* arg) {
	// if(arg != NULL){
	// 	rt_printf("Message from midi port %s ", (const char*) arg);
	// }
	//message.prettyPrint();
	if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
		if (message.getType() == kmmNoteOn) {
			noteOn[voice] = true;
			notes[voice] = message.getDataByte(0);
			rt_printf("Turnign on voice number: %d\n", voice);
			gFreq[voice] = powf(2, (message.getDataByte(0) - 69) / 12.f) * 440.f;
			gOscillator[voice].setFrequency(gFreq[voice]);
			gVelocity[voice] = message.getDataByte(1) / 128.0f;

			if (gAmplitudeADSR[voice].isActive())
				velocityRamp[voice].rampTo(gVelocity[voice], velocityRampTime);
			else
				velocityRamp[voice].setValue(gVelocity[voice]);

			voice++;
		}

		else if (message.getType() == kmmNoteOff) {
			auto it = find(notes.begin(), notes.end(), message.getDataByte(0));
			int theVoice = it - notes.begin();
			noteOff[theVoice] = true;
			rt_printf("Turnign off voice number: %d\n", theVoice);
			voice--;
		}

	} else if (message.getType() == kmmControlChange) {
		masterVolume = message.getDataByte(1) / 128.0f;
	}

}

bool setup(BelaContext *context, void *userData)
{
	std::vector<float> wavetable;
	const unsigned int wavetableSize = 512;
		
	// Populate a buffer with the first 64 harmonics of a sawtooth wave
	wavetable.resize(wavetableSize);
	for(unsigned int n = 0; n < wavetable.size(); n++) {
		wavetable[n] = 0;
		for(unsigned int harmonic = 1; harmonic <= 1; harmonic++) {
			wavetable[n] += 0.5 * sinf(2.0 * M_PI * (float)harmonic * (float)n /
								 (float)wavetable.size()) / (float)harmonic;
		}
	}
	
	for (int i = 0; i < NUM_VOICES; i++) {
		gOscillator[i].setup(context->audioSampleRate, wavetable);
		gAmplitudeADSR[i].setSampleRate(context->audioSampleRate);
		velocityRamp[i].setSampleRate(context->audioSampleRate);
	}

	midi.readFrom(gMidiPort0);
	midi.writeTo(gMidiPort0);
	midi.enableParser(true);
	midi.getParser()->setCallback(midiMessageCallback, (void*) gMidiPort0);
	gSamplingPeriod = 1 / context->audioSampleRate;

	return true;
}

void render(BelaContext *context, void *userData)
{
	// gAmplitudeADSR.setAttackTime(0.1);
	// gAmplitudeADSR.setDecayTime(0.1);
	// gAmplitudeADSR.setSustainLevel(0.8);
	// gAmplitudeADSR.setReleaseTime(0.6);

    for(unsigned int n = 0; n < context->audioFrames; n++) {

		float out = 0.0f;
    	for (int i = 0; i < NUM_VOICES; i++) {
			if (noteOn[i]) {
				gAmplitudeADSR[i].trigger();
				noteOn[i] = false;
			}

			if (noteOff[i]) {
				gAmplitudeADSR[i].release();
				noteOff[i] = false;
			}

	    	float amp = gAmplitudeADSR[i].process();
	    	float vel = velocityRamp[i].process();
	    	out += gOscillator[i].process() * amp * vel;
	    }
            
    	for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
    		audioWrite(context, n, channel, masterVolume * out);
    	}
    }
}

void cleanup(BelaContext *context, void *userData)
{

}
