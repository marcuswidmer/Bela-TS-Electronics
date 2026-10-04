#pragma once
#include <array>
#include <cstdint>

// Original JUNO-inspired six-voice subtractive synth. No Bela dependency,
// dynamic allocation, device access or logging in the audio/MIDI paths.
class JunoSynth {
public:
    static constexpr unsigned int voiceCount = 6;
    bool setup(float sampleRate);
    void setPots(const std::array<float, 8>& pots);
    void pressButton(unsigned int button);
    void noteOn(unsigned int note, unsigned int velocity, unsigned int channel = 0);
    void noteOff(unsigned int note, unsigned int channel = 0);
    void controlChange(unsigned int controller, unsigned int value, unsigned int channel = 0);
    void pitchBend(unsigned int value, unsigned int channel = 0); // 14-bit, +/-2 semitones.
    void panic();
    void process(float& left, float& right);
    unsigned int activeVoices() const;
    unsigned int waveform() const { return waveform_; } // saw+pulse, saw, pulse
    unsigned int chorusMode() const { return chorusMode_; } // off, I, II
private:
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    struct Filter {
        float s1 = 0, s2 = 0;
        float process(float input, float g, float k);
    };
    struct Voice {
        Stage stage = Idle;
        unsigned int note = 0, channel = 0;
        uint64_t age = 0;
        bool held = false;
        float phase = 0, subPhase = 0, envelope = 0, velocity = 0;
        float increment = 0, g = 0, previous = 0, stealTail = 0;
        unsigned int stealSamples = 0;
        Filter filter1, filter2;
    };
    std::array<Voice, voiceCount> voices_{};
    std::array<float, 128> frequencies_{};
    std::array<float, 16> bend_{}, modulation_{};
    std::array<bool, 16> sustain_{};
    std::array<float, 8> pots_{{.7f, .6f, .2f, .35f, .05f, .35f, .7f, .3f}};
    std::array<float, 4096> chorus_{};
    float sampleRate_ = 44100, smoothing_ = 0;
    float volume_ = 0, cutoff_ = 1000, targetCutoff_ = 1000, resonance_ = 0, envDepth_ = 0;
    float attackStep_ = 1, decayMultiplier_ = 0, releaseMultiplier_ = 0;
    float lfoPhase_ = 0, chorusPhase_ = 0, chorusMix_ = 0;
    float hpInput_ = 0, hpOutput_ = 0, hpPole_ = 0;
    unsigned int chorusWrite_ = 0, controlCounter_ = 0;
    unsigned int waveform_ = 0, chorusMode_ = 1;
    uint64_t age_ = 0;
    uint32_t noise_ = 1;
    void updatePitch(Voice& voice);
    float readChorus(float delay) const;
};
