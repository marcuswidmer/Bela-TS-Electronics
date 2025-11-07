#ifndef WI_SYNTHESIZER_HPP
#define WI_SYNTHESIZER_HPP
#include <optional>
#define WI_SYNTHESIZER_NUM_VOICES 16
#define WI_SYNTHESIZER_NUM_SAMPLES_PER_WAVE 2205 // one period of a 20 Hz sinewave at fs = 44100 Hz
#define WI_SYNTHESIZER_NUM_OSCILLATORS 8

#include "ADSR.h"
#include "StateVariableFilter.hpp"
#include "Vibrato.hpp"
#include "GranularReverb.hpp"

#include <map>

enum class OscType {
    Sine,
    Saw,
    Klokkenspel,
    Human,
    AnalogStrings,
    AnalogStringsAlt,
    HammondGlass,
    PulseDuty50,
    Tri,
};

struct Osc {
    bool active;
    float waveData[WI_SYNTHESIZER_NUM_SAMPLES_PER_WAVE];
    OscType type;
    int noteOffset;
    std::optional<bool> left; // false = right, nullopt = mono

};

struct Voice {
    Osc oscs[WI_SYNTHESIZER_NUM_OSCILLATORS];
    int note_;
    ADSR * envGen_;
};

class WiSynthesizer
{
public:
    WiSynthesizer(int fs);

    void process(float out[2]);
    void playNewVoice(int note, int velocity);
    void releaseVoice(int note);

private:
    unsigned int runner_ = 0;
    const int fs_;
    float amplitude_ = 0.05f;
    float noteOffThreshold_ = 0.001f;
    Voice voices_[WI_SYNTHESIZER_NUM_VOICES] = {};
    int currentVoiceIdx_ = 0;
    std::multimap<int, int> activeVoices_;
    StateVariableFilter svf_;
    Vibrato vib_;
    GranularReverb granular_;
};


#endif