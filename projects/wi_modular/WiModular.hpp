#ifndef WI_MODULAR_HPP
#define WI_MODULAR_HPP

#include "WiSequencer.hpp"
#include "Sampler.hpp"
#include "WiLFO.hpp"

#include <random>

enum Pgm {
    PgmRandomOctave,
    PgmOctaveSelector,
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

class Midi;

class WiModular
{
public:
    WiModular();

    void process();
    void processAudio(float * out);
    void init(float analogIOSampleRate, int audioSampleRate = 44100);
    void assignNote(int note);
    void tickSequencer();
    int getMidiNoteOffset() { return midiNoteOffset_; }
    void samplerPlayNewVoice(int note, float velocity);
    void samplerReleaseVoice(int note);
    void setMidiClock(bool val);
    void freezeSequenceChanged(bool freezeSequence);
    AnalogIO analogIO = {};

private:
    void processLFO();
    void processLed();
    void processTriggers();
    void setLedCountdown(float length, bool priority = false);
    void setTriggerCountdown(float length, bool sync = false);
    void sendMidiByte(uint8_t byte);

    int ledCountdown_ = 0;
    int ledPriorityCountdown_ = 0;
    int syncTriggerCountdown_ = 0;
    float sampleCntr_ = 0;
    float analogIOSampleRate_ = 0;
    std::random_device rd_;
    std::mt19937 gen_;
    int randomPeriodShift_ = 0;
    bool buttonPressed_ = false;
    const float octaveScalingFactor_ = 6.0f;
    float speedSmoothed_ = 0.0f;
    float octaveSmoothed_ = 0.0f;
    float octFloatSmoothed_ = 0.0f;
    float lfoAmpSmoothed_ = 0.0f;
    int pitchBiases[numPitchBiases] = {0, 7, 12, 19, 24};
    Midi * midi_;
    const char* midiPort_ = "hw:1,0,0";
    static const int midiNoteOffset_ = 39; //51 is standard
    bool freeze_ = false;

    WiSequencer wiSequencer;
    Sampler sampler;
    WiLFO wiLFO;
};


#endif