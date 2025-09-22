#include <Bela.h>
#include <cmath>
#include <libraries/AudioFile/AudioFile.h>
#include <alsa/asoundlib.h>

#include "Sampler.hpp"

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

Sampler sampler;

static void abortIfLoud(float y)
{
    if (fabs(y) > 2) {
        printf("Too loud! out: %f\n", y);
        abort();
    }
}

bool setup(BelaContext *context, void *userData)
{
    printf("Fs: %f. Buffersize: %d\n", context->audioSampleRate, context->audioFrames);

    sampler.init(context->audioSampleRate);

    gInverseSampleRate = 1.0 / context->audioSampleRate;

    // if(context->audioFrames)
    //     gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    gAudioFramesPerAnalogFrame = 16;

    snd_pcm_hw_params_t *p_params;
    snd_pcm_hw_params_alloca(&p_params);

    return true;
}

void render(BelaContext *context, void *userData)
{
    for (unsigned int n = 0; n < context->audioFrames; n++) {
        if (gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            AnalogIns ins = {
                .input_0 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0),
                .input_1 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1),
                .input_2 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2),
                .input_3 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3),
                .input_4 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4),
                .input_5 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5),
                .input_6 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6),
                .input_7 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7)
            };

            sampler.setAnalogIns(ins);
        }

        float out[2] = {};
        sampler.process(out);
        abortIfLoud(out[0]);

        audioWrite(context, n, 0, out[0]);
        audioWrite(context, n, 1, out[1]);
    }
}

void cleanup(BelaContext *context, void *userData)
{

}
