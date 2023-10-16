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
#include <alsa/asoundlib.h>

#include "GranularReverb.hpp"
#include "KarplusResonator.hpp"
#include "LoFiMachine.hpp"


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
#define NUM_NOTES 36
float freqs[NUM_NOTES] = {n_A, n_As, n_B, n_C, n_Cs, n_D, n_Ds, n_E, n_F, n_Fs, n_G, n_Gs, n_A_1, n_As_1, n_B_1, n_C_1, n_Cs_1, n_D_1, n_Ds_1, n_E_1, n_F_1, n_Fs_1, n_G_1, n_Gs_1, n_A_2, n_As_2, n_B_2, n_C_2, n_Cs_2, n_D_2, n_Ds_2, n_E_2, n_F_2, n_Fs_2, n_G_2, n_Gs_2}; // 36
//float freqs[NUM_NOTES] = {n_A, n_C, n_D, n_E, n_G, n_A_1, n_C_1, n_D_1, n_E_1, n_G_1, n_A_2, n_C_2, n_D_2, n_E_2, n_G_2}; // 15
//float freqs[NUM_NOTES] = {n_A, n_A_1, n_A_2}; // 3

int note = 0;
float freq = 440.0f;
float oldFreq = 440.0f;
float alpha = 0.001;

float vol = 0.3f;
float oldVol = vol;
float volAlpha = 0.01f;

bool onn = false;
std::string gFilename = "../low-fi-machine/jazz_lick.wav";
int gNumFramesInFile;
std::vector<float> gSampleData;

enum Program {
    pgmGranularReverb,
    pgmKarplusResonator,
    pgmLofiMachine,
    ProgramFour,
};

int gAudioFramesPerAnalogFrame = 0;

Program pgm;
GranularReverb granularReverb;
KarplusResonator karplusResonator;
LoFiMachine lofiMachine;

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
        pgm = ProgramFour;
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
                .input_1 = 1.0f, // analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1),
                .input_2 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2),
                .input_3 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3),
                .input_4 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4),
                .input_5 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5),
                .input_6 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6),
                .input_7 = 0.0f // analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7)
            };
            //printf("%f, %f, %f, %f, %f, %f, %f, %f\n", ins.input_0, ins.input_1, ins.input_2, ins.input_3, ins.input_4, ins.input_5, ins.input_6, ins.input_7);
            setProgram(ins);

            note = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1), 0, 1, 0, NUM_NOTES-1);
            onn = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7) > 0.4f;
            //printf("%f, ", freqs[note]);
            karplusResonator.setAnalogIns(ins);
            granularReverb.setAnalogIns(ins);
            lofiMachine.setAnalogIns(ins);
        }

        //float in = readAudioTestInput();
        freq = alpha * freqs[note] + (1 - alpha) * oldFreq;
        oldFreq = freq;

        vol = volAlpha * (float)onn + (1 - volAlpha) * oldVol;
        oldVol = vol;
        //printf("%f, %f\n", vol, (float)onn);
        
        phase += 2.0f * M_PI * freq * gInverseSampleRate;
        if (phase > (2*M_PI))
            phase -= 2*M_PI;

        //printf("%f\n", freq);
        float in = 0.2* vol * sin(phase);
        //float in = audioRead(context, n, 0);
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
