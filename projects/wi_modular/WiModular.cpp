#include "WiModular.hpp"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <chrono>

static inline float midiToAnalogOut(int midiNote, int zeroNote = 12)
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
{
}

void WiModular::init(float analogIOSampleRate, int audioSampleRate)
{
    analogIOSampleRate_ = analogIOSampleRate;
    wiSequencer.init(analogIOSampleRate_);
    sampler.init(audioSampleRate, [this](float length){
        triggerLed(length);
    });
}

void WiModular::reinitSampler(int audioSampleRate)
{
    sampler.reinit();
}

void WiModular::reinitSequencer(float analogSampleRate)
{
    wiSequencer.reinit();
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

        float pitchBias = analogIO.pot2;
        int pitchBiasIdx = pitchBias * numPitchBiases;
        if (pitchBiasIdx >= numPitchBiases)
            pitchBiasIdx = numPitchBiases - 1;

        bool buttonPressedNow = false;
        if (analogIO.button) {
            if (not buttonPressed_) {
                buttonPressedNow = true;
                buttonPressed_ = true;
            }
        } else {
            buttonPressed_ = false;
        }

        if (sampleCntr_ > (periodSamples + randomPeriodShift_) or buttonPressedNow) {
            std::uniform_int_distribution<> randomOctave(0, numOctaves);
            analogIO.cvOut0 = midiToAnalogOut(pitchBiases[pitchBiasIdx], 0) + randomOctave(gen_) / 5.0f;
            analogIO.cvOut1 = 1.0f;
            analogIO.led = true;
            std::uniform_int_distribution<> randomShiftDist(-3 * periodSamples, periodSamples);
            float randomness = analogIO.pot2;
            randomPeriodShift_ = randomness * randomShiftDist(gen_);

            sampleCntr_ = 0;
            triggerCntr_ = 0.01 * analogIOSampleRate_; // 10ms trigger
        }

        sampleCntr_ += 1;

        if (triggerCntr_ > 0)
            triggerCntr_--;
        else {
            analogIO.cvOut1 = 0.0f;
            analogIO.led = false;
        }

    } else if (analogIO.selector == PgmTestBench) { // Ide til nytt program: "Continuous sequencer". Pot0 stepper manualt gjennom steppene. Med litt glide i mellom.
        int octave = analogIO.pot0 * octaveScalingFactor_; // Some fine tuning is needed/done here
        analogIO.cvOut0 = octave / 5.0f; // and here
        analogIO.cvOut1 = analogIO.button;

    } else if (analogIO.selector == PgmSequencer) {
        float speed = analogIO.pot0;
        speedSmoothed_ += 0.0001f * (speed - speedSmoothed_);
        float f = 0.4f + 14.0f * speedSmoothed_;
        wiSequencer.setPeriod(1.0f / f);
        wiSequencer.setButton(analogIO.button);

        float lfoSpeed = analogIO.pot1;
        lfoSpeedSmoothed_ += 0.0001f * (lfoSpeed - lfoSpeedSmoothed_);
        float lfoFreq = 0.1f + 20.0f * lfoSpeedSmoothed_;
        wiSequencer.setLfoPeriod(1.0f / lfoFreq);

        float lfoAmp = analogIO.pot2;
        wiSequencer.setLfoAmplitude(lfoAmp);
        wiSequencer.process();
        analogIO.led = wiSequencer.trigger_;
        analogIO.cvOut0 = midiToAnalogOut(wiSequencer.currentNote_);
        analogIO.cvOut1 = wiSequencer.trigger_; // 10ms trigger
        analogIO.cvOut2 = wiSequencer.lfo_;

    } else if (analogIO.selector == PgmSampler) {
        sampler.setProgram(analogIO.pot0 * 5);
        sampler.setMainLevel(analogIO.pot1);
        sampler.setDroneLevel(analogIO.pot2);
    }

    processLed();
}

void WiModular::triggerLed(float length)
{
    ledCountdown_ = (int)lroundf(length * analogIOSampleRate_);
}

void WiModular::processLed()
{
    if(ledCountdown_ > 0) {
        analogIO.led = true;
        ledCountdown_--;
    } else {
        analogIO.led = false;
    }
}
void WiModular::processAudio(float * out)
{
    if (analogIO.selector == PgmSampler)
    {
        sampler.process(out);
    }
}

void WiModular::assignNote(int note)
{
    wiSequencer.assignNote(note);
}
