/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

*/

#include <Bela.h>
#include <cmath>

#include "KarplusResonator.hpp"
#include "SineGenerator.hpp"
#include "../main_project/mainCommon.hpp"
#include <libraries/Midi/Midi.h>

#define NUM_CHANNELS 1    // NUMBER OF CHANNELS IN THE FILE

// Marcus destroyed again
int gSensorInput0 = 6;
int gSensorInput1 = 7;
int gSensorInput2 = 3;
int gSensorInput3 = 5;
int gSensorInput4 = 0;
int gSensorInput5 = 2;
int gSensorInput6 = 4;
int gSensorInput7 = 1;

int gAudioFramesPerAnalogFrame = 0;
float gInverseSampleRate;
//int updateInterval = KR_LOOK_WINDOW;
int updateInterval = 441;
float amplitude = 0;

KarplusResonator karplusResonator;
SineGenerator sineGenerator(updateInterval);

int analogInCntr = 0;
float rmsThreshold = 0.0f;

Midi midi;
const char* gMidiPort0 = "hw:1,0,0";

static void abortIfLoud(float y)
{
    if (fabs(y) > 1.4) {
        printf("Too loud! out: %f\n", y);
        abort();
    }
}

bool setup(BelaContext *context, void *userData)
{
    Bela_setADCLevel(-6.0); //This used to be -6. So changing to the old value to deal with guitar output level.

    gInverseSampleRate = 1.0 / context->audioSampleRate;

    if(context->audioFrames)
        //gAudioFramesPerAnalogFrame = AUDIOSAMPLERATE_HZ;
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    midi.readFrom(gMidiPort0);
    midi.writeTo(gMidiPort0);
    midi.enableParser(true);

    for (int i = 0; i < 128; i++)
        midi.writeNoteOff(0, i, 0);

    return true;
}

void render(BelaContext *context, void *userData)
{
    for (unsigned int n = 0; n < context->audioFrames; n++) {
        if (analogInCntr == 0) {
            struct AnalogIns ins = {
                .input_0 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0),
                .input_1 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1),
                .input_2 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2),
                .input_3 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3),
                .input_4 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4),
                .input_5 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5),
                .input_6 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6),
                .input_7 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7)
            };
            rmsThreshold = map(ins.input_4, 0, 1, 0, 0.4);
            //printf("0: %f 1: %f 2: %f 3: %f 4: %f 5: %f 6: %f 7: %f\n", ins.input_0, ins.input_1, ins.input_2, ins.input_3, ins.input_4, ins.input_5, ins.input_6, ins.input_7);
            karplusResonator.setAnalogIns(ins);
            sineGenerator.setAnalogIns(ins);
        }

        float out = audioRead(context, n, 0);
        //float out = karplusResonator.readAudioInput();
        float filteredIn = karplusResonator.process(out);

        analogInCntr++;
        if (analogInCntr >= updateInterval) {
            analogInCntr = 0;

            for (int i = 0; i < NUM_NOTES; i++) {
                for (int j = 0; j < NUM_OCTAVES; j++) {
                    float rms = karplusResonator.rootMeanSquare(i, j);
                    sineGenerator.setNoteVelocity(i, j, rms > rmsThreshold ? rms : 0.0f);
                    midi.writeNoteOn(0, 3 * 12 + 21 + note_idx[i] + (j * NUM_NOTES), (int)(rms * 127));
                    sineGenerator.setNotePlaying(i,j,true);
                }
            }

            sineGenerator.rampCntr = 0;
        }


        float sineout = sineGenerator.process();

        abortIfLoud(sineout);

        audioWrite(context, n, 0, sineout);
        audioWrite(context, n, 1, sineout);
    }
}

void cleanup(BelaContext *context, void *userData)
{
}
