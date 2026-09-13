#include "WiPepper.hpp"
#include <algorithm>
#include <cmath>

namespace {
// Pepper LED pins in left-to-right order.
constexpr unsigned int kLedPins[] = {6, 7, 10, 2, 3, 0, 1, 4, 5, 8};
constexpr unsigned int kLedCount = sizeof(kLedPins) / sizeof(kLedPins[0]);
}

bool WiPepper::setup(BelaContext* context)
{
    Bela_setAudioInputGain(0, 0.0f); // Left input
    Bela_setAudioInputGain(1, 0.0f); // Right input
    inputPeak = 0.0f;
    for (unsigned int led = 0; led < kLedCount; ++led) {
        const float thresholdDb = -48.0f + 5.0f * led;
        ledPeakThresholds[led] = std::pow(10.0f, thresholdDb / 20.0f);
        if (context->digitalFrames && kLedPins[led] < context->digitalChannels) {
            pinMode(context, 0, kLedPins[led], OUTPUT);
            digitalWrite(context, 0, kLedPins[led], LOW);
        }
    }

    audioFramesPerAnalogFrame = context->analogFrames
        ? context->audioFrames / context->analogFrames : 0;

    int analogIOSampleRate = (int)context->analogSampleRate;

    printf("Fs: %f. Analog fs: %d Buffersize: %d\n", context->audioSampleRate, analogIOSampleRate, context->audioFrames);

    return true;
}

void WiPepper::processBlock(BelaContext* context)
{
    inputPeak = 0.0f;
    for (unsigned int n = 0; n < context->audioFrames; n++) {

        if (audioFramesPerAnalogFrame && !(n % audioFramesPerAnalogFrame)) {
            volume = analogRead(context, n/audioFramesPerAnalogFrame, 0);
        }
        float in[2] = {}; 
        float out[2] = {};
        
        in[0] = audioRead(context, n, 0);
        in[1] = audioRead(context, n, 1);
        
        process(in, out);
        
        audioWrite(context, n, 0, out[0]);
        audioWrite(context, n, 1, out[1]);
    }

    for (unsigned int frame = 0; frame < context->digitalFrames; ++frame) {
        for (unsigned int led = 0; led < kLedCount; ++led) {
            if (kLedPins[led] < context->digitalChannels) {
                digitalWriteOnce(context, frame, kLedPins[led],
                    inputPeak >= ledPeakThresholds[led] ? HIGH : LOW);
            }
        }
    }
}

void WiPepper::process(const float* in, float* out)
{
    // Track the largest magnitude across both inputs, before volume control.
    inputPeak = std::max(inputPeak, std::max(std::abs(in[0]), std::abs(in[1])));
    const float gain = std::max(0.0f, std::min(volume, 1.0f));
    out[0] = in[0] * gain;
    out[1] = out[0];
}
