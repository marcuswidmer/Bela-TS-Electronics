#include "WiSequencer.hpp"
#include <cmath>
#include <chrono>
#include <cstdio>

WiSequencer::WiSequencer()
    : substepChance_(0, 100)
    , tickRequested_()
    , rd_()
    , gen_(rd_())
{
}

void WiSequencer::init(float fs, std::function<void(float)> ledCb, std::function<void(float)> priorityLedCb, std::function<void(float)> syncTriggerCb, std::function<void(uint8_t)> midiByteCb)
{
    fs_ = fs;

    notes_.assign(6, NoteInfo{0, true});
    numNotes_ = notes_.size();
    numGroups_ = 4;

    randomNotes_.assign(numGroups_, 0);

    // Initialize timing
    currentStep_ = 0;
    stepCountdown_ = 1;
    midiStepCountdown_ = 1;
    samplesPerStep_ = 1;

    ledCb_ = ledCb;
    priorityLedCb_ = priorityLedCb;
    syncTriggerCb_ = syncTriggerCb;
    midiByteCb_ = midiByteCb;

    freezeSequence_ = false;
}

void WiSequencer::process()
{
    if(!fs_)
        return;

    if (midiByteCb_ && midiClockSamplesPerTick_ > 0)
    {
        midiClockPhase_ += 1.0f;

        if (midiClockPhase_ >= midiClockSamplesPerTick_)
        {
            midiClockPhase_ -= midiClockSamplesPerTick_;
            midiByteCb_(0xF8);   // send MIDI clock (0xF8)
        }
    }

    if (!ledCb_) {
#ifndef USE_NO_MIDI
        rt_printf("led cb not set");
#endif
        return;
    }

    if (!priorityLedCb_) {
#ifndef USE_NO_MIDI
        rt_printf("priority led cb not set");
#endif
        return;
    }

    if(button_ && button_ != prevButton_)
    {
        printf("Changing assign mode\n");
        //assignMode_ = !assignMode_;
        resyncRequested_ = true;
        if(assignMode_)
            priorityLedCb_(1);
        else {
            priorityLedCb_(0.5f);
            assignedSequence_ = false;
            includeFractionOfDefaultSequence(0.5f);
        }
    }
    prevButton_ = button_;

    if (numNotes_ == 0) {
        return;
    }

    if (assignCountdown_ > 0)
        assignCountdown_--;

    // if (freezeChangeCountdown_ > 0)
    //     freezeChangeCountdown_--;

    if (midiClock_) {
        if (tickRequested_) {
            currentStep_ = getNextStep();
            ledCb_(0.01);
            tickRequested_ = false;
        }
    } else {
        int newSamplesPerStep = (int)lroundf(period_ * fs_);
        if (newSamplesPerStep == 0)
            newSamplesPerStep = 1;
        samplesPerStep_ = newSamplesPerStep;

        produceSubsteps();

        if(--stepCountdown_ <= 0)
        {
            rt_printf("BPM: %f\n", 60 / period_);
            stepCountdown_ = samplesPerStep_;
            currentStep_ = getNextStep();
            ledCb_(0.01);
            if (resyncRequested_) {
                midiByteCb_(0xFC);
                midiByteCb_(0xFA);
                midiClockPhase_ = 0;
                midiByteCb_(0xF8);
                resyncRequested_ = false;
            }
            //syncTriggerCb_(0.03); // Teori: Hvis triggersignalene er like lange, så vil det noen ganger skje et spenningsfall slik at korg volca beats ikke får med seg triggeret. Løsning: La sync trigger vare litt lengre slik at triggeret kommer gjennom
            //midiClockCb_();
            //rt_printf("clocl\n");
            // Ny teori: Støy på trigger signalet fører noen ganger til falske positive triggers
        }
    }

    currentNote_ = notes_[currentStep_].note;
}

void WiSequencer::tick()
{
    midiClock_ = true;
    tickRequested_ = true;
}

void WiSequencer::setSubstepBehaviour(float val)
{
    if (not assignedSequence_) return;

    if (val > 0 and val <= 0.25) {
        stepDivider_ = 1;
        substepLikelihood_ = 0;
    } else if (val > 0.25 and val <= 0.5) {
        stepDivider_ = 2;
        substepLikelihood_ = (val - 0.25) / 0.25 * 100;
    } else if (val > 0.5 and val <= 0.75) {
        stepDivider_ = 4;
        substepLikelihood_ = (val - 0.5) / 0.25 * 100;
    } else if (val > 0.75 and val <= 1.0) {
        stepDivider_ = 8;
        substepLikelihood_ = (val - 0.75) / 0.25 * 100;
    }
}

void WiSequencer::setRandomSequence(bool rand)
{
    if (randomSequence_ != rand) {
        std::uniform_int_distribution<> randomNote(0, numGroups_ - 1);
        for (int i = 0; i < numGroups_; ++i)
            randomNotes_[i] = randomNote(gen_);
    }
    randomSequence_ = rand;
}

int WiSequencer::getNextStep()
{
    int incrementValue = 1;
    // if (randomSequence_) {
    //     std::uniform_int_distribution<> randomIncr(0, numNotes_);
    //     incrementValue = randomIncr(gen_);
    // }

    int candidateStep = (currentStep_ + incrementValue) % numNotes_;
    while (not notes_[candidateStep].active) {
        candidateStep = (candidateStep + 1) % numNotes_;
    }

    return candidateStep;
}

void WiSequencer::includeFractionOfDefaultSequence(float frac)
{
    if (assignedSequence_) return;

    int numGroups = 4;
    numNotes_ = 6;

    int n = (int)lroundf(frac * numGroups + 0.4);   // simpler intent: map frac to 0..4
    if(n < 1) n = 1;
    if(n > numGroups) n = numGroups;

    // start all off
    for(int i = 0; i < numNotes_; ++i) notes_[i].active = false;

    // your specific pattern
    std::vector<int> notes = {1,13,8, 4};
    if(n >= 1) {
        notes_[0].active = true;
        notes_[0].note = randomSequence_ ? notes[randomNotes_[0] % 1] : notes[0];
    }

    if(n >= 2) {
        notes_[3].active = true;
        notes_[3].note = randomSequence_ ? notes[randomNotes_[1] % 2] : notes[1];
    }

    if(n >= 3) {
        notes_[2].active = true;
        notes_[2].note = randomSequence_ ? notes[randomNotes_[2] % 3] : notes[2];
        notes_[5].active = true;
        notes_[5].note = randomSequence_ ? (notes[randomNotes_[2] % 3] + 12) : 12 + notes[2];
    }

    if(n == 4) {
        notes_[1].active = true;
        notes_[1].note = randomSequence_ ? notes[randomNotes_[3] % 4] : notes[3];
        notes_[4].active = true;
        notes_[4].note = randomSequence_ ? (notes[randomNotes_[3] % 4] + 12) : 12 + notes[3];
    }

    // A better way is to choose number of octaves as well
}

void WiSequencer::setPeriod(float period)
{
    period_ = period;

    if(fs_ <= 0)
        return;

    // period_ is seconds per beat (quarter note)
    float secondsPerClock = period_ / 6.0f;
    midiClockSamplesPerTick_ = secondsPerClock * fs_;

    if(midiClockSamplesPerTick_ < 1)
        midiClockSamplesPerTick_ = 1;
}

void WiSequencer::startAssignCountdown()
{
    assignCountdown_ = 3 * fs_;
    notes_.clear();
    numNotes_ = 0;
}

void WiSequencer::freezeSequenceChanged(bool freezeSequence)
{
    if (freezeSequence != freezeSequence_){//){ and freezeChangeCountdown_ == 0) {
        //freezeChangeCountdown_ = fs_ * 2;
        freezeSequence_ = freezeSequence;
        priorityLedCb_(freezeSequence_ ? 1 : 0.5);
    }
}

void WiSequencer::assignNote(int note)
{
    if (note == -39) { // The note C-2 will be treated as a sequencer tick. Potential bug: If a low note is played in a midi keyboard, midiClock will be true!
        tick();
#ifndef USE_NO_MIDI
        rt_printf("Note outside range. Ticking sequencer\n");
#endif
        return;
    }

    if (note <= 0) { // IDE!!!! Her kan man legge in et "tomt" steg- slik at man kan få pauser i sequenceren
#ifndef USE_NO_MIDI
        rt_printf("note outside range\n");
#endif
        return;
    }

    if (assignMode_ and not freezeSequence_) {
        if (assignCountdown_ == 0)
            startAssignCountdown();

        assignCountdown_ = 1 * fs_; // Prolong countdown
        assignedSequence_ = true;
        notes_.push_back({note, true});
        numNotes_ = notes_.size();

        // // keep step index valid if notes were empty before
        // if (numNotes_ == 1) {
        //     currentStep_ = 0;
        //     stepCountdown_ = 1;
        // }

        // Step 0: Erase all notes when entering assign mode
        // Step 1: Erase all notes. Assign and play first note and start 6 second timer
        // Step 2: Assign and play all notes before timer ends
        // Step 3: resume sequencer when timer ends

        // Step 4: Call includeFractionOfDefaultSequence when leaving assign mode
    }
}

void WiSequencer::produceSubsteps()
{
    unsigned int newSamplesPerSubStep = samplesPerStep_ / stepDivider_;
    if (newSamplesPerSubStep == 0)
        newSamplesPerSubStep = 1;

    if (stepCountdown_ < samplesPerStep_ and
        stepCountdown_ > 0 and
        stepCountdown_ % newSamplesPerSubStep == 0)
    {
        if (substepChance_(gen_) < substepLikelihood_)
            ledCb_(0.01);
    }
}

