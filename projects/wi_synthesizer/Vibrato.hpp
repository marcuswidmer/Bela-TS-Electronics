#define VIBRATO_MAX_DELAY 44100

#include <cmath>

class Vibrato {
public:
    Vibrato(double sampleRate)
    : fs_(sampleRate)
    {

    }

    double process(double in)
    {
        delayLine_[delayWriteCnt_] = in;
        float modulatorOsc = vibDepth_ * (1 + sinf(vibPhase_)) / 2;
        int k0 = modulatorOsc;
        double frac = modulatorOsc - k0;

        double a = delayLine_[(((delayReadCnt_ - k0) % VIBRATO_MAX_DELAY) + VIBRATO_MAX_DELAY) % VIBRATO_MAX_DELAY];
        double b = delayLine_[(((delayReadCnt_ - k0 - 1) % VIBRATO_MAX_DELAY) + VIBRATO_MAX_DELAY) % VIBRATO_MAX_DELAY];

        double y = ((1 - frac) * a + frac * b);

        vibPhase_ += 2.0f * (float)M_PI * vibSpeed_ * 1.0f / fs_;
        if(vibPhase_ > M_PI)
            vibPhase_ -= 2.0f * (float)M_PI;

        delayWriteCnt_++;
        if (delayWriteCnt_ >= VIBRATO_MAX_DELAY)
            delayWriteCnt_ = 0;

        delayReadCnt_++;
        if (delayReadCnt_ >= VIBRATO_MAX_DELAY)
            delayReadCnt_ = 0;

        return y;
    }

private:
    double fs_;
    double delayLine_[VIBRATO_MAX_DELAY] = {};
    int delayWriteCnt_ = 20 * 44;
    int delayReadCnt_ = 0;
    double vibPhase_ = 0;
    double vibDepth_ = 10;
    double vibSpeed_ = 3;
};
