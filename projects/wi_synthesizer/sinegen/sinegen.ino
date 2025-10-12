#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <Bounce.h>

#include "AudioEffectBypass.h"

const int myInput = AUDIO_INPUT_LINEIN;

AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioEffectBypass   l_myEffect;
AudioEffectBypass   r_myEffect;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

AudioConnection c1(audioInput, 0, l_myEffect, 0);
AudioConnection c2(audioInput, 0, r_myEffect, 0);

AudioConnection c3(l_myEffect, 0, audioOutput, 0);
AudioConnection c4(r_myEffect, 0, audioOutput, 1);

AudioControlSGTL5000 audioShield;

// <<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
void setup() {
  
  Serial.begin(9600);
  while (!Serial) ;
  delay(3000);

  AudioMemory(4);

  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.lineInLevel(0);
  audioShield.volume(0.7);

  // Initialize the effect - left channel
  if(!l_myEffect.begin()) {
    Serial.println("AudioEffectBypass - left channel begin failed");
    while(1);
  }

  // Initialize the effect - right channel
  if(!r_myEffect.begin()) {
    Serial.println("AudioEffectBypass - left channel begin failed");
    while(1);
  }

  Serial.println("setup done");
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
  l_myEffect.volume(1.0f);
  r_myEffect.volume(1.0f);
}


// audio volume
int volume = 0;

unsigned long last_time = millis();
void loop()
{
  
  // Volume control
  // int n = analogRead(A1);
  // if (n != volume) {
  //   volume = n;
  //   audioShield.volume((float)n / 1023);
  //   //l_myEffect.volume((float)n / 1023);
  //   //r_myEffect.volume((float)n / 1023);
  // }


if(0) {
  if(millis() - last_time >= 5000) {
    Serial.print("Proc = ");
    Serial.print(AudioProcessorUsage());
    Serial.print(" (");    
    Serial.print(AudioProcessorUsageMax());
    Serial.print("),  Mem = ");
    Serial.print(AudioMemoryUsage());
    Serial.print(" (");    
    Serial.print(AudioMemoryUsageMax());
    Serial.println(")");
    last_time = millis();
  }
}
}

/*
int a1history=0;
float amplitude = 0.0f;

void setup() {
  AudioMemory(10);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  Serial.begin(115200);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.3);
  l_myEffect.voices(0);
  r_myEffect.voices(0);
  delay(1000);
  a1history = analogRead(A1);
}

void loop() {
  // no nothing
  //amplitude = a1history / 1000.0f;
  //wait(10);
}

// void wait(unsigned int milliseconds)
// {
//   elapsedMillis msec=0;

//   while (msec <= milliseconds) {
//     int a1 = analogRead(A1);
//     if (a1 > a1history + 10 || a1 < a1history - 10) {
//       Serial.print("Knob (pin A1) = ");
//       Serial.println(a1);
//       a1history = a1;
//     }
//   }
// }
*/


