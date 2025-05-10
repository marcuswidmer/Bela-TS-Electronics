#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include "ADSR.h"
#include <cmath>
#include <vector>
#include <memory>
#include <map>

class ADSR;

class Sample
{
public:
    Sample();
    ~Sample();

    void process(float out[2]);
    void setPlaying(bool playing);
    void init(int fs);
    void setNote(int note);



private:
    int getNumFrames();

    int readCounter_ = 0;
    std::map<int, std::vector<float>> dataL_;
    std::map<int, std::vector<float>> dataR_;
    std::map<int, int> numFrames_;
    ADSR * envGen_ = nullptr;
    int state_ = 0;
    int note_ = 0;
    int maxNote_ = 0;
    int minNote_ = 1000;
};


#endif