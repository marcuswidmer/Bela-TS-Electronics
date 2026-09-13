#pragma once

#include <Bela.h>

class WiPepper {
public:
    float volume = 0.0f;

    bool setup(BelaContext* context);
    void processBlock(BelaContext* context);
    void process(const float* in, float* out);

private:
    int audioFramesPerAnalogFrame = 0;
    float ledPeakThresholds[10] = {};
    float inputPeak = 0.0f;
};
