#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include "Sample.hpp"

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
    void playNewVoice(int note, float velocity);
    void releaseVoice(int note);
    int getMidiNoteOffset() { return midiNoteOffset_; }

private:
    void loadDataSet(std::string path, DataSet & ds, bool stereo = false);
    void invalidateDataSets();

    int currentVoiceIdx_ = 0;
    int dataSet_ = 0;
    int prevDataSet_ = -1;
	float amplitude_ = 0.0f;
    static const int numVoices_ = 6;
    static const int midiNoteOffset_ = 39; //51 is standard
    std::multimap<int, int> activeVoices_;
    Sample * samples_[numVoices_];
    Midi * midi_;
    DataSet ds_;
    DataSet secondDs_;
    const char* midiPort_ = "hw:1,0,0";
};


#endif