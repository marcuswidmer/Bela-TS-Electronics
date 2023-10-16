// /*
//  ____  _____ _        _
// | __ )| ____| |      / \
// |  _ \|  _| | |     / _ \
// | |_) | |___| |___ / ___ \
// |____/|_____|_____/_/   \_\

// The platform for ultra-low latency audio and sensor processing

// http://bela.io

// A project of the Augmented Instruments Laboratory within the
// Centre for Digital Music at Queen Mary University of London.
// http://www.eecs.qmul.ac.uk/~andrewm

// (c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
//   Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
//   Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

// The Bela software is distributed under the GNU Lesser General Public License
// (LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
// */


// #include <Bela.h>
// #include <libraries/Midi/Midi.h>
// #include <stdlib.h>
// #include <cmath>

// float gFreq;
// float gPhaseIncrement = 0;
// bool gIsNoteOn = 0;
// int gVelocity = 0;
// float gSamplingPeriod = 0;
// int gSampleCount = 44100; // how often to send out a control change

// /*
//  * This callback is called every time a new input Midi message is available
//  *
//  * Note that this is called in a different thread than the audio processing one.
//  *
//  */
// void midiMessageCallback(MidiChannelMessage message, void* arg){
// 	if(arg != NULL){
// 		rt_printf("Message from midi port %s ", (const char*) arg);
// 	}
// 	message.prettyPrint();
// 	if(message.getType() == kmmNoteOn){
// 		gFreq = powf(2, (message.getDataByte(0) - 69) / 12.f) * 440.f;
// 		gVelocity = message.getDataByte(1);
// 		gPhaseIncrement = 2.f * (float)M_PI * gFreq * gSamplingPeriod;
// 		gIsNoteOn = gVelocity > 0;
// 		rt_printf("v0:%f, ph: %6.5f, gVelocity: %d\n", gFreq, gPhaseIncrement, gVelocity);
// 	}
// }

// void sysexCallback(midi_byte_t byte, void* arg)
// {
// 	printf("Sysex byte");
// 	if(arg != NULL){
// 		 printf(" from midi port %s", (const char*) arg);
// 	}
// 	printf(": %d\n", byte);
// }

// Midi midi;

// const char* gMidiPort0 = "hw:1,0,0";

// bool setup(BelaContext *context, void *userData)
// {
// 	midi.readFrom(gMidiPort0);
// 	midi.writeTo(gMidiPort0);
// 	midi.enableParser(true);
// 	midi.getParser()->setCallback(midiMessageCallback, (void*) gMidiPort0);
// 	midi.getParser()->setSysexCallback(sysexCallback, (void*) gMidiPort0);
// 	gSamplingPeriod = 1 / context->audioSampleRate;
// 	return true;
// }

// enum {kVelocity, kNoteOn, kNoteNumber};
// void render(BelaContext *context, void *userData)
// {
// 	// using MIDI control changes
// 	for(unsigned int n = 0; n < context->audioFrames; n++){
// 		float value;
// 		if(gIsNoteOn == 1){
// 			static float phase = 0;
// 			phase += gPhaseIncrement;
// 			if(phase > M_PI)
// 				phase -= 2.f * (float)M_PI;
// 			value = sinf(phase) * gVelocity/128.0f;
// 		} else {
// 			value = 0;
// 		}
// 		for(unsigned int ch = 0; ch < context->audioOutChannels; ++ch)
// 			audioWrite(context, n, ch, value);
// 		// the following block sends a control change output every gSampleCount samples
// 		static int count = 0;
// 		if(count % gSampleCount == 0){
// 			static bool state = 0;
// 			state = !state;
// 			midi_byte_t statusByte = 0xB0; // control change on channel 0
// 			midi_byte_t controller = 30; // controller number 30
// 			midi_byte_t value = state * 127; // value : 0 or 127
// 			midi_byte_t bytes[3] = {statusByte, controller, value};
// 			midi.writeOutput(bytes, 3); // send a control change message
// 		}
// 		++count;
// 	}
// }

// void cleanup(BelaContext *context, void *userData)
// {

// }
