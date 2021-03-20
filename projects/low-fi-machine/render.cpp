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
#include <SampleLoader.h>
#include <SampleData.h>
#include <libraries/Biquad/Biquad.h>

#define MAX_DELAY 1024
#define NUM_CHANNELS 1    // NUMBER OF CHANNELS IN THE FILE

std::string gFilename = "jazz_lick.wav";
int gNumFramesInFile;

// Two buffers for each channel:
// one of them loads the next chunk of audio while the other one is used for playback
SampleData gSampleBuf[NUM_CHANNELS];

// read pointer relative current buffer (range 0-gNumFramesInFile)
// initialise at gNumFramesInFile to pre-load second buffer (see render())
float gReadPtr = gNumFramesInFile;
// read pointer relative to file, increments by gNumFramesInFile (see fillBuffer())
int gBufferReadPtr = 0;
// keeps track of which buffer is currently active (switches between 0 and 1)
int gActiveBuffer = 0;
// this variable will let us know if the buffer doesn't manage to load in time
int gDoneLoadingBuffer = 1;

// Because I destroyed the Bela
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
float speed = 1.0f;

Biquad lpFilter;    // Biquad low-pass frequency;
Biquad hpFilter;    // Biquad high-pass frequency;

float gHPfreq = 600.0;  // Cut-off frequency for high-pass filter (Hz)
float gLPfreq = 3000.0; // Cut-off frequency for low-pass filter (Hz)

float gFilterQ = 1.707; // Quality factor for the biquad filters to provide a Butterworth response

float delayLine[MAX_DELAY] = {0};

float vibDepth = 1;
float vibSpeed = 1;
float blend = 1;
float saturation = 1;
int delay_write_cnt = 0;

float tapeStop;
bool tapeStopF = false;
bool dipStarted = false;
int dipCounter = 0;
float dipper = 0;
const int dipLength = 70000;
float dipIncrements[dipLength];

bool setup(BelaContext *context, void *userData)
{
    // getNumFrames() and getSamples() are helper functions for getting data from wav files declared in SampleLoader.h
    // SampleData is a struct that contains an array of floats and an int declared in SampleData.h

    for (int i = 0; i < dipLength; i++) {
        dipIncrements[i] = i/((float)dipLength) * 1.0f;
    }

    gNumFramesInFile = getNumFrames(gFilename);

    gInverseSampleRate = 1.0 / context->audioSampleRate;

    if(gNumFramesInFile <= 0)
        return false;

    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    for(int ch=0;ch<NUM_CHANNELS;ch++) {
        gSampleBuf[ch].sampleLen = gNumFramesInFile;
        gSampleBuf[ch].samples = new float[gNumFramesInFile];
        if(getSamples(gFilename,gSampleBuf[ch].samples,ch,0,gNumFramesInFile))
            return false;

    }

    // Set low pass filter parameters (type, frequency & Q)
    lpFilter.setup(gLPfreq, context->audioSampleRate, Biquad::lowpass, gFilterQ, 0);
    // Set high pass filter parameters (type, frequency & Q)
    hpFilter.setup(gHPfreq, context->audioSampleRate, Biquad::highpass, gFilterQ, 0);

    return true;
}

void render(BelaContext *context, void *userData)
{
    for(unsigned int n = 0; n < context->audioFrames; n++) {

        if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            amplitude = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0), 0, 1, 0, 1);
            speed = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1), 0, 1, 0, 2);
            vibDepth = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2), 0, 1, 0, 200);
            vibSpeed = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3), 0, 1, 0, 20);
            blend = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4), 0, 1, 0, 1.3);
            saturation = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5), 0, 1, 0, 50);
            tapeStop = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6), 0, 1, 0, 1);
            tapeStopF = tapeStop > 0.5;
        }

        // Increment read pointer and reset to 0 when end of file is reached
        gReadPtr = gReadPtr + speed;

        if(gReadPtr >= gNumFramesInFile - 1)
            gReadPtr = 0;

        int gReadPtr_int = gReadPtr;
        float out_curr = gSampleBuf[0].samples[gReadPtr_int];
        float out_next = gSampleBuf[0].samples[gReadPtr_int + 1];
        float frac_playback = gReadPtr - gReadPtr_int;
        float out = frac_playback * out_next + (1 - frac_playback) * out_curr;

        // float out = audioRead(context, n, 0);
        out =  out / (1 + saturation * fabs(out));

        // Process input signal with high pass filter
        out = hpFilter.process(out);
        // Process signal with low pass filter
        out = lpFilter.process(out);


        delayLine[delay_write_cnt] = out;
        float modulator_osc = vibDepth * (1 + sinf(gPhase)) / 2;

        if (tapeStopF) {
            if (!dipStarted)
                dipCounter = 0;

            dipper = dipper + dipIncrements[dipCounter];
            dipStarted = true;

            if (dipCounter < dipLength - 1)
                dipCounter++;
        } else {
            dipper = 0;
            dipStarted = false;
        }

        int k_0 = modulator_osc + dipper;
        float frac = (modulator_osc + dipper) - k_0;

        float a = delayLine[(((delay_write_cnt - k_0) % MAX_DELAY) + MAX_DELAY) % MAX_DELAY];
        float b = delayLine[(((delay_write_cnt - k_0 - 1) % MAX_DELAY) + MAX_DELAY) % MAX_DELAY];

        float y = ((1 - frac) * a + frac * b) + blend * out;

        gPhase += 2.0f * (float)M_PI * vibSpeed * gInverseSampleRate;
        if(gPhase > M_PI)
            gPhase -= 2.0f * (float)M_PI;

        delay_write_cnt++;
        if (delay_write_cnt >= MAX_DELAY)
            delay_write_cnt = 0;

        if (abs(amplitude * y) > 1) {
            printf("Too loud!\n");
            abort();
        }

        audioWrite(context, n, 0, amplitude * y);
        audioWrite(context, n, 1, amplitude * y);

    }
}


void cleanup(BelaContext *context, void *userData)
{
    // Delete the allocated buffers
    for(int ch=0;ch<NUM_CHANNELS;ch++) {
        delete[] gSampleBuf[ch].samples;
    }
}
