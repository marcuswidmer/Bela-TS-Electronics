#pragma once
#include <algorithm>
#include <cmath>

// Transposed direct-form II. Coefficients are only changed from control code.
class AudioBiquad {
public:
    void clear() { z1_ = z2_ = 0; }
    float process(float x) {
        const float y = b0_ * x + z1_;
        z1_ = b1_ * x - a1_ * y + z2_;
        z2_ = b2_ * x - a2_ * y;
        return y;
    }
    void bandpass(float rate, float frequency, float q) {
        const float w = 6.28318530718f * std::min(frequency, rate * .4f) / rate;
        const float alpha = std::sin(w) / (2 * q);
        coefficients(alpha, 0, -alpha, 1 + alpha, -2 * std::cos(w), 1 - alpha);
    }
    void peak(float rate, float frequency, float q, float db) {
        const float w = 6.28318530718f * std::min(frequency, rate * .4f) / rate;
        const float alpha = std::sin(w) / (2 * q), a = std::pow(10.0f, db / 40);
        coefficients(1 + alpha * a, -2 * std::cos(w), 1 - alpha * a,
                     1 + alpha / a, -2 * std::cos(w), 1 - alpha / a);
    }
private:
    void coefficients(float b0, float b1, float b2, float a0, float a1, float a2) {
        b0_ = b0/a0; b1_ = b1/a0; b2_ = b2/a0;
        a1_ = a1/a0; a2_ = a2/a0;
    }
    float b0_ = 1, b1_ = 0, b2_ = 0, a1_ = 0, a2_ = 0, z1_ = 0, z2_ = 0;
};
