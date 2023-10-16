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


#include <Bela.h>
#include <cmath>
#include "../main_project/mainCommon.hpp"
#include <libraries/AudioFile/AudioFile.h>

#include "GranularReverb.hpp"
#include "KarplusResonator.hpp"
#include "LoFiMachine.hpp"
#include "Tremolo.hpp"


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

std::string gFilename = "../low-fi-machine/jazz_lick.wav";
int gNumFramesInFile;
std::vector<float> gSampleData;

enum Program {
    pgmGranularReverb,
    pgmKarplusResonator,
    pgmLofiMachine,
    pgmTremolo,
};

int gAudioFramesPerAnalogFrame = 0;

Program pgm;
GranularReverb granularReverb;
KarplusResonator karplusResonator;
LoFiMachine lofiMachine;
Tremolo tremolo;

float programLevels[4] = {0.000000, 0.274918, 0.549789, 0.825043};
float levelDiff = programLevels[1];

static float readAudioTestInput()
{
    // Increment read pointer and reset to 0 when end of file is reached
    static int readCounter;
    float out = gSampleData[readCounter];
    readCounter++;
    if (readCounter >= gNumFramesInFile)
        readCounter = 0;

    return out;
}

static void setProgram(AnalogIns ins)
{
    if (ins.input_0 < programLevels[1] - (levelDiff / 2))
        pgm = pgmTremolo;
    else if (ins.input_0 >= programLevels[1] - (levelDiff / 2) && ins.input_0 < programLevels[2] - (levelDiff / 2))
        pgm = pgmLofiMachine;
    else if (ins.input_0 >= programLevels[2] - (levelDiff / 2) && ins.input_0 < programLevels[3] - (levelDiff / 2))
        pgm = pgmKarplusResonator;
    else if (ins.input_0 >= programLevels[3] - (levelDiff / 2))
        pgm = pgmGranularReverb;
}

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

    lofiMachine.setSampleRateAndFilterSettings(context->audioSampleRate);

    gInverseSampleRate = 1.0 / context->audioSampleRate;

    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    gNumFramesInFile = AudioFileUtilities::getNumFrames(gFilename);
    gSampleData = AudioFileUtilities::load(gFilename, gNumFramesInFile, 0)[0];

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
            //printf("%f, %f, %f, %f, %f, %f, %f, %f\n", ins.input_0, ins.input_1, ins.input_2, ins.input_3, ins.input_4, ins.input_5, ins.input_6, ins.input_7);
            setProgram(ins);

            karplusResonator.setAnalogIns(ins);
            granularReverb.setAnalogIns(ins);
            lofiMachine.setAnalogIns(ins);
            tremolo.setAnalogIns(ins);
        }

        //float in = readAudioTestInput();
        float in = audioRead(context, n, 0);
        float out[2] = {};

        switch (pgm) {
            case pgmGranularReverb:
                granularReverb.process(in, out);
                break;
            case pgmKarplusResonator:
                karplusResonator.process(in, out);
                break;
            case pgmLofiMachine:
                lofiMachine.process(in, out);
                break;
            case pgmTremolo:
                tremolo.process(in, out);
                break;
            default:
                break;
        }

        abortIfLoud(out[0]);

        audioWrite(context, n, 0, out[0]);
        audioWrite(context, n, 1, out[1]);
    }
}

void cleanup(BelaContext *context, void *userData)
{

}
