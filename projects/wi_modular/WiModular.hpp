#ifndef WI_MODULAR_HPP
#define WI_MODULAR_HPP

#include "WiSequencer.hpp"
#include "../sampler/Sampler.hpp"

#include <random>

enum Pgm {
    PgmRandomOctave,
    PgmTestBench,
    PgmSequencer,
    PgmSampler,
    PgmUnused,
};

struct AnalogIO {
    bool button;
    bool led; // Not strictly analog, but digital (who cares!)
    float pot0;
    float pot1;
    float pot2;
    Pgm selector;
    float cvOut0;
    float cvOut1;
    float cvOut2;
    float cvOut3;
};

const int numPitchBiases = 5;

class WiModular
{
public:
    WiModular();

    void process();
    void processAudio(float * out);
    void init(float analogIOSampleRate, int audioSampleRate = 44100);
    void assignNote(int note);
    void reinitSequencer(float analogSampleRate);
    void reinitSampler(int audioSampleRate);
    AnalogIO analogIO = {};

private:
    void processLed();
    void triggerLed(float length);

    int ledCountdown_ = 0;
    float sampleCntr_ = 0;
    float analogIOSampleRate_ = 0;
    std::random_device rd_;
    std::mt19937 gen_;
    int randomPeriodShift_ = 0;
    int triggerCntr_ = 0;
    bool buttonPressed_ = false;
    const float octaveScalingFactor_ = 6.0f;
    float speedSmoothed_ = 0.0f;
    float lfoSpeedSmoothed_ = 0.0f;
    int pitchBiases[numPitchBiases] = {0, 7, 12, 19, 24};

    WiSequencer wiSequencer;
    Sampler sampler;
};


#endif