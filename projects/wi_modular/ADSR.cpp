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
  setADSR(1.0, 0.1, 0.9999, 0.03);
  float attackTimeMs = 1 / (sampleRate * attackCoef) * 1000;
  float decayTimeMs = 1 / (sampleRate * decayCoef) * 1000;
  float releaseTimeMs = 1 / (sampleRate * releaseCoef) * 1000;
  //printf("Attack (ms): %f , Decay (ms): %f, Sustain (level): %lf, Release (ms): %f\n", attackTimeMs, decayTimeMs, sustainLevel, releaseTimeMs);
}

ADSR::~ADSR(void) {
}

void ADSR::setADSR(float attack, float decay, float sustain, float release)
{
  setSustain(sustain);
  setAttack(attack);
  setDecay(decay);
  setRelease(release);
}
void ADSR::setAttack(float attack) {

  this->attack = attack;
  if (attack < 0.001)
  {
      attack = 0.001;
  }

  attackCoef = 1.0 / attack;
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
  double maxLength = sampleRate * 5;
  if (release < 0.001)
  {
    release = 0.001;
  }
  releaseCoef = this->sustainLevel / (maxLength * this->release);
  releaseBase = this->sustainLevel;
}
