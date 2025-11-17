#include "WiSustainer.hpp"

#include <cmath>


WiSustainer::WiSustainer(int fs)
   : fs_(fs)
   , rms_()
{
    for (int i = 0; i < WI_SUSTAINER_NUM_VOICES; ++i)
    {
        voices_[i].note_ = 0;
        voices_[i].envGen_ = new ADSR(fs);
        voices_[i].envGen_->setADSR(200, 100000, 1.0, 0.1);
        voices_[i].envGen_->setIdleCb([=]{ donePlaying(); });
        voices_[i].osc.active = true;
    }
}

void WiSustainer::init()
{
}

void WiSustainer::donePlaying()
{
    wavePhase_ = WavePhase::Measure;
    rms_.reset();
    printf("Done playing cb\n");
};

void WiSustainer::measureWave(float in)
{
    rms_.push(in);

    if (rms_.rms() > trigThresh_)
    {
        wavePhase_ = WavePhase::Record;
        withinZeroCrossings_ = false;
    }
}

void WiSustainer::recordWave(float in)
{
    // Toggle recording at rising edge zero-crossing
    if (prevIn_ < 0.0f && in > 0.0f) {
        if (not withinZeroCrossings_) {
            withinZeroCrossings_ = true;
            recordIdx_ = 0;
        } else if (withinZeroCrossings_ && recordIdx_ > 1000){
            withinZeroCrossings_ = false;
            voices_[currentVoiceIdx_].osc.waveLength = recordIdx_;
            wavePhase_ = WavePhase::Play;
            playIdx_ = 0;
            voices_[currentVoiceIdx_].envGen_->gate(true);
        }
    }

    if (withinZeroCrossings_) {
        voices_[currentVoiceIdx_].osc.waveData[recordIdx_] = in;
        recordIdx_++;
    }
}

void WiSustainer::playWave(float out[2])
{
    float env = voices_[currentVoiceIdx_].envGen_->process();
    float sample = env * voices_[currentVoiceIdx_].osc.waveData[playIdx_];
    out[0] = sample;
    out[1] = sample;

    if (++playIdx_ >= voices_[currentVoiceIdx_].osc.waveLength)
        playIdx_ = 0;
}

void WiSustainer::process(float in, float out[2], float * rms)
{
    float sampleMono = 0.0f;
    float sampleL = 0.0f;
    float sampleR = 0.0f;

    switch (wavePhase_) {
        case WavePhase::Measure:
            measureWave(in);
            break;

        case WavePhase::Record:
            recordWave(in);
            break;

        case WavePhase::Play:
            playWave(out);
            break;

    }
    // out[0] = 0.8 * out[0] + 0.2 * in;
    // out[1] = 0.8 * out[1] + 0.2 * in;
    prevIn_ = in;
    *rms = rms_.rms();
}

void WiSustainer::playNewVoice(int note, int velocity)
{
    currentVoiceIdx_++;
    if (currentVoiceIdx_ >= WI_SUSTAINER_NUM_VOICES)
        currentVoiceIdx_ = 0;

    for (int i = 0; i < WI_SUSTAINER_NUM_VOICES; ++i)
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
                if (currentVoiceIdx_ >= WI_SUSTAINER_NUM_VOICES)
                    currentVoiceIdx_ = 0;
                break;
            }
        }
    }

    voices_[currentVoiceIdx_].note_ = note;
    voices_[currentVoiceIdx_].envGen_->gate(true);
    activeVoices_.insert({note, currentVoiceIdx_});
}

void WiSustainer::releaseVoice(int note)
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
