#include "WiModular.hpp"
#include "WiLFO.hpp"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <chrono>
#include <fstream>

#ifndef USE_NO_MIDI
#include <libraries/Midi/Midi.h>
void midiMessageCallback(MidiChannelMessage message, void* arg)
{
    // rt_printf("MIDI: type=%d status=0x%02X ch=%d d0=%d(0x%02X) d1=%d(0x%02X)\n",
    //         (int)message.getType(),
    //         (unsigned)message.getStatusByte(),
    //         (int)message.getChannel(),
    //         (int)message.getDataByte(0), (unsigned)message.getDataByte(0),
    //         (int)message.getDataByte(1), (unsigned)message.getDataByte(1));
    auto * wiModular = static_cast<WiModular *>(arg);

	if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
        int note = message.getDataByte(0) - wiModular->getMidiNoteOffset();
        int vel = message.getDataByte(1);
		if (message.getType() == kmmNoteOn) {
			rt_printf("note on: %d. Vel: %d\n", note, vel);
            if (wiModular->analogIO.selector == PgmSequencer)
                wiModular->assignNote(note);

            if (wiModular->analogIO.selector == PgmSampler)
                wiModular->samplerPlayNewVoice(note, vel / 128.0f);

        } else if (message.getType() == kmmNoteOff) {
            if (wiModular->analogIO.selector == PgmSampler)
		        wiModular->samplerReleaseVoice(note);
		}

	} else if (message.getType() == kmmControlChange) {
        rt_printf("control change %d %d\n", message.getDataByte(0), message.getDataByte(1));
        int ctrlType = message.getDataByte(0);
        int ctrlVal = message.getDataByte(1);
        wiModular->samplerSetDroneVelocity(ctrlType, ctrlVal);
    }

    // Your system reports MIDI clock as type == 7
    constexpr int kMidiClockType = 7;

    // MIDI clock is 24 pulses per quarter note (PPQN)
    constexpr int kPpqn = 6;

    using Clock = std::chrono::steady_clock;
    static int clockCount = 0;
    static Clock::time_point lastBeatTp{};
    static bool haveLastBeat = false;

    if((int)message.getType() == kMidiClockType)
    {
        if (message.getChannel() == 10)
            clockCount = 0;

        if(++clockCount >= kPpqn)
        {
            //wiModular->tickSequencer();
            clockCount = 0;

            // auto now = Clock::now();
            // if(haveLastBeat)
            // {
            //     const std::chrono::duration<double> dt = now - lastBeatTp;
            //     const double bpm = (dt.count() > 0.0) ? (60.0 / dt.count()) : 0.0;
            //     //rt_printf("BEAT  BPM: %.2f\n", bpm);
            //     //wiModular->tickSequencer();
            // }
            // else
            // {
            //     //rt_printf("BEAT\n");
            //     haveLastBeat = true;
            // }

            // lastBeatTp = now;
        }
        return;
    }
    //     auto * sampler = static_cast<Sampler *>(arg);
	// if (message.getType() == kmmNoteOn || (message.getType() == kmmNoteOff)) {
	// 	if (message.getType() == kmmNoteOn) {
    //         sampler->playNewVoice(message.getDataByte(0) - sampler->getMidiNoteOffset(), message.getDataByte(1) / 128.0f);
	// 		rt_printf("note on: %d. Vel: %d\n", message.getDataByte(0),message.getDataByte(1));

    //     } else if (message.getType() == kmmNoteOff) {
    //         sampler->releaseVoice(message.getDataByte(0) - sampler->getMidiNoteOffset());
	// 		rt_printf("note off: %d\n", message.getDataByte(0));
	// 	}

	// } else if (message.getType() == kmmControlChange) {
    //     rt_printf("control change\n");
    // }
}
#endif

static inline float midiToAnalogOut(int midiNote, int zeroNote = 0)
{
    float volts = (midiNote - zeroNote) / 12.0f;

    if(volts < 0.0f) volts = 0.0f;
    if(volts > 5.0f) volts = 5.0f;

    return volts / 5.0f;
}

WiModular::WiModular()
    : rd_()
    , gen_(rd_())
    , wiSequencer()
    , sampler()
    , wiLFO()
{
}

void WiModular::init(float analogIOSampleRate, int audioSampleRate)
{
    analogIOSampleRate_ = analogIOSampleRate;
    auto triggerLedCb = [this](float length){
        triggerLed(length, false);
    };

    auto triggerPriorityLedCb = [this](float length){
        triggerLed(length, true);
    };
    wiSequencer.init(analogIOSampleRate_, triggerLedCb, triggerPriorityLedCb);
    wiLFO.init(analogIOSampleRate_);

    sampler.init(audioSampleRate, triggerLedCb);

#ifndef USE_NO_MIDI
    midi_ = new Midi();
    midi_->readFrom(midiPort_);
	midi_->enableParser(true);
	midi_->getParser()->setCallback(midiMessageCallback, (void*) this);
#endif

}

void WiModular::process()
{
    if (!analogIOSampleRate_) {
        printf("analogIOSampleRate is 0!");
        abort();
    }

    if (analogIO.selector == PgmRandomOctave) {
        float speed = analogIO.pot0;
        speedSmoothed_ += 0.0001f * (speed - speedSmoothed_);
        float f = 0.4f + 14.0f * speedSmoothed_;
        float periodSec = 1.0f / f;
        int periodSamples = periodSec * analogIOSampleRate_;
        int numOctaves = analogIO.pot1 * octaveScalingFactor_; // V/OCT standard. 5 volts max

        float pitchBias = 0;//analogIO.pot2;
        int pitchBiasIdx = pitchBias * numPitchBiases;
        if (pitchBiasIdx >= numPitchBiases)
            pitchBiasIdx = numPitchBiases - 1;

        if (analogIO.button) {
            if (not buttonPressed_) {
                freeze_ = not freeze_;
                buttonPressed_ = true;
            }
        } else {
            buttonPressed_ = false;
        }
        analogIO.cvOut3 = freeze_ ? 1.0 : 0.0;

        if (sampleCntr_ > (periodSamples + randomPeriodShift_)) {
            std::uniform_int_distribution<> randomOctave(0, numOctaves);
            analogIO.cvOut0 = midiToAnalogOut(pitchBiases[pitchBiasIdx], 0) + randomOctave(gen_) / 5.0f;
            triggerLed(0.01);
            std::uniform_int_distribution<> randomShiftDist(-3 * periodSamples, periodSamples);
            float randomness = analogIO.pot2;
            randomPeriodShift_ = randomness * randomShiftDist(gen_);

            sampleCntr_ = 0;
        }

        sampleCntr_ += 1;

    } else if (analogIO.selector == PgmOctaveSelector) { // Ide til nytt program: "Continuous sequencer". Pot0 stepper manualt gjennom steppene. Med litt glide i mellom.
        float cvIn = analogIO.pot1;
        octFloatSmoothed_ += 0.001f * (cvIn - octFloatSmoothed_);
        int octave = octFloatSmoothed_ * octaveScalingFactor_; // Some fine tuning is needed/done here
        octaveSmoothed_ += 0.0005f * (octave - octaveSmoothed_);
        analogIO.cvOut0 = octaveSmoothed_ / 5.0f; // and here

        if (analogIO.button) {
            if (not buttonPressed_) {
                freeze_ = not freeze_;
                buttonPressed_ = true;
            }
        } else {
            buttonPressed_ = false;
        }
        analogIO.cvOut3 = freeze_ ? 1.0 : 0.0;

    } else if (analogIO.selector == PgmSequencer) {
        float speed = analogIO.pot0;
        speedSmoothed_ += 0.0001f * (speed - speedSmoothed_);
        float f = 0.4f + 14.0f * speedSmoothed_;
        wiSequencer.setPeriod(1.0f / f);
        wiSequencer.setButton(analogIO.button);
        wiSequencer.setRandomSequence(analogIO.pot2 > 0.5);
        wiSequencer.freezeSequenceChanged(analogIO.pot2 > 0.5);
        wiSequencer.includeFractionOfDefaultSequence(analogIO.pot1);
        wiSequencer.process();
        analogIO.cvOut0 = midiToAnalogOut(wiSequencer.currentNote_);
        analogIO.cvOut2 = wiSequencer.lfo_;

    } else if (analogIO.selector == PgmSampler) {
        sampler.setProgram(analogIO.pot0 * 4.0f);
        sampler.setMainLevel(analogIO.pot1);
        sampler.setDroneLevel(analogIO.pot2);
    }

    //processLFO();
    processLedAndTrigger();
}

void WiModular::triggerLed(float length, bool priority)
{
    if (priority)
        ledPriorityCountdown_ = (int)lroundf(length * analogIOSampleRate_);
    else
        ledCountdown_ = (int)lroundf(length * analogIOSampleRate_);
}

void WiModular::setMidiClock(bool val)
{
    wiSequencer.midiClock_ = val;
}

void WiModular::processLFO()
{
    float lfoSpeed = 0.05;
    float lfoFreq = 0.1f + 20.0f * lfoSpeed;
    wiLFO.setPeriod(1.0f / lfoFreq);

    float lfoAmp = (analogIO.pot2 - 0.036 /*this is the min val for pot */) / 4 ;
    lfoAmpSmoothed_ += 0.0001f * (lfoAmp - lfoAmpSmoothed_);
    wiLFO.setAmplitude(lfoAmpSmoothed_);
    wiLFO.process();
    analogIO.cvOut2 = wiLFO.lfo_;
}

void WiModular::processLedAndTrigger()
{
    if(ledCountdown_ > 0 or ledPriorityCountdown_ > 0) {
        analogIO.led = true;
    } else {
        analogIO.led = false;
    }

    analogIO.cvOut1 = ledCountdown_ > 0;

//    {
//         using Clock = std::chrono::steady_clock;

//         static bool prevCvOut1 = false;
//         static unsigned long risingEdgeCount = 0;
//         static Clock::time_point startTp = Clock::now();   // program start reference
//         static Clock::time_point lastEdgeTp{};
//         static bool haveLastEdge = false;
//         static bool fileInitialized = false;

//         const bool currCvOut1 = analogIO.cvOut1 > 0.5f;

//         if (currCvOut1 && !prevCvOut1) {
//             // First time: clear file and write a header
//             if (!fileInitialized) {
//                 std::ofstream ofs("cvOut1_log.txt",
//                                   std::ios::out | std::ios::trunc);
//                 if (ofs.is_open()) {
//                     ofs << "# cvOut1 rising-edge log\n";
//                     ofs << "# columns: index  elapsed_ms  elapsed_s  delta_ms\n";
//                     ofs.close();
//                 }
//                 fileInitialized = true;
//             }

//             const auto now = Clock::now();
//             const auto elapsedMs =
//                 std::chrono::duration_cast<std::chrono::milliseconds>(
//                     now - startTp).count();
//             const double elapsedS = elapsedMs / 1000.0;

//             long long deltaMs = -1;  // -1 means "no previous edge"
//             if (haveLastEdge) {
//                 deltaMs = std::chrono::duration_cast<std::chrono::milliseconds>(
//                               now - lastEdgeTp).count();
//             }

//             ++risingEdgeCount;

//             // Append this edge's timing to the file
//             std::ofstream ofs("cvOut1_log.txt",
//                               std::ios::out | std::ios::app);
//             if (ofs.is_open()) {
//                 ofs << risingEdgeCount
//                     << "\t" << elapsedMs
//                     << "\t" << elapsedS
//                     << "\t" << deltaMs
//                     << "\n";
//                 ofs.close();
//             }

//             lastEdgeTp    = now;
//             haveLastEdge  = true;
//         }
//         prevCvOut1 = currCvOut1;
//     }

    if(ledCountdown_ > 0)
        ledCountdown_--;

    if(ledPriorityCountdown_ > 0)
       ledPriorityCountdown_--;

}
void WiModular::processAudio(float * out)
{
    if (analogIO.selector == PgmSampler or analogIO.selector == PgmSequencer)
    {
        sampler.process(out);
    }
}

void WiModular::assignNote(int note)
{
    wiSequencer.assignNote(note);
}

void WiModular::tickSequencer()
{
    wiSequencer.tick();
}

void WiModular::freezeSequenceChanged(bool freezeSequence)
{
    wiSequencer.freezeSequenceChanged(freezeSequence);
}

void WiModular::samplerPlayNewVoice(int note, float velocity)
{
    if (note <= 0) {
#ifndef USE_NO_MIDI
        rt_printf("note outside range\n");
#endif
        return;
    }

    sampler.playNewVoice(note, velocity);
    triggerLed(0.01);
    analogIO.cvOut0 = midiToAnalogOut(note);
    //rt_printf("Note: %d. Cv is: %f\n", note, analogIO.cvOut0);
}

void WiModular::samplerSetProgramFromSequencer(int pgm)
{
    sampler.setProgram(pgm, true);
    sampler.setMainLevel(1.0f);
    sampler.setDroneLevel(1.0f);
}

void WiModular::samplerSetDroneVelocity(int ctrlType, int ctrlVal)
{
    if (ctrlType == 74)
        sampler.setDroneVoiceVelocity(0, ctrlVal / 127.0f);

    if (ctrlType == 71)
        sampler.setDroneVoiceVelocity(1, ctrlVal / 127.0f);
}

void WiModular::samplerReleaseVoice(int note)
{
    sampler.releaseVoice(note);
}
