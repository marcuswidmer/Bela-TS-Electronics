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
#include <optional>

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

    for (int i = 0; i < WI_SYNTHESIZER_NUM_VOICES; ++i)
    {
        voices_[i].note_ = 0;
        voices_[i].envGen_ = new ADSR(fs);
        voices_[i].envGen_->setADSR(200, 100000, 1.0, 0.2);


        voices_[i].oscs[0].type = OscType::Sine;
        voices_[i].oscs[0].active = false;

        voices_[i].oscs[1].type = OscType::Saw;
        voices_[i].oscs[1].active = true;
        voices_[i].oscs[1].noteOffset = -12;

        voices_[i].oscs[2].type = OscType::Klokkenspel;
        voices_[i].oscs[2].active = true;

        voices_[i].oscs[3].type = OscType::Human;
        voices_[i].oscs[3].active = false;

        voices_[i].oscs[4].type = OscType::AnalogStrings;
        voices_[i].oscs[4].active = true;
        voices_[i].oscs[4].left = true;

        voices_[i].oscs[5].type = OscType::AnalogStringsAlt;
        voices_[i].oscs[5].active = true;
        voices_[i].oscs[5].left = false;

        voices_[i].oscs[6].type = OscType::PulseDuty50;
        voices_[i].oscs[6].active = false;

        voices_[i].oscs[7].type = OscType::Tri;
        voices_[i].oscs[7].active = true;


        for (int k = 0; k < WI_SYNTHESIZER_NUM_OSCILLATORS; ++k)
        {
            for (int j = 0; j < WI_SYNTHESIZER_NUM_SAMPLES_PER_WAVE; ++j)
            {
                voices_[i].oscs[k].waveData[j] = getWavetableData(voices_[i].oscs[k].type, j);
            }
        }
    }
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
                    if (not voices_[i].oscs[j].left.has_value())
                        sampleMono += sample;
                    else if (*voices_[i].oscs[j].left)
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
    float tmpOut[2];
    granular_.process(vibratoOutput, tmpOut);
    out[0] = amplitude_ * (vibratoOutput + sampleL);
    out[1] = amplitude_ * (vibratoOutput + sampleR);
    runner_++;
}

void WiSynthesizer::playNewVoice(int note, int velocity)
{
    currentVoiceIdx_++;
    if (currentVoiceIdx_ >= WI_SYNTHESIZER_NUM_VOICES)
        currentVoiceIdx_ = 0;
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
}