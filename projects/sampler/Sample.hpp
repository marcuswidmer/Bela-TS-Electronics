#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include "ADSR.h"
#include <cmath>
#include <vector>
#include <memory>
#include <map>
#include "../mainCommon.hpp"

//#define USE_NO_MIDI

class ADSR;

struct DataSet {
    std::map< int, int > numFrames;
    std::map< int, std::vector<float> > dataL;
    std::map< int, std::vector<float> > dataR;
    int minNote;
    int maxNote;
    bool valid;
};

class Sample
{
public:
    Sample(DataSet * ds, DataSet * secondDs);
    ~Sample();

    void process(float out[2]);
    void processDataSet(float out[2], DataSet * ds);
    void setPlaying(bool playing, float velocity = 0.0f);
    void init(int fs);
    void setNote(int note);

private:
    int getNumFrames();

    int readCounter_ = 0;
    ADSR * envGen_ = nullptr;
    int state_ = 0;
    int note_ = 0;
    float velocity_ = 0.0f;
    DataSet * ds_;
    DataSet * secondDs_;
};


#endif