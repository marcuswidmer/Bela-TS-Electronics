#include "Sampler.hpp"
#include "../main_project/mainCommon.hpp"

#include <Bela.h>
#include <libraries/Midi/Midi.h>

Sampler::Sampler() : midi_()
{
}

void Sampler::init(int fs)
{
    for (int i = 0; i < numSamples_; ++i) {
        samples_[i] = new Sample();
        samples_[i]->init(fs);
    }

    midi_.readFrom(midiPort_);
	//midi_.writeTo(midiPort_);
	midi_.enableParser(true);
	midi_.getParser()->setCallback(midiMessageCallback, (void*) midiPort_);
}

void Sampler::process(float out[2])
{
    float out_tmp[2];
    out_tmp[0] = 0.0f;
    out_tmp[1] = 0.0f;

    for (int i = 0; i < numSamples_; ++i) {
        samples_[i]->process(out_tmp);
        out[0] += amplitude_ * out_tmp[0];
        out[1] += amplitude_ * out_tmp[1];
    }
}

void Sampler::playNewSample()
{
    currentSampleIdx_++;
    if (currentSampleIdx_ >= numSamples_)
        currentSampleIdx_ = 0;

    samples_[currentSampleIdx_]->setPlaying(true);
}

void Sampler::releaseCurrentSample()
{
    samples_[currentSampleIdx_]->setPlaying(false);
}

void Sampler::setAnalogIns(AnalogIns ins)
{
    amplitude_ = map(ins.input_1, 0, 1, 0, 1);
    bool play = ins.input_7 > 0.42;
    if (play != play_) {
        if (play)
            playNewSample();
        else
            releaseCurrentSample();
        play_ = play;
    }

}

void Sampler::midiMessageCallback(MidiChannelMessage message, void* arg) {
	// if(arg != NULL){
	// 	rt_printf("Message from midi port %s ", (const char*) arg);
	// }
	//message.prettyPrint();
	if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
		if (message.getType() == kmmNoteOn)
			rt_printf("note on: %d\n", message.getDataByte(0));
		else if (message.getType() == kmmNoteOff) {
			rt_printf("note off: %d\n", message.getDataByte(0));
		}

	} else if (message.getType() == kmmControlChange) {
        rt_printf("control change");
	}

}
