#include "WiSequencer.hpp"
#include <cmath>
#include <chrono>

WiSequencer::WiSequencer()
    : tickRequested_()
    , rd_()
    , gen_(rd_())
{
}

void WiSequencer::init(float fs, std::function<void(float)> ledCb)
{
    fs_ = fs;

    notes_.assign(6, NoteInfo{0, true});
    numNotes_ = notes_.size();
    numGroups_ = 4;

    randomNotes_.assign(numGroups_, 0);

    // Initialize timing
    currentStep_ = 0;
    stepCountdown_ = 1;
    samplesPerStep_ = 1;

    ledCb_ = ledCb;
}

void WiSequencer::process()
{
    if(!fs_)
        return;

    if (!ledCb_) {
#ifndef USE_NO_MIDI
        rt_printf("led cb not set");
#endif
        return;
    }

    if(button_ && button_ != prevButton_)
    {
        printf("Changing assign mode\n");
        assignMode_ = !assignMode_;
        if(assignMode_)
            ledCb_(1);
        else {
            ledCb_(0.1f);
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

    if (midiClock_) {
        if (tickRequested_) {
            currentStep_ = getNextStep();
            ledCb_(0.01);
            tickRequested_ = false;
        }
    } else {
        int newSamplesPerStep = (int)lroundf(period_ * fs_);
        if(newSamplesPerStep < 1)
            newSamplesPerStep = 1;
        samplesPerStep_ = newSamplesPerStep;

        if(--stepCountdown_ <= 0)
        {
            stepCountdown_ = samplesPerStep_;
            currentStep_ = getNextStep();
            ledCb_(0.01);
        }
    }

    currentNote_ = notes_[currentStep_].note;
}

void WiSequencer::tick()
{
    midiClock_ = true;
    tickRequested_ = true;
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
    std::vector<int> notes = {0,12,7, 3};
    if(n >= 1) {
        notes_[0].active = true;
        notes_[0].note = randomSequence_ ? notes[randomNotes_[0] % 1] : 0;
    }

    if(n >= 2) {
        notes_[3].active = true;
        notes_[3].note = randomSequence_ ? notes[randomNotes_[1] % 2] : 12;
    }

    if(n >= 3) {
        notes_[2].active = true;
        notes_[2].note = randomSequence_ ? notes[randomNotes_[2] % 3] : 7;
        notes_[5].active = true;
        notes_[5].note = randomSequence_ ? (notes[randomNotes_[2] % 3] + 12) : 12 + 7;
    }

    if(n == 4) {
        notes_[1].active = true;
        notes_[1].note = randomSequence_ ? notes[randomNotes_[3] % 4] : 3;
        notes_[4].active = true;
        notes_[4].note = randomSequence_ ? (notes[randomNotes_[3] % 4] + 12) : 12 + 3;
    }

    // A better way is to choose number of octaves as well
}

void WiSequencer::setPeriod(float period)
{
    period_ = period;
}

void WiSequencer::startAssignCountdown()
{
    assignCountdown_ = 3 * fs_;
    notes_.clear();
    numNotes_ = 0;
}

void WiSequencer::freezeSequenceChanged()
{
    
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

    if (assignMode_) {
        if (assignCountdown_ == 0)
            startAssignCountdown();

        assignCountdown_ = 1 * fs_; // Prolong countdown
        assignedSequence_ = true;
        notes_.push_back({note, true});
        numNotes_ = notes_.size();

        // keep step index valid if notes were empty before
        if (numNotes_ == 1) {
            currentStep_ = 0;
            stepCountdown_ = 1;
        }

        // Step 0: Erase all notes when entering assign mode
        // Step 1: Erase all notes. Assign and play first note and start 6 second timer
        // Step 2: Assign and play all notes before timer ends
        // Step 3: resume sequencer when timer ends

        // Step 4: Call includeFractionOfDefaultSequence when leaving assign mode
    }
}
