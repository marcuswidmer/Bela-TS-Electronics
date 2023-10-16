#include "ADSR.h"

ADSR::ADSR()
{
	attackTime_ = 0.5;
	decayTime_ = 0.1;
	sustainLevel_ = 0.8;
	releaseTime_ = 1;

	state_ = StateOff;
}

void ADSR::setSampleRate(float rate) 
{
	ramp_.setSampleRate(rate);
}

void ADSR::trigger() 
{
	state_ = StateAttack;
	ramp_.rampTo(1.0, attackTime_);
}

void ADSR::release() 
{
	state_ = StateRelease;
	ramp_.rampTo(0, releaseTime_);
}

float ADSR::process() 
{
	if(state_ == StateAttack) {
		if (ramp_.finished()) {
			state_ = StateDecay;
			ramp_.rampTo(sustainLevel_, decayTime_);
		}
	}
	else if(state_ == StateDecay) {
		if (ramp_.finished()) {
			state_ = StateSustain;
		}
	}
	else if(state_ == StateRelease) {
		if (ramp_.finished()) {
			state_ = StateOff;
		}
	}

    return ramp_.process();
}

bool ADSR::isActive() 
{
	return (state_ != StateOff);
}

void ADSR::setAttackTime(float attackTime)
{
	attackTime_ = attackTime;
}

void ADSR::setDecayTime(float decayTime)
{
	decayTime_ = decayTime;
}

void ADSR::setSustainLevel(float sustainLevel)
{
	sustainLevel_ = sustainLevel;
}

void ADSR::setReleaseTime(float releaseTime)
{
	releaseTime_ = releaseTime;
}

// Destructor
ADSR::~ADSR() 
{
	// Nothing to do here
}