#include <Bela.h>
#include <cmath>
#include "../main_project/mainCommon.hpp"
#include <libraries/AudioFile/AudioFile.h>
#include <alsa/asoundlib.h>

#include "WiSynthesizer.hpp"

// Marcus destroyed again
int gSensorInput0 = 6;
int gSensorInput1 = 7;
int gSensorInput2 = 3;
int gSensorInput3 = 5;
int gSensorInput4 = 0;
int gSensorInput5 = 2;
int gSensorInput6 = 4;
int gSensorInput7 = 1;

float gInverseSampleRate;


int gAudioFramesPerAnalogFrame = 0;

float programLevels[4] = {0.000000, 0.274918, 0.549789, 0.825043};
float levelDiff = programLevels[1];

WiSynthesizer wiSynth(44100);

static void abortIfLoud(float y)
{
    if (fabs(y) > 2) {
        printf("Too loud! out: %f\n", y);
        abort();
    }
}

bool setup(BelaContext *context, void *userData)
{
    // Check if analog channels are enabled
    Bela_setADCLevel(-6.0); //This used to be -6. So changing to the old value to deal with guitar output level.

    printf("Fs: %f. Buffersize: %d\n", context->audioSampleRate, context->audioFrames);

    gInverseSampleRate = 1.0 / context->audioSampleRate;

    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
    wiSynth.init();

    // snd_pcm_hw_params_t *p_params;
    // snd_pcm_hw_params_alloca(&p_params);

    return true;
}

void render(BelaContext *context, void *userData)
{
    for (unsigned int n = 0; n < context->audioFrames; n++) {
        // if (gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
        //     AnalogIns ins = {
        //         .input_0 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0),
        //         .input_1 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1),
        //         .input_2 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2),
        //         .input_3 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3),
        //         .input_4 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4),
        //         .input_5 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5),
        //         .input_6 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6),
        //         .input_7 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7)
        //     };
        //     //printf("%f, %f, %f, %f, %f, %f, %f, %f\n", ins.input_0, ins.input_1, ins.input_2, ins.input_3, ins.input_4, ins.input_5, ins.input_6, ins.input_7);
        // }

        float in[2] = {};
        in[0] = audioRead(context, n, 0);
        in[1] = audioRead(context, n, 1);
        float out[2] = {};
        //wiSynth.process(out);
        abortIfLoud(out[0]);

        audioWrite(context, n, 0, in[0]);
        audioWrite(context, n, 1, in[1]);
    }
}

void cleanup(BelaContext *context, void *userData)
{

}
