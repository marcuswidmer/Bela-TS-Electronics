#include "KarplusResonator.hpp"
#include <algorithm>
#include <cmath>

namespace {
float clamp(float x, float lo, float hi) { return std::max(lo, std::min(x, hi)); }
constexpr float pi = 3.14159265358979323846f;
// Semitone offsets from the root: chromatic, major, unison, minor pentatonic.
constexpr int notes[4][12] = {
    {0,1,2,3,4,5,6,7,8,9,10,11},
    {0,2,4,5,7,9,11,12,14,16,17,19},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,3,5,7,10,12,15,17,19,22,24,27}
};
}

bool KarplusResonator::setup(float rate)
{
    if(!std::isfinite(rate) || rate < 22050 || rate > 96000) return false;
    sampleRate = rate;
    smoothing = 1 - std::exp(-1 / (0.01f * sampleRate));
    pluckDecay = std::exp(-1 / (0.003f * sampleRate));
    noteSet = 0;
    sustain = false;
    noiseState = 1;
    clear();
    setPots(controls);
    master = controls[0]; excitation = controls[1]; mix = controls[5]; width = controls[7];
    damping = targetDamping;
    for(auto& string : strings) {
        string.delay = string.targetDelay;
        string.feedback = string.targetFeedback;
    }
    return true;
}

void KarplusResonator::clear()
{
    for(auto& string : strings) {
        string.line.fill(0);
        string.lowpass = 0;
        string.write = 0;
    }
    pluckEnvelope = 0;
}

void KarplusResonator::setPots(const std::array<float, 8>& pots)
{
    for(unsigned int i=0; i<controls.size(); ++i)
        controls[i] = std::isfinite(pots[i]) ? clamp(pots[i],0,1) : 0;
    const float cutoff = 500 * std::pow(40.0f, controls[3]);
    targetDamping = 1 - std::exp(-2*pi*std::min(cutoff, sampleRate*0.4f)/sampleRate);
    updateTuning();
}

void KarplusResonator::updateTuning()
{
    // A2 at center, transposed in semitone steps across A1-A3.
    const int transpose = std::lround(24*controls[4]) - 12;
    const float decaySeconds = 0.5f * std::pow(120.0f, controls[2]);
    for(unsigned int i=0; i<voices; ++i) {
        const float cents = (2.0f*i/(voices-1)-1) * 20 * controls[6];
        const float frequency = 110 * std::pow(2.0f,(notes[noteSet][i]+transpose+cents/100)/12);
        auto& string = strings[i];
        string.targetDelay = clamp(sampleRate/frequency,2,capacity-2);
        string.targetFeedback = sustain ? 0.9999f :
            std::min(0.9999f, std::pow(0.001f,string.targetDelay/(sampleRate*decaySeconds)));
    }
}

void KarplusResonator::pressButton(unsigned int button)
{
    switch(button) {
    case 0: pluckEnvelope = 0.5f; break;
    case 1: noteSet = (noteSet+1)%4; updateTuning(); break;
    case 2: sustain = !sustain; updateTuning(); break;
    case 3: clear(); break;
    }
}

void KarplusResonator::process(float input, float& left, float& right)
{
    if(!std::isfinite(input)) input = 0;
    master += smoothing*(controls[0]-master);
    excitation += smoothing*(controls[1]-excitation);
    mix += smoothing*(controls[5]-mix);
    width += smoothing*(controls[7]-width);
    damping += smoothing*(targetDamping-damping);
    noiseState ^= noiseState << 13; noiseState ^= noiseState >> 17; noiseState ^= noiseState << 5;
    const float noise = 2.0f*(noiseState / 4294967295.0f)-1;
    const float drive = (sustain ? 0 : 0.2f*excitation*clamp(input,-1,1)) + noise*pluckEnvelope;
    pluckEnvelope *= pluckDecay;
    if(pluckEnvelope < 1e-7f) pluckEnvelope = 0;
    float wetL=0, wetR=0;
    for(unsigned int i=0; i<voices; ++i) {
        auto& string = strings[i];
        string.delay += smoothing*(string.targetDelay-string.delay);
        string.feedback += smoothing*(string.targetFeedback-string.feedback);
        float read = string.write - string.delay;
        if(read<0) read += capacity;
        const unsigned int index = static_cast<unsigned int>(read);
        const float fraction = read-index;
        const float delayed = string.line[index]*(1-fraction) + string.line[(index+1)%capacity]*fraction;
        string.lowpass += damping*(delayed-string.lowpass);
        float value = drive + string.lowpass*string.feedback;
        // Preserve quiet ringing; smoothly limit only above unity, bounded by +/-4.
        const float excess = std::abs(value) - 1.0f;
        if(excess > 0)
            value = std::copysign(1.0f + excess/(1.0f + excess/3.0f), value);
        string.line[string.write] = std::abs(value)<1e-20f ? 0 : value;
        string.write = (string.write+1)%capacity;
        const float pan = width*(2.0f*i/(voices-1)-1);
        wetL += delayed*(1-pan)/voices;
        wetR += delayed*(1+pan)/voices;
    }
    left = clamp(master*((1-mix)*input+mix*wetL),-1,1);
    right = clamp(master*((1-mix)*input+mix*wetR),-1,1);
}
