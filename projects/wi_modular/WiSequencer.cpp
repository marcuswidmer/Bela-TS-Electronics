#include "WiSequencer.hpp"
#include <cmath>
#include <chrono>

#ifndef USE_NO_MIDI
#include <libraries/Midi/Midi.h>
void midiMessageCallback(MidiChannelMessage message, void* arg)
{
    auto * wiSequencer = static_cast<WiSequencer *>(arg);

	if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
		if (message.getType() == kmmNoteOn) {
            int note = message.getDataByte(0);
			rt_printf("note on: %d. Vel: %d\n", note, message.getDataByte(1));
            wiSequencer->assignNote(note);
        }


        // else if (message.getType() == kmmNoteOff) {
		// 	rt_printf("note off: %d\n", message.getDataByte(0));
		// }

	}
    else if (message.getType() == kmmControlChange) {
        rt_printf("control change\n");
    }

    // Your system reports MIDI clock as type == 7
    constexpr int kMidiClockType = 7;

    // MIDI clock is 24 pulses per quarter note (PPQN)
    constexpr int kPpqn = 24;

    using Clock = std::chrono::steady_clock;
    static int clockCount = 0;
    static Clock::time_point lastBeatTp{};
    static bool haveLastBeat = false;

    if((int)message.getType() == kMidiClockType)
    {
        if(++clockCount >= kPpqn)
        {
            clockCount = 0;

            auto now = Clock::now();
            if(haveLastBeat)
            {
                const std::chrono::duration<double> dt = now - lastBeatTp;
                const double bpm = (dt.count() > 0.0) ? (60.0 / dt.count()) : 0.0;
                rt_printf("BEAT  BPM: %.2f\n", bpm);
            }
            else
            {
                rt_printf("BEAT\n");
                haveLastBeat = true;
            }

            lastBeatTp = now;
        }
        return;
    }
}
#endif

WiSequencer::WiSequencer()
{
#ifndef USE_NO_MIDI
    midi_ = new Midi();
#endif
}

void WiSequencer::init(float fs)
{
    fs_ = fs;

    notes_.clear();
    notes_.push_back(12);
    notes_.push_back(12 + 7);      // Fifth
    notes_.push_back(12 + 12);     // Octave
    notes_.push_back(12 + 12 + 7); // Octave + fifth
    numNotes_ = notes_.size();

    // Initialize timing
    currentStep_ = 0;
    stepCountdown_ = 1;
    lfoCycleCountdown_ = 1;
    samplesPerStep_ = 1;
    samplesPerLfoCycle_ = 1;

#ifndef USE_NO_MIDI
    midi_->readFrom(midiPort_);
	midi_->enableParser(true);
	midi_->getParser()->setCallback(midiMessageCallback, (void*) this);
#endif
}

void WiSequencer::reinit()
{
    notes_.clear();
    notes_.push_back(12);
    notes_.push_back(12 + 7);      // Fifth
    notes_.push_back(12 + 12);     // Octave
    notes_.push_back(12 + 12 + 7); // Octave + fifth
    numNotes_ = notes_.size();

    // Initialize timing
    currentStep_ = 0;
    stepCountdown_ = 1;
    lfoCycleCountdown_ = 1;
    samplesPerStep_ = 1;
    samplesPerLfoCycle_ = 1;

#ifndef USE_NO_MIDI
	delete midi_;
    midi_ = new Midi();
    midi_->readFrom(midiPort_);
    midi_->enableParser(true);
    midi_->getParser()->setCallback(midiMessageCallback, (void*)this);
#endif
}

void WiSequencer::process()
{
    if(!fs_)
        return;

    if(button_ && button_ != prevButton_)
    {
        assignMode_ = !assignMode_;
        if(assignMode_)
            trigger(1);
        else
            trigger(0.1f);

        startEraseCountdown();
    }
    prevButton_ = button_;

    if(assignMode_)
        processAssignMode();
    else
        processNormalMode();
}

void WiSequencer::processNormalMode()
{
    if(numNotes_ == 0) {
        processLed();
        return;
    }

    int newSamplesPerStep = (int)lroundf(period_ * fs_);
    if(newSamplesPerStep < 1)
        newSamplesPerStep = 1;
    samplesPerStep_ = newSamplesPerStep;

    if(--stepCountdown_ <= 0)
    {
        stepCountdown_ = samplesPerStep_;
        currentStep_ = (currentStep_ + 1) % numNotes_;
        trigger();
    }

    currentNote_ = notes_[currentStep_];
    processLfo();
    processLed();
}

void WiSequencer::processLfo()
{
    int newSamplesPerCycle = (int)lroundf(lfoPeriod_ * fs_);
    if (newSamplesPerCycle < 1)
        newSamplesPerCycle = 1;
    samplesPerLfoCycle_ = newSamplesPerCycle;

    lfo_ = lfoAmp_ * (sin(lfoCycleCountdown_ / samplesPerLfoCycle_ * 2 * M_PI) + 1.0f) / 2.0f;

    lfoCycleCountdown_--;
    if (lfoCycleCountdown_ <= 0)
        lfoCycleCountdown_ = samplesPerLfoCycle_;
}

void WiSequencer::processAssignMode()
{
    if(button_ && eraseCountdown_ > 0)
        eraseCountdown_--;

    if(eraseCountdown_ == 1)
    {
        notes_.clear();
        numNotes_ = 0;
        currentStep_ = 0;
        stepCountdown_ = 1;
    }

    processLed();
}

void WiSequencer::setPeriod(float period)
{
    period_ = period;
}

void WiSequencer::setLfoPeriod(float period)
{
    lfoPeriod_ = period;
}

void WiSequencer::setLfoAmplitude(float amp)
{
    lfoAmp_ = amp;
}

void WiSequencer::processLed()
{
    if(triggerCountdown_ > 0) {
        trigger_ = true;
        triggerCountdown_--;
    } else {
        trigger_ = false;
    }
}

void WiSequencer::trigger(float length)
{
    triggerCountdown_ = (int)lroundf(length * fs_);
}

void WiSequencer::assignNote(int note)
{
    if(!assignMode_)
        return;

    trigger();
    currentNote_ = note;

    notes_.push_back(note);
    numNotes_ = notes_.size();

    // keep step index valid if notes were empty before
    if(numNotes_ == 1) {
        currentStep_ = 0;
        stepCountdown_ = 1;
    }
}