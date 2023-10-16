/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

*/

#include <Bela.h>
#include <libraries/Midi/Midi.h>
#include <stdlib.h>
#include <cmath>

float amplitude = 0;
float gSamplingPeriod = 0;
int gSampleCount = 441; // how often to send out a control change

// INPUT MAPPING!!! Wierd because I destroyed the Bela
int gSensorInput0 = 5;
int gSensorInput1 = 6;
int gSensorInput2 = 1;
int gSensorInput3 = 4;
int gSensorInput4 = 2;
int gSensorInput5 = 3;
int gSensorInput6 = 7;
int gSensorInput7 = 0;

int gAudioFramesPerAnalogFrame = 0;

float amplitudes[8] = {0};

Midi midi;

const int rms_frame_size = 441;
int rms_cntr = 0;
float rms_array[rms_frame_size] = {0};
float rms = 0;

const char* gMidiPort0 = "hw:0,0";

float calcRms(float * rmsArray, int len)
{
	float rms = 0;
	for (int i = 0; i < len; i++)
		rms += 1.0f / len * sqrt(rmsArray[i] * rmsArray[i]);
	return rms;
}

//float Calc

bool setup(BelaContext *context, void *userData)
{
    if(context->audioFrames)
	    gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

	midi.readFrom(gMidiPort0);
	midi.writeTo(gMidiPort0);
	gSamplingPeriod = 1 / context->audioSampleRate;
	return true;
}

void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
        if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            amplitudes[0] = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0), 0, 1, 0, 127);
            amplitudes[1] = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1), 0, 1, 0, 127);
            amplitudes[2] = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2), 0, 1, 0, 127);
            amplitudes[3] = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3), 0, 1, 0, 127);
            amplitudes[4] = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4), 0, 1, 0, 127);
            amplitudes[5] = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5), 0, 1, 0, 127);
            amplitudes[6] = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6), 0, 1, 0, 127);
            amplitudes[7] = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7), 0, 1, 0, 127);
        }

        // float out = audioRead(context, n, 0);
        // rms_array[rms_cntr] = out;
        // rms_cntr++;
        // if (rms_cntr >= rms_frame_size) {
        // 	rms = 128 * pow(calcRms(rms_array, rms_frame_size), 0.4);
        // 	rms_cntr = 0;
        // }



		static int count = 0;
		if (count % gSampleCount == 0){
			midi_byte_t bytes[3];
			bytes[0] = 0xB0;

			for (int i = 0; i < 8; i++) {
				bytes[1] = i;
				bytes[2] = amplitudes[i];
				midi.writeOutput(bytes, 3); // send a control change message
			}
		}
		++count;
	}
}

void cleanup(BelaContext *context, void *userData)
{

}