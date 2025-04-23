#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include "ADSR.h"
#include <cmath>
#include <vector>
#include <memory>

class ADSR;

class Sample
{
public:
    Sample();
    ~Sample();

    void process(float out[2]);
    void setPlaying(bool playing);
    void init(int fs);


private:
    int numFrames_ = 0;
    int readCounter_ = 0;
    std::vector<float> dataL_;
    std::vector<float> dataR_;
    ADSR * envGen_ = nullptr;
    int state_ = 0;
};


#endif