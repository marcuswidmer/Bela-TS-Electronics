#ifndef WI_LFO_HPP
#define WI_LFO_HPP

class WiLFO
{
public:
    WiLFO();
    void init(float fs);
    void process();
    void setPeriod(float period);
    void setAmplitude(float amp);
    float lfo_ = 0.0f;

private:
    float amp_ = 0.0f;
    float period_ = 0.0f;
    float fs_ = 0.0f;
    float samplesPerCycle_ = 1.0f;
    int cycleCountdown_ = 1;
};


#endif