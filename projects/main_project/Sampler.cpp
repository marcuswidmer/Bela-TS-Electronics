#include "Sampler.hpp"
#include "../main_project/mainCommon.hpp"

#include <Bela.h>
#include <libraries/Midi/Midi.h>

void midiMessageCallback(MidiChannelMessage message, void* arg)
{
    auto * sampler = static_cast<Sampler *>(arg);
	if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
		if (message.getType() == kmmNoteOn) {
			rt_printf("note on: %d\n", message.getDataByte(0));
            sampler->setNote(message.getDataByte(0) - sampler->getMidiNoteOffset());
            sampler->playNewVoice();

        } else if (message.getType() == kmmNoteOff) {
			rt_printf("note off: %d\n", message.getDataByte(0) - sampler->getMidiNoteOffset());
		}

	} else if (message.getType() == kmmControlChange) {
        rt_printf("control change\n");
    }
}

Sampler::Sampler() : midi_()
{
}

void Sampler::init(int fs)
{
    for (int i = 0; i < numVoices_; ++i) {
        samples_[i] = new Sample();
        samples_[i]->init(fs);
    }

    midi_.readFrom(midiPort_);
	//midi_.writeTo(midiPort_);
	midi_.enableParser(true);
	midi_.getParser()->setCallback(midiMessageCallback, (void*) this);
}

void Sampler::process(float out[2])
{
    float out_tmp[2];
    out_tmp[0] = 0.0f;
    out_tmp[1] = 0.0f;

    for (int i = 0; i < numVoices_; ++i) {
        samples_[i]->process(out_tmp);
        out[0] += amplitude_ * out_tmp[0];
        out[1] += amplitude_ * out_tmp[1];
    }
}

void Sampler::playNewVoice(int note)
{
    currentVoiceIdx_++;
    if (currentVoiceIdx_ >= numVoices_)
        currentVoiceIdx_ = 0;

    samples_[currentVoiceIdx_]->setNote(note);
    samples_[currentVoiceIdx_]->setPlaying(true);
    activeVoices_[note] = currentVoiceIdx_;
}

void Sampler::releaseVoice(int note)
{
    if (!activeVoices_.contains(note)) {
        printf("Inactive voice released. This should not happen\n");
        return;
    }

    samples_[activeVoices_.at(note)]->setPlaying(false);
}

void Sampler::setAnalogIns(AnalogIns ins)
{
    amplitude_ = map(ins.input_1, 0, 1, 0, 1);
    bool play = ins.input_7 > 0.42;
    // if (play != play_) {
    //     if (play)
    //         playNewVoice();
    //     else
    //         releaseCurrentSample();
    //     play_ = play;
    // }

}
