#include "Sample.hpp"
#include "../main_project/mainCommon.hpp"

#include <libraries/AudioFile/AudioFile.h>
#include <stdio.h>
#include <memory>
#include <chrono>
#include <ctime>

Sample::Sample()
{
}

Sample::~Sample()
{
    delete envGen_;
}

void Sample::init(int fs)
{
    std::string gFilename = "skateboy.wav";
    numFrames_ = AudioFileUtilities::getNumFrames(gFilename);
    printf("sample_.numFrames: %d\n", numFrames_);
    dataL_ = AudioFileUtilities::load(gFilename, numFrames_, 0)[0];
    dataR_ = AudioFileUtilities::load(gFilename, numFrames_, 1)[0];
    envGen_ = new ADSR(fs);
}

void Sample::process(float out[2])
{
    int state = envGen_->getState();
    if (state != state_) {
        state_ = state;
        printf("ADSR state: %d\n", state);
    }

    float env = envGen_->process();
    out[0] = env * dataL_[readCounter_];
    out[1] = env * dataR_[readCounter_];
    readCounter_++;
    if (readCounter_ >= numFrames_)
        readCounter_ = 0;

}

void Sample::setPlaying(bool playing)
{
    envGen_->gate(playing);

    if (playing)
        readCounter_ = 0;
}
