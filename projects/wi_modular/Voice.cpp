#include "Voice.hpp"

#include <stdio.h>
#include <memory>
#include <chrono>
#include <ctime>
#include <iostream>

Voice::Voice(DataSet * ds, DataSet * secondDs) : ds_(ds), secondDs_(secondDs)
{
}

Voice::~Voice()
{
    delete envGen_;
}

void Voice::init(int fs)
{
    envGen_ = new ADSR(fs);
}

void Voice::process(float out[2], float firstSecondMix)
{
    if (!ds_->valid || note_ < ds_->minNote || note_ > ds_->maxNote) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        return;
    }

    if (!secondDs_->valid || note_ < secondDs_->minNote || note_ > secondDs_->maxNote) {
        firstSecondMix = 0.0f;
    }

    int state = envGen_->getState();
    if (state != state_) {
        state_ = state;
    }

    float env = envGen_->process();

    float firstOut[2];
    float secondOut[2];

    firstOut[0] = velocity_ * env * ds_->dataL.at(note_)[readCounter_];
    firstOut[1] = velocity_ * env * ds_->dataR.at(note_)[readCounter_];

    if (firstSecondMix) {
        secondOut[0] = velocity_ * env * secondDs_->dataL.at(note_)[readCounter_];
        secondOut[1] = velocity_ * env * secondDs_->dataR.at(note_)[readCounter_];
    }

    out[0] = (1 - firstSecondMix) * firstOut[0] + firstSecondMix * secondOut[0];
    out[1] = (1 - firstSecondMix) * firstOut[1] + firstSecondMix * secondOut[1];

    if (readCounter_ < getNumFrames())
        readCounter_++;
    else if (ds_->loopSamples)
        readCounter_ = 0;
    else
        setPlaying(false);
}

void Voice::setPlaying(bool playing, float velocity)
{
    envGen_->gate(playing);

    if (playing) {
        readCounter_ = 0;
        velocity_ = velocity;
    }
}

void Voice::setNote(int note)
{
    note_ = note;
}

int Voice::getNumFrames()
{
    if (!secondDs_->valid)
        return ds_->numFrames.at(note_);

    return std::min(ds_->numFrames.at(note_), secondDs_->numFrames.at(note_));
}

void Voice::setRelease(float r)
{
    envGen_->setRelease(r);
}

void Voice::setVelocity(float v)
{
    velocity_ = v;
}
