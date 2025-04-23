//
//  ADSR.h
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

#ifndef ADSR_h
#define ADSR_h

#include <cstdint>
#include <stdio.h>

class  ADSR {
public:
  ADSR();
	ADSR(float sampleRate);
	~ADSR(void);

	float process(void);
  float getOutput(void);
  int getState(void);
	void setADSR(int8_t attack, int8_t decay, int8_t sustain, int8_t release);
	void gate(int on);
  void setAttack(float attack);
  void setDecay(float decay);
  void setRelease(float release);
	void setSustain(float level);
  float getAttack(){return attack;}
  float getDecay(){return decay;}
  float getSustain(){return sustainLevel;}
  float getRelease(){return release;}
  void reset(void);
  void setAttackMidi(int8_t attack);
  void setDecayMidi(int8_t decay);
  void setSustainMidi(int8_t sustain);
  void setReleaseMidi(int8_t release);
  int8_t getAttackMidi(){return midiAttack;}
  int8_t getDecayMidi(){return midiDecay;}
  int8_t getSustainMidi(){return midiSustain;}
  int8_t getReleaseMidi(){return midiRelease;}
  double getAttackCoef(){return attackCoef;}
  double getDecayCoef(){return decayCoef;}
  double getReleaseCoef(){return releaseCoef;}
  double getReleaseBase(){return releaseBase;}

  enum envState {
        env_idle = 0,
        env_attack,
        env_decay,
        env_sustain,
        env_release
    };

protected:

  int state;
  float output;
	float attack;
	float decay;
	float release;
	int32_t counter;
	double attackCoef;
	double decayCoef;
	double releaseCoef;
	double sustainLevel;
  double attackBase;
  double decayBase;
  double releaseBase;
  int8_t midiAttack;
  int8_t midiDecay;
  int8_t midiSustain;
  int8_t midiRelease;
  float sampleRate;
};

inline float ADSR::process() {
	switch (state) {
        case env_idle:
			output = 0.0;
            break;
        case env_attack:
            output = output + attackBase + attackCoef;
            if (output >= 1.0) {
                output = 1.0; //http://www.earlevel.com/main/2013/06/01/envelope-generators/
                state = env_decay;

            }
            break;
        case env_decay:
            output = output - decayCoef;

            if (output <= sustainLevel || output <=0) {
                output = sustainLevel;
                state = env_sustain;
            }
            break;
        case env_sustain:
			    output = sustainLevel;
            break;
        case env_release:
            output = output - releaseCoef;

            if (output <= 0.0) {
                output = 0.0;
                state = env_idle;
            }
			break;
	}
	return output;
}

inline void ADSR::gate(int gate) {
    if (gate)
	{
		output = 0.0;
		state = env_attack;
	}
	else if (state != env_idle)
	{
		state = env_release;

	}
}

inline int ADSR::getState() {
    return state;
}

inline void ADSR::reset() {
    state = env_idle;
    output = 0.0;
}

inline float ADSR::getOutput() {
	return output;
}

#endif