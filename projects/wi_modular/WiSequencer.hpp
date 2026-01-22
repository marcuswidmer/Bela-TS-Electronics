#ifndef WI_SEQUENCER_HPP
#define WI_SEQUENCER_HPP

#include <random>

class Midi;

const float ledOnDuration = 0.01f; // 10 ms

class WiSequencer
{
public:
    WiSequencer();
    void process();
    void init(float fs);
    void reinit();
    void setPeriod(float period);
    void setLfoPeriod(float period);
    void setLfoAmplitude(float amp);
    void setButton(bool button) { button_ = button; };
    void assignNote(int note);
    void trigger(float length = ledOnDuration);

    bool trigger_ = false;
    int currentNote_ = 0;
    float lfo_ = 0.0f;

private:
    void processLed();
    void processLfo();
    void processNormalMode();
    void processAssignMode();
    void startEraseCountdown() { eraseCountdown_ = 1 * fs_; };

    bool assignMode_;
    std::vector<int> notes_;
    float fs_;
    float period_ = 1.0f; // 1 second default
    float lfoPeriod_ = 1.0f;
    float lfoAmp_ = 0.0f;
    int currentStep_;
    int samplesPerStep_;
    float samplesPerLfoCycle_;
    int stepCountdown_;
    int lfoCycleCountdown_;
    int numNotes_;
    int triggerCountdown_ = 0;
    int eraseCountdown_ = 0;
    bool button_ = false;
    bool prevButton_ = false;
    Midi * midi_;
    const char* midiPort_ = "hw:1,0,0";
    static const int midiNoteOffset_ = 51; //51 is standard
};


#endif