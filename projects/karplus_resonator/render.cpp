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
#include "../main_project/mainCommon.hpp"

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

KarplusResonator karplusResonator;

int analogInCntr = 0;

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

            karplusResonator.setAnalogIns(ins);

        }
        analogInCntr++;
        if (analogInCntr >= 441)
            analogInCntr = 0;

        float out = audioRead(context, n, 0);
        float krOut = karplusResonator.process(out);

        abortIfLoud(krOut);

        audioWrite(context, n, 0, krOut);
        audioWrite(context, n, 1, krOut);

    }
}

void cleanup(BelaContext *context, void *userData)
{
}
