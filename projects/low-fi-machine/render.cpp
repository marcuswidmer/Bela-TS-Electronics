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

std::string gFilename = "jazz_lick.wav";
std::string gTapeStopFilename = "tape_lever.wav";
std::string gTapeStartFilename = "tape_start.wav";
std::string gHissFilename = "tape_hiss.wav";
int gNumFramesInFile;
int gNumFramesInTapeStopFile;
int gNumFramesInTapeStartFile;
int gNumFramesInHissFile;

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
float hissAmplitude = 0.0f;
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

bool doTapeStop = false;
bool dipStarted = false;
int dipCounter = 0;
float dipper = 0;
float dipVolume = 1.0f;
const int nDipVolumes = 100;
const int dipLength = 0.6 * 44100;
const int riseLength = 2 * 44100;
float dipVolumes[nDipVolumes];
float dipValues[dipLength];
int dipLimit = 40000;

bool riseStarted = false;
int riseCounter = 0;
float riseVolume = 1.0f;
float riseVolumes[nDipVolumes];
float riseValues[riseLength];

int stopSampleCounter = 0;
float stopSampleOut = 0.0f;
bool stopSampleStarted = false;

int startSampleCounter = 0;
float startSampleOut = 0.0f;
bool startSampleStarted = false;

std::vector<float> gSampleData;
std::vector<float> gHissData;
std::vector<float> gTapeStopData;
std::vector<float> gTapeStartData;

static void tapeStartStop(bool doTapeStop)
{
    if (doTapeStop) {
        if (!dipStarted) {
            dipCounter = 0;
        }
        dipper = dipValues[dipCounter];
        dipStarted = true;

        if (dipCounter < dipLength - 1) {
            dipCounter++;
            if (dipCounter > dipLength - nDipVolumes) {
                dipVolume = dipVolumes[dipCounter - dipLength + nDipVolumes];
            }
        } else {
            if (!stopSampleStarted) {
                stopSampleCounter = 0;
            }
            stopSampleStarted = true;

            if (stopSampleCounter < gNumFramesInTapeStopFile) {
                stopSampleOut = gTapeStopData[stopSampleCounter];
                stopSampleCounter++;
            } else {
                stopSampleOut = 0.0f;
            }
        }
        riseStarted = false;
        startSampleStarted = false;
    } else {
        if (!startSampleStarted) {
            startSampleCounter = 0;
        }

        startSampleStarted = true;
        if (startSampleCounter < gNumFramesInTapeStartFile) {
            startSampleOut = gTapeStartData[startSampleCounter];
            startSampleCounter++;
        } else {
            startSampleOut = 0.0f;
            if (!riseStarted)
                riseCounter = 0;

            dipper = riseValues[riseCounter];
            riseStarted = true;

            if (riseCounter < riseLength - 1) {
                riseCounter++;
                if (riseCounter < nDipVolumes) {
                    dipVolume = riseVolumes[riseCounter];
                }
            }
        }

        dipStarted = false;
        stopSampleStarted = false;
    }
}

static float readAudioTestInput()
{
    // Increment read pointer and reset to 0 when end of file is reached
    static int readCounter;
    float out = gSampleData[readCounter];

    if (readCounter >= gNumFramesInFile)
        readCounter = 0;

    return out;
}

static float tapeHiss(float amplitude)
{
    static int hissCounter;
    float hiss = amplitude * gHissData[hissCounter];
    hissCounter++;
    if (hissCounter >= gNumFramesInHissFile) {
        hissCounter = 0;
    }

    return hiss;
}

bool setup(BelaContext *context, void *userData)
{
    Bela_setADCLevel(-6.0); //This used to be -6. So changing to the old value to deal with guitar output level.

    printf("Fs: %f. Buffersize: %d\n", context->audioSampleRate, context->audioFrames);

    for (int i = 0; i < dipLength; i++) {
        float x_dip = (float)i / dipLength * 0.5 * M_PI;
        dipValues[i] = dipLength * 1.0f * (1.0f - pow(cosf(x_dip),1)) / 2.0f;
    }

    for (int i = 0; i < riseLength; i++) {
        float x = (float)i / riseLength * 3.0f;
        riseValues[i] = riseLength / 4.0f * x * exp(-pow(x, 1.5));
    }
    riseValues[riseLength - 2] *= 0.5f;
    riseValues[riseLength - 1] = 0.0f;

    for (int i = 0; i < nDipVolumes; i++) {
        dipVolumes[i] = 1.0f - (i+1)/((float)nDipVolumes);
        riseVolumes[i] = i/((float)nDipVolumes);
    }

    gInverseSampleRate = 1.0 / context->audioSampleRate;

    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;


    gNumFramesInFile = AudioFileUtilities::getNumFrames(gFilename);
    gSampleData = AudioFileUtilities::load(gFilename, gNumFramesInFile, 0)[0];

    gNumFramesInTapeStopFile = AudioFileUtilities::getNumFrames(gTapeStopFilename);
    gTapeStopData = AudioFileUtilities::load(gTapeStopFilename, gNumFramesInTapeStopFile, 0)[0];

    gNumFramesInTapeStartFile = AudioFileUtilities::getNumFrames(gTapeStartFilename);
    gTapeStartData = AudioFileUtilities::load(gTapeStartFilename, gNumFramesInTapeStartFile, 0)[0];

    gNumFramesInHissFile = AudioFileUtilities::getNumFrames(gHissFilename);
    gHissData = AudioFileUtilities::load(gHissFilename, gNumFramesInHissFile, 0)[0];

    BiquadCoeff::Settings lp_s = {
        .fs = context->audioSampleRate,
        .type = Biquad::lowpass,
        .cutoff = 3000.0,  // Cut-off frequency for low-pass filter (Hz)
        .q = gFilterQ,
        .peakGainDb = 0};

    BiquadCoeff::Settings hp_s = {
        .fs = context->audioSampleRate,
        .type = Biquad::highpass,
        .cutoff = 600.0, // Cut-off frequency for high-pass filter (Hz)
        .q = gFilterQ,
        .peakGainDb = 0};

    // Set low pass filter parameters (type, frequency & Q)
    lpFilter.setup(lp_s);
    // Set high pass filter parameters (type, frequency & Q)
    hpFilter.setup(hp_s);

    return true;
}

void render(BelaContext *context, void *userData)
{
    for(unsigned int n = 0; n < context->audioFrames; n++) {
        if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            amplitude = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0), 0, 1, 0, 1);
            hissAmplitude = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1), 0, 1, 0, 10);
            vibDepth = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2), 0, 1, 0, 200);
            vibSpeed = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3), 0, 1, 0, 20);
            blend = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4), 0, 1, 0, 1.3);
            saturation = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5), 0, 1, 0, 20);
            doTapeStop = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6), 0, 1, 0, 1) < 0.4;
        }

        //float out = readAudioTestInput();
        float out = audioRead(context, n, 0);

        // Add tape hiss
        out = out + tapeHiss(hissAmplitude);

        // Saturation
        out =  out / (1 + saturation * fabs(out));

        // Process input signal with high pass filter
        out = hpFilter.process(out);
        // Process signal with low pass filter
        out = lpFilter.process(out);


        delayLine[delay_write_cnt] = out;
        float modulator_osc = vibDepth * (1 + sinf(gPhase)) / 2;

        tapeStartStop(doTapeStop);

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

        float yOut = amplitude * (dipVolume * y + stopSampleOut + startSampleOut);
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
