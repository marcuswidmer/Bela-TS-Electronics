#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include "Sample.hpp"

struct AnalogIns;
struct MidiChannelMessage;
class Midi;
class Sampler
{
public:
    Sampler();

    void process(float out[2]);
    void init(int fs);
    void setAnalogIns(AnalogIns ins);

private:
    void playNewSample();
    void releaseCurrentSample();
    void midiMessageCallback(MidiChannelMessage message, void* arg) {

    int currentSampleIdx_ = 0;
	float amplitude_ = 0.0f;
    bool play_ = false;
    static const int numSamples_ = 10;
    Sample * samples_[numSamples_];
    Midi midi_;
    const char* midiPort_ = "hw:1,0,0";
};


#endif