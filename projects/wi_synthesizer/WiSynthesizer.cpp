#include "WiSynthesizer.hpp"
#include "GranularReverb.hpp"
#include "tables/sine.h"
#include "tables/saw.h"
#include "tables/analog_strings.h"
#include "tables/analog_strings_alt.h"
#include "tables/hammond_glass.h"
#include "tables/human.h"
#include "tables/klokkenspel.h"
#include "tables/rev_saw.h"
#include "tables/tri.h"
#include "tables/pulse_duty_50.h"

#include <cmath>

#ifndef USE_NO_MIDI
#include <libraries/Midi/Midi.h>
void midiMessageCallback(MidiChannelMessage message, void* arg)
{
    auto * wiSynth = static_cast<WiSynthesizer *>(arg);
	if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
		if (message.getType() == kmmNoteOn) {
            wiSynth->playNewVoice(message.getDataByte(0) - wiSynth->getMidiNoteOffset(), message.getDataByte(1) / 128.0f);
			rt_printf("note on: %d. Vel: %d\n", message.getDataByte(0),message.getDataByte(1));

        } else if (message.getType() == kmmNoteOff) {
            wiSynth->releaseVoice(message.getDataByte(0) - wiSynth->getMidiNoteOffset());
			rt_printf("note off: %d\n", message.getDataByte(0));
		}

	} else if (message.getType() == kmmControlChange) {
        rt_printf("control change\n");
    }
}
#endif

static float getWavetableData(OscType type, int idx)
{
    switch (type) {
        case OscType::Sine:
            return sine_wave[idx];
        case OscType::Saw:
            return saw[idx];
        case OscType::Klokkenspel:
            return klokkenspel[idx];
        case OscType::Human:
            return human[idx];
        case OscType::AnalogStrings:
            return analog_strings[idx];
        case OscType::AnalogStringsAlt:
            return analog_strings_alt[idx];
        case OscType::HammondGlass:
            return hammond_glass[idx];
        case OscType::PulseDuty50:
            return pulse_duty_50[idx];
        case OscType::Tri:
            return tri[idx];
        default:
            return 0.0f;
    }
}

WiSynthesizer::WiSynthesizer(int fs)
   : fs_(fs)
   , svf_(fs, 3000, 2)
   , vib_(fs)
   , granular_()
{
#ifndef USE_NO_MIDI
    midi_ = new Midi();
#endif

    for (int i = 0; i < WI_SYNTHESIZER_NUM_VOICES; ++i)
    {
        voices_[i].note_ = 0;
        voices_[i].envGen_ = new ADSR(fs);
        voices_[i].envGen_->setADSR(200, 100000, 1.0, 0.2);


        voices_[i].oscs[0].type = OscType::Klokkenspel;
        voices_[i].oscs[0].active = true;
        voices_[i].oscs[0].noteOffset = 12;

        voices_[i].oscs[1].type = OscType::AnalogStrings;
        voices_[i].oscs[1].active = true;
        voices_[i].oscs[1].left = true;
        voices_[i].oscs[1].isStereo = true;

        voices_[i].oscs[2].type = OscType::AnalogStringsAlt;
        voices_[i].oscs[2].active = true;
        voices_[i].oscs[2].left = false;
        voices_[i].oscs[2].isStereo = true;

        for (int k = 0; k < WI_SYNTHESIZER_NUM_OSCILLATORS; ++k)
        {
            for (int j = 0; j < WI_SYNTHESIZER_NUM_SAMPLES_PER_WAVE; ++j)
            {
                voices_[i].oscs[k].waveData[j] = getWavetableData(voices_[i].oscs[k].type, j);
            }
        }
    }
}

void WiSynthesizer::init()
{
#ifndef USE_NO_MIDI
    midi_->readFrom(midiPort_);
	midi_->enableParser(true);
	midi_->getParser()->setCallback(midiMessageCallback, (void*) this);
#endif
}

void WiSynthesizer::process(float out[2])
{
    float sampleMono = 0.0f;
    float sampleL = 0.0f;
    float sampleR = 0.0f;

    for (int i = 0; i < WI_SYNTHESIZER_NUM_VOICES; ++i)
    {
        float env = voices_[i].envGen_->process();

        if (voices_[i].note_ != 0)
        {
            for (int j = 0; j < WI_SYNTHESIZER_NUM_OSCILLATORS; ++j)
            {
                if (voices_[i].oscs[j].active) {
                    int note = voices_[i].note_ + voices_[i].oscs[j].noteOffset;
                    float freq = 440.0 * std::pow(2.0, (note - 69) / 12.0);
                    float baseFreq = 20.0f;
                    float stride = freq / baseFreq;

                    int idx = stride * runner_;
                    float sample = env * voices_[i].oscs[j].waveData[idx % WI_SYNTHESIZER_NUM_SAMPLES_PER_WAVE];
                    if (not voices_[i].oscs[j].isStereo)
                        sampleMono += sample;
                    else if (voices_[i].oscs[j].left)
                        sampleL += sample;
                    else
                        sampleR += sample;
                }
            }

            if (env < noteOffThreshold_)
            {
                voices_[i].note_ = 0;
            }
        }
    }

    float preFilter = sampleMono;
    FilterOutputs outputs = svf_.processSample(preFilter);
    float vibratoOutput = vib_.process(outputs.lowPass);
    //float tmpOut[2];
    //granular_.process(vibratoOutput, tmpOut);
    out[0] = amplitude_ * (vibratoOutput + sampleL);
    out[1] = amplitude_ * (vibratoOutput + sampleR);
    runner_++;
}

void WiSynthesizer::playNewVoice(int note, int velocity)
{
    currentVoiceIdx_++;
    if (currentVoiceIdx_ >= WI_SYNTHESIZER_NUM_VOICES)
        currentVoiceIdx_ = 0;

    for (int i = 0; i < WI_SYNTHESIZER_NUM_VOICES; ++i)
    {
        for (auto& pair : activeVoices_)
        {
    #ifndef USE_NO_MIDI
            rt_printf("Checking voices_\n");
    #endif
            if (pair.second == currentVoiceIdx_)
            {
                // voice is aready active
                currentVoiceIdx_++;
                if (currentVoiceIdx_ >= WI_SYNTHESIZER_NUM_VOICES)
                    currentVoiceIdx_ = 0;
                break;
            }
        }
    }

    voices_[currentVoiceIdx_].note_ = note;
    voices_[currentVoiceIdx_].envGen_->gate(true);
    activeVoices_.insert({note, currentVoiceIdx_});
}

void WiSynthesizer::releaseVoice(int note)
{
    if (!activeVoices_.count(note)) {
        printf("Inactive voice released. This should not happen\n");
        return;
    }

    auto voicesWithNote = activeVoices_.equal_range(note);
    auto it = voicesWithNote.first;
    voices_[it->second].envGen_->gate(false);
    activeVoices_.erase(it);
#ifndef USE_NO_MIDI
    rt_printf("Releasing note %d on voice_ nr:%d\n", note, it->second);
#endif
}