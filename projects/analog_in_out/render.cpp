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
#include "TempoSeeker.hpp"
#include "Delay.hpp"
#include <libraries/AudioFile/AudioFile.h>


std::string gFilename = "snare_weight_off.wav";

int gNumFramesInFile;
std::vector<float> gSampleData;
// Set range for analog outputs designed for driving LEDs

int gSensorInput0 = 1;
int gSensorInput1 = 3;
int gSensorInput2 = 2;
int gSensorInput3 = 7;
int gSensorInput4 = 5;
int gSensorInput5 = 4;
int gSensorInput6 = 0;
int gSensorInput7 = 6;

float gFrequency = 3.0;
float gPhase;
float gInverseSampleRate;

enum Program {
    programOff,
    programDelay,
};

int gAudioFramesPerAnalogFrame = 0;

TempoSeeker tempoSeeker;
Program pgm;
AnalogIns ins;
Delay delay;

static void setProgram()
{
    pgm = ins.input_5 > 0.2 ? programDelay : programOff;
}

static void abortIfLoud(float y)
{
    if (fabs(y) > 2) {
        printf("Too loud! out: %f\n", y);
        abort();
    }
}

static void setTempoSeekerPeriod(int period)
{
    tempoSeeker.period = period;
}

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

bool setup(BelaContext *context, void *userData)
{
    // Check if analog channels are enabled
    Bela_setADCLevel(-6.0); //This used to be -6. So changing to the old value to deal with guitar output level.

    if(context->analogFrames == 0 || context->analogFrames > context->audioFrames) {
        rt_printf("Error: this example needs analog enabled, with 4 or 8 channels\n");
        return false;
    }

    gInverseSampleRate = 1.0 / context->analogSampleRate;
    gPhase = 0.0;

    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    delay.setPeriodCallback = setTempoSeekerPeriod;

    gNumFramesInFile = AudioFileUtilities::getNumFrames(gFilename);
    gSampleData = AudioFileUtilities::load(gFilename, gNumFramesInFile, 0)[0];

    return true;
}

void render(BelaContext *context, void *userData)
{
    for (unsigned int n = 0; n < context->audioFrames; n++) {
        if (gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            ins.input_0 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0);
            ins.input_1 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1);
            ins.input_2 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2);
            ins.input_3 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3);
            ins.input_4 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4);
            ins.input_5 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5);
            ins.input_6 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6);
            ins.input_7 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7);
            //printf("%f, %f, %f, %f, %f, %f, %f, %f\n", ins.input_0, ins.input_1, ins.input_2, ins.input_3, ins.input_4, ins.input_5, ins.input_6, ins.input_7);
            tempoSeeker.setAnalogIns(ins);
            delay.setAnalogIns(ins);
        }

        setProgram();

        tempoSeeker.process();
        float ledOut = tempoSeeker.tempoLed;

        if (tempoSeeker.risingEdge)
            delay.setTempo(tempoSeeker.period);

        float out = audioRead(context, n, 1);
        // float out = readAudioTestInput();
        float audioOut = delay.process(out);

        switch (pgm) {
            case programOff:
                if (ins.input_4 < 0.4) {
                    ledOut = 0.0f;
                    audioOut = 0.0f;
                }
                break;

            case programDelay:
                break;
        }

        analogWriteOnce(context, n, 0, ledOut);
        audioWrite(context, n, 1, audioOut);
    }
}

void cleanup(BelaContext *context, void *userData)
{

}
