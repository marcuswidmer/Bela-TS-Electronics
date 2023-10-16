/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

*/

#include <Bela.h>
#include <q/pitch_detector.hpp>
#include <libraries/Midi/Midi.h>

// Marcus destroyed again Okt 2021
int gSensorInput0 = 6;
int gSensorInput1 = 7;
int gSensorInput2 = 3;
int gSensorInput3 = 5;
int gSensorInput4 = 0;
int gSensorInput5 = 2;
int gSensorInput6 = 4;
int gSensorInput7 = 1;

Midi midi;

const char* gMidiPort0 = "hw:1,0,0";

cycfi::q::pitch_detector<>pitchDet(80, 1000, 44100, 0.1);
float out_2_ma = 0.0f;
float alpha = 0.999f;
float thresholdRising = 0.1;
float thresholdFalling = 0.1;

bool noteIsOn = false;
int theNote = 0;
float inputGain = 0.0f;
int prevNote = 0;

int gAudioFramesPerAnalogFrame = 0;

bool setup(BelaContext *context, void *userData)
{
    if(context->audioFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    midi.readFrom(gMidiPort0);
    midi.writeTo(gMidiPort0);
    midi.enableParser(true);

	return true;
}

void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
        if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            inputGain = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7), 0, 1, 0, 1);
        }

        float out = inputGain * audioRead(context, n, 0);
        out_2_ma = alpha * out_2_ma + (1 - alpha) * (out * out);
        std::size_t extra = 0;
        pitchDet(out, extra);
    }
    float rms = sqrt(out_2_ma);
    int note = 12 * log2(32 / 440.0 * pitchDet.frequency()) + 9;
    //printf("Note: %d\n", note);
    bool sendNoteOn = false;
    bool sendNoteOff = false;

    // Rising and falling edge detect
    if (rms > thresholdRising && !noteIsOn) {
        sendNoteOn = true;
        noteIsOn = true;
    } else if ((rms <= thresholdFalling || note != prevNote) && noteIsOn) {
        sendNoteOff = true;
        noteIsOn = false;
    }

    if (sendNoteOn) {
        midi.writeNoteOn(0, note, 100);
        theNote = note;
        printf("NoteOn: %d\n", note);
        sendNoteOn = false;
    }

    if (sendNoteOff) {
        midi.writeNoteOff(0, theNote, 100);
        printf("NoteOff: %d\n", theNote);
        sendNoteOff = false;
    }
    prevNote = note;
    //printf("Rms: %f\n", );
}

void cleanup(BelaContext *context, void *userData)
{

}