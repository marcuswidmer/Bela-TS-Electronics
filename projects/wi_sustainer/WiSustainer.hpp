#ifndef WI_SUSTAINER_HPP
#define WI_SUSTAINER_HPP
#define WI_SUSTAINER_NUM_VOICES 6
#define WI_SUSTAINER_MAX_NUM_SAMPLES_PER_WAVE 44100 // one period of a 20 Hz sinewave at fs = 44100 Hz

#include "ADSR.h"
#include <map>

struct Osc {
    bool active;
    float waveData[WI_SUSTAINER_MAX_NUM_SAMPLES_PER_WAVE] = {};
    int noteOffset;
    bool left; // false = right, nullopt = mono
    bool isStereo;
    unsigned int waveLength = 0;
};

struct Voice {
    Osc osc;
    int note_;
    ADSR * envGen_;
};

enum class WavePhase {
    Measure,
    Record,
    Play,
};

class InstantaneousRms {
public:
    void reset() {
        idx_ = 0;
        count_ = 0;
        sumsq_ = 0.0;
        buf_.fill(0.0f);
    }

    void push(float x) {
        if (count_ < kWindow) {
            // Still filling the window.
            ++count_;
        } else {
            // Remove the oldest sample's contribution.
            const double old = static_cast<double>(buf_[idx_]);
            sumsq_ -= old * old;
        }

        buf_[idx_] = x;
        sumsq_ += static_cast<double>(x) * x;

        idx_ = (idx_ + 1) % kWindow;
    }

    float rms() const {
        if (count_ < kWindow) return 0.0f;
        return static_cast<float>(std::sqrt(sumsq_ / static_cast<double>(kWindow)));
    }

private:
    static constexpr std::size_t kWindow = 200;
    std::array<float, kWindow> buf_{};
    std::size_t idx_ = 0;
    std::size_t count_ = 0;
    double sumsq_ = 0.0;
};

class WiSustainer
{
public:
    WiSustainer(int fs);

    void process(float in, float out[2], float * rms);
    void init();
    void playNewVoice(int note, int velocity);
    void releaseVoice(int note);

private:
    void measureWave(float in);
    void recordWave(float in);
    void playWave(float in, float out[2]);
    void donePlaying();

    unsigned int runner_ = 0;
    const int fs_;
    float amplitude_ = 0.05f;
    Voice voices_[WI_SUSTAINER_NUM_VOICES] = {};
    int currentVoiceIdx_ = 0;
    std::multimap<int, int> activeVoices_;
    InstantaneousRms rms_;
    float trigThresh_ = 0.01f;
    float prevIn_ = 0.0f;
    bool withinZeroCrossings_ = false;
    bool recordingStarted_ = false;
    unsigned int recordIdx_ = 0;
    unsigned int playIdx_ = 0;
    WavePhase wavePhase_ = WavePhase::Measure;
};

#endif