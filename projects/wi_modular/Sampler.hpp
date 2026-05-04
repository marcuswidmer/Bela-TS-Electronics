#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include "Voice.hpp"

#include <functional>
#include <random>

struct AnalogIns;
class Sampler
{
public:
    Sampler();

    void process(float out[2]);
    void init(int fs, std::function<void(float)> = nullptr);
    void setAnalogIns(AnalogIns ins);
    void playNewVoice(int note, float velocity);
    void releaseVoice(int note);
    int convertToProgram(float analogIn);
    float convertFromProgram(int program);
    int getProgram() { return program_; };
    void setRelease(float r);
    void setProgram(float program, bool hard = false);
    void setDroneVoiceVelocity(int voice, float velocity);
    void setMainLevel(float lev);
    void setDroneLevel(float lev);
    void deleteMidi();

private:
    void loadDataSet(std::string path, DataSet & ds, bool stereo = false);
    void invalidateDataSets();
    void playNewDroneVoice(int note, int voice = 0);

    int currentVoiceIdx_ = 0;
    int program_ = 0;
    float programSmoothed_ = 0.0f;
    int prevProgram_ = -1;
	float amplitude_ = 0.0f;
    float firstSecondMix_ = 0.0f;
    float droneAmpl_ = 0.0f;
    static const int numRegVoices_ = 6;
    static const int numDroneVoices_ = 1;
    static const int numVoices_ = numRegVoices_ + numDroneVoices_;
    static const int midiNoteOffset_ = 39; //51 is standard
    std::multimap<int, int> activeVoices_;
    Voice * voices_[numVoices_];
    DataSet ds_;
    DataSet secondDs_;
    DataSet droneDs_;
    bool play_ = false;
    int testNote_ = 0;
    std::function<void(float)> ledCb_;
};


#endif