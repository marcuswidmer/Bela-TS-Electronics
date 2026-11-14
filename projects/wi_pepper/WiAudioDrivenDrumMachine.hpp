#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// The retained class name preserves project integration; drums now use a
// programmed groove and a brightness control, independent of audio input.
class WiAudioDrivenDrumMachine {
public:
    static constexpr unsigned int VoiceCount = 7, StepCount = 16, KitCount = 2, PatternCount = 7, SampleCount = 20;
    static constexpr unsigned int RealKitPattern = 6;
    static float bandFrequency(unsigned int voice); // Nominal brightness; also used by the optional test-source EQ.
    static const char* voiceName(unsigned int voice, unsigned int pattern = 0);
    static const char* sampleFilename(unsigned int sample);
    static unsigned int patternSample(unsigned int voice, unsigned int pattern, unsigned int step = 0, unsigned int variation = 0);
    static unsigned int patternStepCount(unsigned int pattern); // StepCount is the maximum bar length.
    static const char* patternName(unsigned int pattern);
    static const char* kitName(unsigned int kit);
    static float patternVelocity(unsigned int voice, unsigned int step, unsigned int pattern = 0);
    bool setup(float sampleRate);
    bool loadSamples(const std::string& projectDirectory); // Lo-fi, Yamaha rim and real-kit samples; setup thread only.
    const std::string& loadError() const { return loadError_; }
    unsigned int loadedSamples() const;
    void setControls(float volume, float filter);
    void process(float input, float& left, float& right);
    void advanceStep();
    void seekStep(unsigned int position);
    void silence();
    void nextPattern() { selectPattern((pattern_ + 1) % PatternCount); }
    bool selectPattern(unsigned int pattern);
    unsigned int currentPattern() const { return pattern_; }
    unsigned int currentKit() const { return pattern_ == RealKitPattern ? 1 : 0; }
    float filterOpening() const { return filterSmoothed_; }
    int currentStep() const { return currentStep_; }
    const std::array<uint64_t, VoiceCount>& triggerCounts() const { return triggers_; }
    const std::array<float, VoiceCount>& intensities() const { return intensities_; }
    const std::array<float, VoiceCount>& lastVelocities() const { return lastVelocities_; }
private:
    struct Sample { std::vector<float> stereo; float rate = 44100; };
    struct Playback { double position = 0; float gain = 0; unsigned int sample = 0; bool active = false; };
    struct Voice { std::array<Playback, 4> playback{}; unsigned int nextPlayback = 0; };
    static bool readWave(const std::string& path, Sample& sample, std::string& error);
    void updateIntensities();
    std::array<Sample, SampleCount> samples_;
    std::array<Voice, VoiceCount> voices_{};
    std::array<float, VoiceCount> intensities_{}, lastVelocities_{};
    std::array<uint64_t, VoiceCount> triggers_{};
    std::string loadError_;
    float rate_ = 0, smoothing_ = 0, volume_ = .5f, gain_ = 0;
    float filterTarget_ = 1, filterSmoothed_ = 1;
    bool controlsInitialized_ = false;
    unsigned int nextStep_ = 0, pattern_ = 0, nextVariation_ = 0;
    int currentStep_ = -1;
};
