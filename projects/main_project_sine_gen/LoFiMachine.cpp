#include "LoFiMachine.hpp"
#include "../main_project/mainCommon.hpp"

#include <stdio.h>

static void incrGlobCounter(int & globCounter)
{
    globCounter++;
    if (globCounter >= 10000)
        globCounter = 0;
}

LoFiMachine::LoFiMachine()
{
    auto tapeStopFilename = "../low-fi-machine/tape_lever.wav";
    auto tapeStartFilename = "../low-fi-machine/tape_start.wav";
    auto hissFilename = "../low-fi-machine/tape_hiss.wav";

    numFramesInTapeStopFile = AudioFileUtilities::getNumFrames(tapeStopFilename);
    gTapeStopData = AudioFileUtilities::load(tapeStopFilename, numFramesInTapeStopFile, 0)[0];

    numFramesInTapeStartFile = AudioFileUtilities::getNumFrames(tapeStartFilename);
    gTapeStartData = AudioFileUtilities::load(tapeStartFilename, numFramesInTapeStartFile, 0)[0];

    numFramesInHissFile = AudioFileUtilities::getNumFrames(hissFilename);
    gHissData = AudioFileUtilities::load(hissFilename, numFramesInHissFile, 0)[0];

    dipVolume = 1.0f;
    riseVolume = 1.0f;
    vibSpeed = 1.0f;
    gPhase = 0.0f;

    lpCutoff = 3000.0f;
    hpCutoff = 600.0f;

    for (int i = 0; i < LO_FI_MACHINE_DIP_LENGTH; i++) {
        float x_dip = (float)i / LO_FI_MACHINE_DIP_LENGTH * 0.5 * M_PI;
        dipValues[i] = LO_FI_MACHINE_DIP_LENGTH * 1.0f * (1.0f - pow(cosf(x_dip),1)) / 2.0f;
    }

    for (int i = 0; i < LO_FI_MACHINE_RISE_LENGTH; i++) {
        float x = (float)i / LO_FI_MACHINE_RISE_LENGTH * 3.0f;
        riseValues[i] = LO_FI_MACHINE_RISE_LENGTH / 4.0f * x * exp(-pow(x, 1.5));
    }

    riseValues[LO_FI_MACHINE_RISE_LENGTH - 2] *= 0.5f;
    riseValues[LO_FI_MACHINE_RISE_LENGTH - 1] = 0.0f;

    for (int i = 0; i < LO_FI_MACHINE_N_VOLUMES; i++) {
        dipVolumes[i] = 1.0f - (i+1)/((float)LO_FI_MACHINE_N_VOLUMES);
        riseVolumes[i] = i/((float)LO_FI_MACHINE_N_VOLUMES);
    }
}

void LoFiMachine::setSampleRateAndFilterSettings(const float sampleRate)
{
    fs = sampleRate;

    lpSettings = {
        .fs = fs,
        .type = Biquad::lowpass,
        .cutoff = lpCutoff,  // Cut-off frequency for low-pass filter (Hz)
        .q = 1.707,
        .peakGainDb = 0};

    hpSettings = {
        .fs = fs,
        .type = Biquad::highpass,
        .cutoff = hpCutoff, // Cut-off frequency for high-pass filter (Hz)
        .q = 1.707,
        .peakGainDb = 0};

    // Set low pass filter parameters (type, frequency & Q)
    lpFilter.setup(lpSettings);
    // Set high pass filter parameters (type, frequency & Q)
    hpFilter.setup(hpSettings);
}

void LoFiMachine::process(float in, float out[2])
{
    incrGlobCounter(globCounter);

    if (globCounter % 10000 == 0) {
        lpSettings.cutoff = lpCutoff - 200 * wear;
        hpSettings.cutoff = hpCutoff + 200 * wear;
        wearComp = 1 + wear / 5;
        lpFilter.setup(lpSettings);
        hpFilter.setup(hpSettings);
    }

    // Add tape hiss
    in = in + tapeHiss(wear / 5);

    // Saturation
    in =  (1 + 0.1 * saturation) * in / (1 + saturation * fabs(in));

    // Process input signal with high pass filter
    in = hpFilter.process(in);
    // Process signal with low pass filter
    in = lpFilter.process(in);

    delayLine[delayWriteCnt] = in;
    float modulatorOsc = vibDepth * (1 + sinf(gPhase)) / 2;

    tapeStartStop();

    int k0 = modulatorOsc + dipper;
    float frac = (modulatorOsc + dipper) - k0;

    float a = delayLine[(((delayWriteCnt - k0) % LO_FI_MACHINE_MAX_DELAY) + LO_FI_MACHINE_MAX_DELAY) % LO_FI_MACHINE_MAX_DELAY];
    float b = delayLine[(((delayWriteCnt - k0 - 1) % LO_FI_MACHINE_MAX_DELAY) + LO_FI_MACHINE_MAX_DELAY) % LO_FI_MACHINE_MAX_DELAY];

    float y = ((1 - frac) * a + frac * b) + blend * in;

    gPhase += 2.0f * (float)M_PI * vibSpeed * 1.0f / fs;
    if(gPhase > M_PI)
        gPhase -= 2.0f * (float)M_PI;

    delayWriteCnt++;
    if (delayWriteCnt >= LO_FI_MACHINE_MAX_DELAY)
        delayWriteCnt = 0;

    float yOut = wearComp * amplitude * (dipVolume * y + stopSampleOut + startSampleOut);

    out[0] = yOut;
    out[1] = yOut;
}

void LoFiMachine::tapeStartStop()
{
    if (doTapeStop) {
        if (!dipStarted) {
            dipCounter = 0;
        }
        dipper = dipValues[dipCounter];
        dipStarted = true;

        if (dipCounter < LO_FI_MACHINE_DIP_LENGTH - 1) {
            dipCounter++;
            if (dipCounter > LO_FI_MACHINE_DIP_LENGTH - LO_FI_MACHINE_N_VOLUMES) {
                dipVolume = dipVolumes[dipCounter - LO_FI_MACHINE_DIP_LENGTH + LO_FI_MACHINE_N_VOLUMES];
            }
        } else {
            if (!stopSampleStarted) {
                stopSampleCounter = 0;
            }
            stopSampleStarted = true;

            if (stopSampleCounter < numFramesInTapeStopFile) {
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
        if (startSampleCounter < numFramesInTapeStartFile) {
            startSampleOut = gTapeStartData[startSampleCounter];
            startSampleCounter++;
        } else {
            startSampleOut = 0.0f;
            if (!riseStarted)
                riseCounter = 0;

            dipper = riseValues[riseCounter];
            riseStarted = true;

            if (riseCounter < LO_FI_MACHINE_RISE_LENGTH - 1) {
                riseCounter++;
                if (riseCounter < LO_FI_MACHINE_N_VOLUMES) {
                    dipVolume = riseVolumes[riseCounter];
                }
            }
        }

        dipStarted = false;
        stopSampleStarted = false;
    }
}

void LoFiMachine::setAnalogIns(AnalogIns ins)
{
    amplitude = map(ins.input_1, 0, 1, 0, 1);
    wear = map(ins.input_2, 0, 1, 0, 10);
    vibDepth = map(ins.input_3, 0, 1, 0, 200);
    vibSpeed = map(ins.input_4, 0, 1, 0, 10);
    blend = map(ins.input_5, 0, 1, 0, 1.3);
    saturation = map(ins.input_6, 0, 1, 0, 20);
    doTapeStop = ins.input_7 > 0.42;
}

float LoFiMachine::tapeHiss(float amp)
{
    float hiss = amp * gHissData[hissCounter];
    hissCounter++;
    if (hissCounter >= numFramesInHissFile) {
        hissCounter = 0;
    }

    return hiss;
}
