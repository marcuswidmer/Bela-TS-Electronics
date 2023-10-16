/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

*/

#include <Bela.h>
#include <cmath>

#include "KeyResonator.hpp"
#include "../main_project/mainCommon.hpp"

#define NUM_CHANNELS 1    // NUMBER OF CHANNELS IN THE FILE

// INPUT MAPPING!!! Wierd because I destroyed the Bela

//Elias
// int gSensorInput0 = 0;
// int gSensorInput1 = 1;
// int gSensorInput2 = 2;
// int gSensorInput3 = 3;
// int gSensorInput4 = 4;
// int gSensorInput5 = 5;
// int gSensorInput6 = 6;
// int gSensorInput7 = 7;

// Marcus
int gSensorInput0 = 5;
int gSensorInput1 = 6;
int gSensorInput2 = 1;
int gSensorInput3 = 4;
int gSensorInput4 = 2;
int gSensorInput5 = 3;
int gSensorInput6 = 7;
int gSensorInput7 = 0;

int gAudioFramesPerAnalogFrame = 0;
float gInverseSampleRate;

KeyResonator keyResonator;

static void abortIfLoud(float y)
{
    if (fabs(y) > 2) {
        printf("Too loud! out: %f\n", y);
        abort();
    }
}

bool setup(BelaContext *context, void *userData)
{
    Bela_setADCLevel(-6.0); //This used to be -6. So changing to the old value to deal with guitar output level.

    gInverseSampleRate = 1.0 / context->audioSampleRate;

    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    return true;
}

void render(BelaContext *context, void *userData)
{
    for (unsigned int n = 0; n < context->audioFrames; n++) {
        if (gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
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

            keyResonator.setAnalogIns(ins);

        }

        float out = audioRead(context, n, 0);
        //float out = audioRead(context, n, 1); // Elias reads from 1
        float krOut = keyResonator.process(out);


        abortIfLoud(krOut);

        audioWrite(context, n, 0, krOut);
        audioWrite(context, n, 1, krOut);

    }
}

void cleanup(BelaContext *context, void *userData)
{
}
