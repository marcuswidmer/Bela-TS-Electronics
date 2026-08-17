#ifndef effect_bypass_h_
#define effect_bypass_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h

/******************************************************************/

class AudioEffectBypass : 
public AudioStream
{
public:
  AudioEffectBypass(void):
  AudioStream(1,inputQueueArray)
  { }

  boolean begin();
  virtual void update(void);
  void volume(float vol);
  
private:
  audio_block_t *inputQueueArray[1];
  float volume_ = 0.0f;
};

#endif
