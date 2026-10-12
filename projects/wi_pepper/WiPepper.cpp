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
    stopMidiNotes();
    midiOutput_ = std::move(midiOutput);
    program_ = PgmSequencer;
    speedSmoothed_ = 0;
    directMidiMode_ = false;
    noteOverlay_ = false;
    directPitch_ = 0;
    directTriggerCountdown_ = 0;
    stepCountdown_ = priorityCountdown_ = syncCountdown_ = 0;
    sequencerButton_ = false;
    sequencerLed_ = false;
    sequenceTriggered_ = false;
    cvOutputs_ = {};
    sequencer.init(controlSampleRate_,
        [this](float seconds) {
            stepCountdown_ = std::lround(seconds * controlSampleRate_);
            sequenceTriggered_ = true;
        },
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
        stopMidiNotes();
        if(program_ == PgmSequencer && midiOutput_) midiOutput_(0xFC);
        juno.panic(); // Do not carry held notes, pedals or chorus tails across programs.
        program_ = static_cast<Program>((program_ + 1) % ProgramCount);
        directMidiMode_ = false;
        noteOverlay_ = false;
        directPitch_ = 0;
        directTriggerCountdown_ = 0;
        cvOutputs_ = {};
        stepCountdown_ = priorityCountdown_ = syncCountdown_ = 0;
        sequencerButton_ = false;
        sequencerLed_ = false;
        sequencer.setButton(false);
        sequencer.midiClock_ = false;
    } else if(program_ == PgmSequencer) {
        if(button == 1) {
            stopMidiNotes();
            directMidiMode_ = !directMidiMode_;
            directPitch_ = 0;
            directTriggerCountdown_ = 0;
            cvOutputs_ = {};
            stepCountdown_ = priorityCountdown_ = syncCountdown_ = 0;
            sequencerButton_ = sequencerLed_ = false;
            sequencer.setButton(false);
            if(directMidiMode_ && midiOutput_) midiOutput_(0xFC);
        } else if(button == 2) {
            sequencer.freezeSequenceChanged(!sequencer.sequenceFrozen());
        } else if(button == 3 && !directMidiMode_) {
            noteOverlay_ = !noteOverlay_;
            if(!noteOverlay_) stopLiveMidiNotes();
        }
    } else if(program_ == PgmJuno) {
        juno.pressButton(button);
    } else {
        resonator.pressButton(button);
    }
}

void WiPepper::midiNoteOn(unsigned int note, unsigned int velocity, unsigned int channel)
{
    if(note > 127 || velocity > 127 || channel > 15) return;
    if(directMidiMode() || noteOverlay()) {
        sendLiveMidiNote(0x90, note, velocity, channel);
        if(noteOverlay() || !velocity) return;
    }
    if(!velocity) { midiNoteOff(note, channel); return; }
    if(program_ == PgmSequencer) {
        if(directMidiMode_) {
            directPitch_ = std::max(0.0f, std::min(1.0f, (static_cast<float>(note) - 39) / 60));
            directTriggerCountdown_ = std::max(1, static_cast<int>(std::lround(controlSampleRate_ * .01f)));
        } else sequencer.assignNote(static_cast<int>(note) - 39);
    }
    else if(program_ == PgmJuno)
        juno.noteOn(note, velocity, channel);
}

void WiPepper::midiNoteOff(unsigned int note, unsigned int channel, unsigned int velocity)
{
    if(note > 127 || channel > 15 || velocity > 127) return;
    if(directMidiMode() || noteOverlay()) sendLiveMidiNote(0x80, note, velocity, channel);
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

void WiPepper::sendMidiNote(uint8_t status, unsigned int note, unsigned int velocity, unsigned int channel)
{
    if(!midiOutput_) return;
    midiOutput_(status | channel);
    midiOutput_(note);
    midiOutput_(velocity);
}

void WiPepper::sendLiveMidiNote(uint8_t status, unsigned int note, unsigned int velocity, unsigned int channel)
{
    if(!midiOutput_) return;
    const bool held = status == 0x90 && velocity > 0;
    liveMidiNotes_[channel][note] = held;
    // A pitch shared with the sequence stays on until both sources release it.
    if(!held && channel == 0 && sequenceMidiNote_ == static_cast<int>(note)) return;
    sendMidiNote(status, note, velocity, channel);
}

void WiPepper::stopLiveMidiNotes()
{
    for(unsigned int channel = 0; channel < liveMidiNotes_.size(); ++channel)
        for(unsigned int note = 0; note < liveMidiNotes_[channel].size(); ++note)
            if(liveMidiNotes_[channel][note]) sendLiveMidiNote(0x80, note, 0, channel);
}

void WiPepper::stopSequenceMidiNote()
{
    if(sequenceMidiNote_ >= 0 && !liveMidiNotes_[0][sequenceMidiNote_])
        sendMidiNote(0x80, sequenceMidiNote_, 0, 0);
    sequenceMidiNote_ = -1;
    sequenceMidiCountdown_ = 0;
}

void WiPepper::stopMidiNotes()
{
    stopSequenceMidiNote();
    stopLiveMidiNotes();
}

void WiPepper::processControls()
{
    if(program_ != PgmSequencer) return;
    if(directMidiMode_) {
        cvOutputs_[0] = directPitch_;
        cvOutputs_[1] = directTriggerCountdown_ > 0;
        cvOutputs_[2] = cvOutputs_[3] = 0;
        sequencerLed_ = false;
        if(directTriggerCountdown_ > 0) --directTriggerCountdown_;
        return;
    }
    if(sequenceMidiCountdown_ > 0 && --sequenceMidiCountdown_ == 0)
        stopSequenceMidiNote();
    // Match PgmSequencer's analog-rate smoothing and control mapping.
    speedSmoothed_ += 0.0001f * (pots_[0] - speedSmoothed_);
    const float stepPeriod = 1.0f / (0.4f + 14.0f * speedSmoothed_);
    sequencer.setPeriod(stepPeriod);
    sequencer.setButton(sequencerButton_);
    sequencerButton_ = false;
    sequencer.setRandomSequence(pots_[2] > 0.5f);
    sequencer.includeFractionOfDefaultSequence(pots_[1]);
    sequencer.setSubstepBehaviour(pots_[1]);
    sequenceTriggered_ = false;
    sequencer.process();
    if(sequenceTriggered_ && midiOutput_) {
        stopSequenceMidiNote(); // Retrigger even when successive steps share a pitch.
        sequenceMidiNote_ = std::max(0, std::min(127, sequencer.currentNote_ + 39));
        if(!liveMidiNotes_[0][sequenceMidiNote_])
            sendMidiNote(0x90, sequenceMidiNote_, 100, 0);
        // Pot 3 sets each new MIDI note's duration, from 10 ms to one step.
        const float noteSeconds = .01f + pots_[3] * (stepPeriod - .01f);
        sequenceMidiCountdown_ = std::max(1, static_cast<int>(std::lround(controlSampleRate_ * noteSeconds)));
    }
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
