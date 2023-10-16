/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

*/

#include <Bela.h>
#include <cmath>
#include <libraries/AudioFile/AudioFile.h>

#include "Delay.hpp"
#include "Resampler.hpp"
#include "../main_project/mainCommon.hpp"

#define NUM_CHANNELS 1    // NUMBER OF CHANNELS IN THE FILE
#define MOD(n,M) (((n % M) + M) % M)

std::string gFilename = "../low-fi-machine/jazz_lick.wav";
//std::string gFilename = "../main_project/snare_weight_off.wav";

int gNumFramesInFile;

// INPUT MAPPING!!! Wierd because I destroyed the Bela

// //Elias
// int gSensorInput0 = 0;
// int gSensorInput1 = 1;
// int gSensorInput2 = 2;
// int gSensorInput3 = 3;
// int gSensorInput4 = 4;
// int gSensorInput5 = 5;
// int gSensorInput6 = 6;
// int gSensorInput7 = 7;


//Marcus
int gSensorInput0 = 5;
int gSensorInput1 = 6;
int gSensorInput2 = 1;
int gSensorInput3 = 4;
int gSensorInput4 = 2;
int gSensorInput5 = 3;
int gSensorInput6 = 7;
int gSensorInput7 = 0;

int gAudioFramesPerAnalogFrame = 0;
int counter = 0;

float gInverseSampleRate;
float amplitude = 1.0f;

std::vector<float> gSampleData;

AuxiliaryTask gResampleTask;
bool ping = false;

float power = 0.0f;
float offset = 0.0f;
float ringbuf[10] = {};
int writeInd = 0;
Delay delay;
Resampler resampler;

static float tukeyWindowFunction(int index, int N)
{
    float alpha = 0.1f;
    int n;

    if (index > N / 2)
        n = N - index;
    else
        n = index;

    if (n >= 0 && n < (alpha * N / 2))
        return 0.5f * (1.0f - cosf(2.0f * M_PI * index / (alpha * N)));
    else if (n >= (alpha * N / 2) && n <= N / 2)
        return 1.0f;
}

static float readAudioTestInput()
{
    // Increment read pointer and reset to 0 when end of file is reached
    static int readCounter;
    float out = tukeyWindowFunction(readCounter, gNumFramesInFile) * gSampleData[readCounter];
    readCounter++;
    if (readCounter >= gNumFramesInFile)
        readCounter = 0;

    return out;
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
    Bela_setADCLevel(-6.0); //This used to be -6. So changing to the old value to deal with guitar output level.

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
            //printf("%f, %f, %f, %f\n", ins.input_0, ins.input_1, ins.input_2, ins.input_3);

            delay.setAnalogIns(ins);
            // amplitude = map(ins.input_0, 0, 1, 0, 1);
            // power = map(ins.input_1, 0, 1, 1, 20);
            // offset = map(ins.input_2, 0, 1, 0, 10);
            //printf("%f\n", power);
        }


        float out = readAudioTestInput();
        //float out = audioRead(context, n, 1);
        float dOut = delay.process(out);
        // ringbuf[writeInd] = out;

        // out = out + ringbuf[MOD(writeInd - (int)offset, 10)];
        // if (writeInd++ > 10)
        //     writeInd = 0;
        // else if (out < 0)
        //     out = sin(-out);
        // else
        //     out = 0;


        float yOut = dOut;

        // counter++;
        // if (counter >= RESAMPLER_FFTSIZE) {
        //     //resampler.process(resampInBuf, resampOutBuf);
        //     Bela_scheduleAuxiliaryTask(gResampleTask);
        //     counter = 0;
        // }


        abortIfLoud(yOut);

        audioWrite(context, n, 0, yOut);
        audioWrite(context, n, 1, yOut);
    }
}


void cleanup(BelaContext *context, void *userData)
{
}
