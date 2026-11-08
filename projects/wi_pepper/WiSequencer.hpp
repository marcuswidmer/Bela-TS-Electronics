#ifndef WI_SEQUENCER_HPP
#define WI_SEQUENCER_HPP

#include <atomic>
#include <cstdint>
#include <random>
#include <functional>
#include <vector>

const float ledOnDuration = 0.01f; // 10 ms

struct NoteInfo {
    int note;
    bool active;
};
class WiSequencer
{
public:
    WiSequencer();
    void process();
    void init(float fs, std::function<void(float)> = nullptr, std::function<void(float)> = nullptr, std::function<void(float)> = nullptr, std::function<void(uint8_t)> = nullptr);
    void tick();
    void advanceClockTime() { ++clockSamples_; }
    void midiRealtime(uint8_t byte, bool playbackEnabled = true);
    void midiSongPosition(unsigned int position);
    void pauseMidiClockPlayback();
    bool externalMidiClock() const { return externalMidiClock_; }
    float stepPeriod() const { return externalMidiClock_ ? externalStepPeriod_ : period_; }
    void setPeriod(float period);
    void setButton(bool button) { button_ = button; };
    void setRandomSequence(bool rand);
    void setSubstepBehaviour(float val);
    void assignNote(int note);
    void includeFractionOfDefaultSequence(float frac);
    void freezeSequenceChanged(bool freezeSequence);
    bool sequenceFrozen() const { return freezeSequence_; }


    bool midiClock_ = false;
    int currentNote_ = 0;
    float lfo_ = 0.0f;

private:
    void startAssignCountdown();
    int getNextStep();
    void produceSubsteps();
    void produceMidiSubsteps();
    void useExternalMidiClock();

    bool assignMode_ = true;
    std::vector<NoteInfo> notes_;
    float fs_;
    float period_ = 1.0f; // 1 second default
    int currentStep_;
    int samplesPerStep_;
    int stepCountdown_;
    int midiStepCountdown_;
    int numNotes_;
    int numGroups_;
    int assignCountdown_ = 0;
    int freezeChangeCountdown_ = 0;
    int stepDivider_ = 1;
    int substepLikelihood_ = 0;
    std::atomic_bool freezeSequence_;
    bool button_ = false;
    bool prevButton_ = false;
    bool assignedSequence_ = false;
    bool firstAssignedStepPending_ = false;
    bool randomSequence_ = false;
    float midiClockPhase_ = 0.0f;
    float midiClockSamplesPerTick_ = 1.0f;
    bool resyncRequested_ = false;
    bool externalMidiClock_ = false;
    bool midiTransportRunning_ = false;
    bool clockMeasurementValid_ = false;
    bool positionPending_ = false;
    unsigned int pendingClockTicks_ = 0, clocksUntilStep_ = 0;
    unsigned int nextStepPosition_ = 0, clockMeasurementTicks_ = 0;
    uint64_t clockSamples_ = 0, clockMeasurementStart_ = 0;
    float externalStepPeriod_ = 0.125f; // Initial estimate: a sixteenth at 120 BPM.
    std::uniform_int_distribution<> substepChance_;
    std::vector<int> randomNotes_;
    std::atomic_bool tickRequested_;
    std::function<void(float)> ledCb_;
    std::function<void(float)> priorityLedCb_;
    std::function<void(float)> syncTriggerCb_;
    std::function<void(uint8_t)> midiByteCb_;
    std::random_device rd_;
    std::mt19937 gen_;
};


#endif
