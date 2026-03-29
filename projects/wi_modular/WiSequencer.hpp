#ifndef WI_SEQUENCER_HPP
#define WI_SEQUENCER_HPP

#include <atomic>
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
    void init(float fs, std::function<void(float)> = nullptr);
    void tick();
    void setPeriod(float period);
    void setButton(bool button) { button_ = button; };
    void setRandomSequence(bool rand);
    void assignNote(int note);
    void includeFractionOfDefaultSequence(float frac);
    void freezeSequenceChanged();


    bool midiClock_ = false;
    int currentNote_ = 0;
    float lfo_ = 0.0f;

private:
    void startAssignCountdown();
    int getNextStep();

    bool assignMode_ = false;
    std::vector<NoteInfo> notes_;
    float fs_;
    float period_ = 1.0f; // 1 second default
    int currentStep_;
    int samplesPerStep_;
    int stepCountdown_;
    int numNotes_;
    int numGroups_;
    int assignCountdown_ = 0;
    bool button_ = false;
    bool prevButton_ = false;
    bool assignedSequence_ = false;
    bool randomSequence_ = false;
    std::vector<int> randomNotes_;
    std::atomic_bool tickRequested_;
    std::function<void(float)> ledCb_;
    std::random_device rd_;
    std::mt19937 gen_;
};


#endif