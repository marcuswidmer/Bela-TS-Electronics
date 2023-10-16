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
#include <libraries/AudioFile/AudioFile.h>
#include <libraries/Biquad/Biquad.h>

#define MAX_DELAY (10 * 44100)
#define NUM_CHANNELS 1    // NUMBER OF CHANNELS IN THE FILE

std::string gFilename = "../low-fi-machine/jazz_lick.wav";

int gNumFramesInFile;

// INPUT MAPPING!!! Wierd because I destroyed the Bela
int gSensorInput0 = 5;
int gSensorInput1 = 6;
int gSensorInput2 = 1;
int gSensorInput3 = 4;
int gSensorInput4 = 2;
int gSensorInput5 = 3;
int gSensorInput6 = 7;
int gSensorInput7 = 0;

int gAudioFramesPerAnalogFrame = 0;
float gPhase = 0;
float gInverseSampleRate;

float amplitude = 0.0f;
float wah = 0.0f;
float phase = 0.0f;
float curveSaturation = 0;
int globCounter = 0;

Biquad bpFilter;    // Biquad band-pass frequency;

std::vector<float> gSampleData;

BiquadCoeff::Settings bp_s;

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

static void incrGlobCounter()
{
    globCounter++;
    if (globCounter >= 10000)
        globCounter = 0;
}

bool setup(BelaContext *context, void *userData)
{
    Bela_setADCLevel(-6.0); //This used to be -6. So changing to the old value to deal with guitar output level.

    printf("Fs: %f. Buffersize: %d\n", context->audioSampleRate, context->audioFrames);

    gInverseSampleRate = 1.0 / context->audioSampleRate;

    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    gNumFramesInFile = AudioFileUtilities::getNumFrames(gFilename);
    gSampleData = AudioFileUtilities::load(gFilename, gNumFramesInFile, 0)[0];

    bp_s = {
        .fs = context->audioSampleRate,
        .type = Biquad::bandpass,
        .cutoff = 1000.0,  // Cut-off frequency for low-pass filter (Hz)
        .q = 1.0,
        .peakGainDb = 0};

    // Set band pass filter parameters (type, frequency & Q)
    bpFilter.setup(bp_s);

    return true;
}

void render(BelaContext *context, void *userData)
{
    for(unsigned int n = 0; n < context->audioFrames; n++) {
        if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            amplitude = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0), 0, 1, 0, 1);
            wah = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1), 0, 1, 0, 0.02);
            curveSaturation = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2), 0, 1, 1, 100);
        }

        incrGlobCounter();

        phase = phase + wah;
        if (phase > 10000 * M_PI) {
            printf("Resetting phase\n");
            phase = 0.0f;
        }
        //printf("Wah: %f. Phase: %f\n", wah, phase);

        float curve = sinf(phase);

        // Saturation of wah envelope
        curve = 1.01 * curveSaturation * curve / (1 + curveSaturation * fabs(curve));

        float theWah = (curve + 1.0f) / 2.0f;
        if (theWah < 0.0f)
             abort();

        if (globCounter % 2 == 0) {
            bp_s.q = pow(2.0,  (2.0 * (1.0 - theWah) + 1.0));
            bp_s.cutoff = 450.0 *  pow(2.0, 2.3 * theWah);
            bp_s.peakGainDb = 20 * log10(0.1 * pow(4.0, theWah));

            bpFilter.setup(bp_s);
        }

        // float out = readAudioTestInput();
        float out = audioRead(context, n, 0);

        // Process input signal with band pass filter
        out = bpFilter.process(out);

        float yOut = amplitude * out ;
        if (fabs(yOut) > 3) {
            printf("Too loud! out: %f\n", yOut);
            abort();
        }

        audioWrite(context, n, 0, yOut);
        audioWrite(context, n, 1, yOut);

    }
}


void cleanup(BelaContext *context, void *userData)
{
}
