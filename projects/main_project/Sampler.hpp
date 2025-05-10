#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include "Sample.hpp"

#include <libraries/Midi/Midi.h>
#include <atomic>

struct AnalogIns;
class MidiChannelMessage;
class Midi;
class Sampler
{
public:
    Sampler();

    void process(float out[2]);
    void init(int fs);
    void setAnalogIns(AnalogIns ins);
    void playNewVoice();
    void releaseVoice();
    int getMidiNoteOffset() { return midiNoteOffset_; }

private:
    int currentVoiceIdx_ = 0;
	float amplitude_ = 0.0f;
    static const int numVoices_ = 4;
    static const int midiNoteOffset_ = 51;
    std::map<int, int> activeVoices_;
    Sample * samples_[numVoices_];
    Midi midi_;
    const char* midiPort_ = "hw:1,0,0";
};


#endif