#include "WiPepper.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

bool WiPepper::setup(float sampleRate, float controlSampleRate,
                     std::function<void(uint8_t)> midiOutput)
{
    if(controlSampleRate == 0) controlSampleRate = sampleRate;
    if(!std::isfinite(controlSampleRate) || controlSampleRate <= 0 || !resonator.setup(sampleRate) || !juno.setup(sampleRate))
        return false;
    controlSampleRate_ = controlSampleRate;
    midiOutput_ = std::move(midiOutput);
    program_ = PgmSequencer;
    speedSmoothed_ = 0;
    stepCountdown_ = priorityCountdown_ = syncCountdown_ = 0;
    sequencerButton_ = false;
    sequencerLed_ = false;
    cvOutputs_ = {};
    sequencer.init(controlSampleRate_,
        [this](float seconds) { stepCountdown_ = std::lround(seconds * controlSampleRate_); },
        [this](float seconds) { priorityCountdown_ = std::lround(seconds * controlSampleRate_); },
        [this](float seconds) { syncCountdown_ = std::lround(seconds * controlSampleRate_); },
        [this](uint8_t byte) { if(midiOutput_) midiOutput_(byte); });
    return true;
}

void WiPepper::setPots(const std::array<float, 8>& pots)
{
    for(unsigned int i = 0; i < pots_.size(); ++i)
        pots_[i] = std::isfinite(pots[i]) ? std::max(0.0f, std::min(1.0f, pots[i])) : 0;
    volume = pots_[0];
    resonator.setPots(pots_);
    juno.setPots(pots_);
}

void WiPepper::pressButton(unsigned int button)
{
    if(button == 0) {
        if(program_ == PgmSequencer && midiOutput_) midiOutput_(0xFC);
        juno.panic(); // Do not carry held notes, pedals or chorus tails across programs.
        program_ = static_cast<Program>((program_ + 1) % ProgramCount);
        cvOutputs_ = {};
        stepCountdown_ = priorityCountdown_ = syncCountdown_ = 0;
        sequencerButton_ = false;
        sequencerLed_ = false;
        sequencer.setButton(false);
        sequencer.midiClock_ = false;
    } else if(program_ == PgmSequencer) {
        if(button == 1) sequencerButton_ = true;
    } else if(program_ == PgmJuno) {
        juno.pressButton(button);
    } else {
        resonator.pressButton(button);
    }
}

void WiPepper::midiNoteOn(unsigned int note, unsigned int velocity, unsigned int channel)
{
    if(note > 127 || velocity > 127 || channel > 15) return;
    if(!velocity) { midiNoteOff(note, channel); return; }
    if(program_ == PgmSequencer)
        sequencer.assignNote(static_cast<int>(note) - 39);
    else if(program_ == PgmJuno)
        juno.noteOn(note, velocity, channel);
}

void WiPepper::midiNoteOff(unsigned int note, unsigned int channel)
{
    if(program_ == PgmJuno) juno.noteOff(note, channel);
}

void WiPepper::midiControlChange(unsigned int controller, unsigned int value, unsigned int channel)
{
    if(program_ == PgmJuno) juno.controlChange(controller, value, channel);
}

void WiPepper::midiPitchBend(unsigned int value, unsigned int channel)
{
    if(program_ == PgmJuno) juno.pitchBend(value, channel);
}

void WiPepper::processControls()
{
    if(program_ != PgmSequencer) return;
    // Match PgmSequencer's analog-rate smoothing and control mapping.
    speedSmoothed_ += 0.0001f * (pots_[0] - speedSmoothed_);
    sequencer.setPeriod(1.0f / (0.4f + 14.0f * speedSmoothed_));
    sequencer.setButton(sequencerButton_);
    sequencerButton_ = false;
    sequencer.setRandomSequence(pots_[2] > 0.5f);
    sequencer.freezeSequenceChanged(pots_[2] > 0.5f);
    sequencer.includeFractionOfDefaultSequence(pots_[1]);
    sequencer.setSubstepBehaviour(pots_[1]);
    sequencer.process();
    cvOutputs_[0] = std::max(0.0f, std::min(1.0f, sequencer.currentNote_ / 60.0f));
    cvOutputs_[1] = stepCountdown_ > 0;
    cvOutputs_[2] = syncCountdown_ > 0;
    sequencerLed_ = stepCountdown_ > 0 || priorityCountdown_ > 0;
    if(stepCountdown_ > 0) --stepCountdown_;
    if(priorityCountdown_ > 0) --priorityCountdown_;
    if(syncCountdown_ > 0) --syncCountdown_;
}

void WiPepper::process(const float* in, float* out)
{
    out[0] = out[1] = 0;
    if(program_ == PgmKarplusResonator)
        resonator.process(in[0], out[0], out[1]);
    else if(program_ == PgmJuno)
        juno.process(out[0], out[1]);
}
