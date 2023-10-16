#include <Bela.h>
#include <libraries/Midi/Midi.h>
#include <cmath>
#include <algorithm>
#include <vector>

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

int sendInterval = 44100 / 16;
int frameCounter = 0;
bool setup(BelaContext *context, void *userData)
{
	midi.readFrom(gMidiPort0);
	midi.writeTo(gMidiPort0);
	midi.enableParser(true);

	return true;
}

void render(BelaContext *context, void *userData)
{
	frameCounter++;
	if (frameCounter == sendInterval / 2) {
		// MidiChannelMessage message(kmmNoteOn);
		// message.setDataByte(0, 40);
		// message.setDataByte(1, 127);

		// int ret = midi.writeOutput(message.getDataByte(0));

		midi.writeNoteOn(0, 69, 100);
		midi.writeNoteOn(0, 70, 100);
		rt_printf("Note On\n");

	} else if (frameCounter == sendInterval) {
		// MidiChannelMessage message(kmmNoteOff);
		// message.setDataByte(0, 40);
		// message.setDataByte(1, 127);

		// int ret = midi.writeOutput(message.getDataByte(0));


		midi.writeNoteOff(0, 69, 100);
		midi.writeNoteOff(0, 70, 100);
		printf("Note Off \n");

		frameCounter = 0;
	}


}

void cleanup(BelaContext *context, void *userData)
{

}
