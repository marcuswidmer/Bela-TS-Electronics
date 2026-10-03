#pragma once
#include <array>
#include <cstdint>

// Twelve parallel, damped feedback strings, adapted from karplus_resonator.
// All memory belongs to the instance; no file access or allocation in process.
class KarplusResonator {
public:
    bool setup(float sampleRate);
    void setPots(const std::array<float, 8>& pots);
    void pressButton(unsigned int button); // Zero-based, debounced rising edges.
    void process(float input, float& left, float& right);
    void clear();
private:
    static constexpr unsigned int voices = 12, capacity = 2048;
    struct String {
        std::array<float, capacity> line{};
        unsigned int write = 0;
        float lowpass = 0, delay = 400, targetDelay = 400;
        float feedback = 0, targetFeedback = 0;
    };
    std::array<String, voices> strings;
    std::array<float, 8> controls{{0, 0.5f, 0.5f, 0.5f, 0.5f, 1, 0, 0}};
    float sampleRate = 44100, smoothing = 0;
    float master = 0, excitation = 0, mix = 1, width = 0, damping = 0;
    float targetDamping = 0, pluckEnvelope = 0, pluckDecay = 0;
    unsigned int noteSet = 0;
    bool sustain = false;
    uint32_t noiseState = 1;
    void updateTuning();
};
