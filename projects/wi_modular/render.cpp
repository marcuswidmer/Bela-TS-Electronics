#include <Bela.h>
#include <cmath>
#include "../main_project/mainCommon.hpp"
#include <libraries/AudioFile/AudioFile.h>
#include <alsa/asoundlib.h>

#include "WiModular.hpp"

// Marcus destroyed yet again
int gSensorInput0 = 0;
int gSensorInput1 = 1;
int gSensorInput2 = 2;
int gSensorInput3 = 3;
int gSensorInput4 = 4;
int gSensorInput5 = 5;
int gSensorInput6 = 6;
int gSensorInput7 = 7;

float gInverseSampleRate;

int gAudioFramesPerAnalogFrame = 0;

float programLevels[4] = {0.000000, 0.342, 0.68, 0.99};
float levelDiff = programLevels[1];

Pgm prevProgram = PgmUnused;

WiModular wiModular;

static void abortIfLoud(float y)
{
    if (fabs(y) > 2) {
        printf("Too loud! out: %f\n", y);
        abort();
    }
}

static Pgm setProgram(float val)
{
    if (val < programLevels[1] - (levelDiff / 2))
        return PgmRandomOctave;
    else if (val >= programLevels[1] - (levelDiff / 2) && val < programLevels[2] - (levelDiff / 2))
        return PgmOctaveSelector;
    else if (val >= programLevels[2] - (levelDiff / 2) && val < 0.75)
        return PgmSequencer;
    else if (val >= 0.75)
        return PgmSampler;
}

// static void abortIfLoud(float y)
// {
//     if (fabs(y) > 1.0) {
//         printf("Too loud! out: %f\n", y);
//         //abort();
//     }
// }

bool setup(BelaContext *context, void *userData)
{
    Bela_setADCLevel(-12.0); //This used to be -6. So changing to the old value to deal with guitar output level.
    gInverseSampleRate = 1.0 / context->audioSampleRate;

    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    int analogIOSampleRate = (int)context->analogSampleRate;
    wiModular.init(analogIOSampleRate, context->audioSampleRate);

    printf("Fs: %f. Analog fs: %d Buffersize: %d\n", context->audioSampleRate, analogIOSampleRate, context->audioFrames);

    pinMode(context, 0, 0, OUTPUT);
    return true;
}

void render(BelaContext *context, void *userData)
{
    for (unsigned int n = 0; n < context->audioFrames; n++) {

        if (gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            wiModular.analogIO.button = analogRead(context, n/gAudioFramesPerAnalogFrame, 0) > 0.5f;
            wiModular.analogIO.pot0 = 1.0f - analogRead(context, n/gAudioFramesPerAnalogFrame, 1); // Pots are soldered the wrong way
            wiModular.analogIO.pot1 = 1.0f - analogRead(context, n/gAudioFramesPerAnalogFrame, 2);
            wiModular.analogIO.pot2 = 1.0f - analogRead(context, n/gAudioFramesPerAnalogFrame, 3);
            Pgm pgm = setProgram(analogRead(context, n/gAudioFramesPerAnalogFrame, 4));
            if (pgm != wiModular.analogIO.selector) {
                wiModular.analogIO.selector = pgm;
                rt_printf("Setting pgm: %d\n", pgm);
                wiModular.setMidiClock(false);
            }

            wiModular.process();

            analogWrite(context, n / gAudioFramesPerAnalogFrame, 0, wiModular.analogIO.cvOut0);
            analogWrite(context, n / gAudioFramesPerAnalogFrame, 1, wiModular.analogIO.cvOut1);
            analogWrite(context, n / gAudioFramesPerAnalogFrame, 2, wiModular.analogIO.cvOut2);
            analogWrite(context, n / gAudioFramesPerAnalogFrame, 3, wiModular.analogIO.cvOut3);
        }
        if(context->digitalFrames > 0) {
            unsigned int dn = (unsigned int)((uint64_t)n * context->digitalFrames / context->audioFrames);
            digitalWrite(context, dn, 0, wiModular.analogIO.led);
        }

        float out[2] = {};
        wiModular.processAudio(out);
        abortIfLoud(out[0]);

        audioWrite(context, n, 0, out[0]);
        audioWrite(context, n, 1, out[1]);
    }
}

void cleanup(BelaContext *context, void *userData)
{

}
