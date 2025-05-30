#include "Sample.hpp"

#include <stdio.h>
#include <memory>
#include <chrono>
#include <ctime>
#include <iostream>

Sample::Sample(DataSet * ds, DataSet * secondDs) : ds_(ds), secondDs_(secondDs)
{
}

Sample::~Sample()
{
    delete envGen_;
}

void Sample::init(int fs)
{
    envGen_ = new ADSR(fs);
}

void Sample::process(float out[2])
{
    float tmpOut[2];
    float tmp2Out[2];
    processDataSet(tmpOut, ds_);
    processDataSet(tmp2Out, secondDs_);
    out[0] = tmpOut[0] + tmp2Out[0];
    out[1] = tmpOut[1] + tmp2Out[1];
}

void Sample::processDataSet(float out[2], DataSet * ds)
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
    else
        setPlaying(false);

}

void Sample::setPlaying(bool playing, float velocity)
{
    envGen_->gate(playing);

    if (playing) {
        readCounter_ = 0;
        velocity_ = velocity;
    }
}

void Sample::setNote(int note)
{
    note_ = note;
}

int Sample::getNumFrames()
{
    return ds_->numFrames.at(note_);
}