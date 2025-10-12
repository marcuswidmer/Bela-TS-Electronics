/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
  Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
  Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/


#include "usb_interface.hpp"

#include <Bela.h>
#include <cmath>
#include "../main_project/mainCommon.hpp"
#include <alsa/asoundlib.h>

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
float phase = 0.0f;

int gAudioFramesPerAnalogFrame = 0;

snd_pcm_t *capture;
snd_pcm_t *playback;

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

    snd_pcm_hw_params_t *p_params;
    snd_pcm_hw_params_alloca(&p_params);
    
    int ret = usb_interface_open_and_initialise(&capture, &playback, "plughw:1");

    if (ret == -1) {
        printf("failed to open usb interface");
        return false;
    }

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
        }

        phase += 2.0f * M_PI * 440.0f * gInverseSampleRate;
        if (phase > (2*M_PI))
            phase -= 2*M_PI;

        float in = 0.1 * sin(phase);

        abortIfLoud(in);

        audioWrite(context, n, 0, in);
        audioWrite(context, n, 1, in);
    }

    char buffer[context->audioFrames * snd_pcm_format_width(SND_PCM_FORMAT_S32_LE) / 8 * USB_INTERFACE_NUM_CHANNELS];
    usb_interface_readi(&capture, buffer, context->audioFrames);
    usb_interface_writei(&playback, buffer, context->audioFrames);
}

void cleanup(BelaContext *context, void *userData)
{

}
