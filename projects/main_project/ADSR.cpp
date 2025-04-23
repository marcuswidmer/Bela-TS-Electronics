//
//  ADSR.cpp
//
//  Created by Nigel Redmon on 12/18/12.
//  EarLevel Engineering: earlevel.com
//  Copyright 2012 Nigel Redmon
//
//  For a complete explanation of the ADSR envelope generator and code,
//  read the series of articles by the author, starting here:
//  http://www.earlevel.com/main/2013/06/01/envelope-generators/
//
//  License:
//
//  This source code is provided as is, without warranty.
//  You may copy and distribute verbatim copies of this document.
//  You may modify and use this source code to create binary code for your own purposes, free or commercial.
//
//  1.01  2016-01-02  njr   added calcCoef to SetTargetRatio functions that were in the ADSR widget but missing in this code
//  1.02  2017-01-04  njr   in calcCoef, checked for rate 0, to support non-IEEE compliant compilers
//

#include "ADSR.h"

#include <stdio.h>


ADSR::ADSR(float sampleRate) {
  reset();
  this->sampleRate = sampleRate;
  setADSR(127, 10, 126, 100);
  int attackTimeMs = 1 / (sampleRate * attackCoef) * 1000;
  int decayTimeMs = 1 / (sampleRate * decayCoef) * 1000;
  int releaseTimeMs = 1 / (sampleRate * releaseCoef) * 1000;
  printf("Attack (ms): %d , Decay (ms): %d, Sustain (level): %lf, Release (ms): %d\n", attackTimeMs, decayTimeMs, sustainLevel, releaseTimeMs);
}

ADSR::~ADSR(void) {
}

void ADSR::setADSR(int8_t attack, int8_t decay, int8_t sustain, int8_t release)
{
  setSustainMidi(sustain);
  setAttackMidi(attack);
  setDecayMidi(decay);
  setReleaseMidi(release);
}
void ADSR::setAttack(float attack) {

  this->attack = attack;
  if (attack < 0.001)
  {
    attack = 0.001;
  }

  attackCoef = 1.0/attack;
  attackBase = 0.0;

}

void ADSR::setDecay(float decay) {
  this->decay = decay;
  if (decay < 0.001)
  {
    decay = 0.001;
  }
  decayCoef = (1.0 - this->sustainLevel) / this->decay;
}

void ADSR::setSustain(float level) {
  sustainLevel = level;
  decayBase = sustainLevel;
}

void ADSR::setRelease(float releaseval)
{
  release = releaseval;
  if (release < 0.001)
  {
    release = 0.001;
  }
  releaseCoef = (this->sustainLevel / this->release);
  releaseBase = this->sustainLevel;
}

void ADSR::setAttackMidi(int8_t attack)
{
  this->midiAttack=attack;
  this->attack = ((float)midiAttack)/127.0;
  double maxlength = 0.01 * sampleRate;
  if(midiAttack==0)
  {
    attackCoef = 1.0;
  }
  else
  {
    attackCoef = 127.0 /(maxlength*(double)midiAttack);
  }
  attackBase = 0.0;

}
void ADSR::setDecayMidi(int8_t decay)
{
  this->midiDecay=decay;
  this->decay = ((float)this->midiDecay)/127.0;
  double maxlength = 1.0*sampleRate;
  if(midiDecay==0)
  {
    decayCoef=1.0 - this->sustainLevel;
  }
  else
  {
    decayCoef = ((1.0 - this->sustainLevel)*127.0) / (maxlength*(float)midiDecay);
  }
}

void ADSR::setSustainMidi(int8_t sustain)
{
  this->midiSustain=sustain;
  this->sustainLevel = ((float)midiSustain)/127.0;
  decayBase = sustainLevel;
}

void ADSR::setReleaseMidi(int8_t release)
{
  this->midiRelease=release;
  this->release = ((float)this->midiRelease)/127.0;
  double maxlength = sampleRate*5.0;

  if(midiRelease==0)
  {
    releaseCoef=this->sustainLevel;
  }
  else
  {
    releaseCoef = (this->sustainLevel*127.0) / (maxlength*((double)midiRelease));
    //releaseCoef = ((this->sustainLevel * 127.0)/maxlength)/(float)midiRelease;
  }
  releaseBase = this->sustainLevel;
}
