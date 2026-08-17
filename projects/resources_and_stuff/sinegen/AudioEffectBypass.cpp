#include <Arduino.h>
#include "AudioEffectBypass.h"

/******************************************************************/

boolean AudioEffectBypass::begin()
{
#if 1
Serial.print("AudioEffectBypass.begin(");
Serial.println(")");
#endif
 
  return(true);
}

void AudioEffectBypass::update(void)
{
  audio_block_t *block;
  short *bp;
  //int c_idx;
  
  // Just passthrough
  block = receiveWritable(0);
  if (block) {
    bp = block->data;
    for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
      short sample = *bp;
      *bp = volume_ * sample;
      bp++;
    }
    transmit(block,0);
    release(block);
  }
  return;
}

void AudioEffectBypass::volume(float vol)
{
  volume_ = vol;
}
