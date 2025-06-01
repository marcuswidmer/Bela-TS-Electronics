#include "Voice.hpp"

#include <stdio.h>
#include <memory>
#include <chrono>
#include <ctime>
#include <iostream>

Voice::Voice(DataSet * ds, DataSet * secondDs, DataSet * droneDs) : ds_(ds), secondDs_(secondDs), droneDs_(droneDs)
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

void Voice::process(float out[2])
{
    float tmpOut[2];
    float tmp2Out[2];
    float tmp3Out[2];
    processDataSet(tmpOut, ds_);
    processDataSet(tmp2Out, secondDs_);
    processDataSet(tmp3Out, droneDs_, 1);
    out[0] = tmpOut[0] + tmp2Out[0] + tmp3Out[0];
    out[1] = tmpOut[1] + tmp2Out[1] + tmp3Out[1];
}

void Voice::processDataSet(float out[2], DataSet * ds)
{
    if (!ds->valid || note_ < ds->minNote || note_ > ds->maxNote) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        return;
    }

    int state = envGen_->getState();
    if (state != state_) {
        state_ = state;
    }

    float env = envGen_->process();

    out[0] = velocity_ * env * ds->dataL.at(note_)[readCounter_];
    out[1] = velocity_ * env * ds->dataR.at(note_)[readCounter_];

    if (readCounter_ < getNumFrames())
        readCounter_++;
    else if (ds->loopSamples)
        readCounter_ = 0;
    else
        setPlaying(false);
}

void Voice::processDataSet(float out[2], DataSet * ds, int note)
{
    if (!ds->valid || note < ds->minNote || note > ds->maxNote) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        return;
    }

    int state = envGen_->getState();
    if (state != state_) {
        state_ = state;
    }

    float env = envGen_->process();

    out[0] = velocity_ * env * ds->dataL.at(note)[readCounter_];
    out[1] = velocity_ * env * ds->dataR.at(note)[readCounter_];

    if (readCounter_ < getNumFrames())
        readCounter_++;
    else if (ds->loopSamples)
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
    return ds_->numFrames.at(note_);
}