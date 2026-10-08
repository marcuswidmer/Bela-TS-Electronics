#include "JunoSynth.hpp"
#include <algorithm>
#include <cmath>

namespace {
constexpr float pi = 3.14159265358979323846f;
float clamp(float x, float lo, float hi) { return std::max(lo, std::min(hi, x)); }
float triangle(float phase) { return 4 * std::abs(phase - .5f) - 1; }
// Polynomial correction at oscillator discontinuities reduces aliasing.
float blep(float phase, float step) {
    if(phase < step) {
        const float x = phase / step;
        return x + x - x*x - 1;
    }
    if(phase > 1 - step) {
        const float x = (phase - 1) / step;
        return x*x + x + x + 1;
    }
    return 0;
}
float pulse(float phase, float step, float width) {
    float edge = phase - width;
    if(edge < 0) edge += 1;
    return (phase < width ? 1.0f : -1.0f) + blep(phase, step) - blep(edge, step);
}
}

float JunoSynth::Filter::process(float input, float g, float k)
{
    const float a = 1 / (1 + g * (g + k));
    const float v1 = a * (s1 + g * (input - s2));
    const float v2 = s2 + g * v1;
    s1 = 2 * v1 - s1;
    s2 = 2 * v2 - s2;
    // Avoid denormal slowdowns in long releases.
    if(std::abs(s1) < 1e-20f) s1 = 0;
    if(std::abs(s2) < 1e-20f) s2 = 0;
    return v2;
}

bool JunoSynth::setup(float sampleRate)
{
    if(!std::isfinite(sampleRate) || sampleRate < 22050 || sampleRate > 96000) return false;
    sampleRate_ = sampleRate;
    smoothing_ = 1 - std::exp(-1 / (.01f * sampleRate));
    hpPole_ = std::exp(-2 * pi * 25 / sampleRate);
    for(unsigned int note = 0; note < frequencies_.size(); ++note)
        frequencies_[note] = 440 * std::exp2((static_cast<float>(note) - 69) / 12);
    waveform_ = 0;
    chorusMode_ = 1;
    lfoPhase_ = chorusPhase_ = 0;
    age_ = 0;
    noise_ = 1;
    panic();
    setPots(pots_);
    cutoff_ = targetCutoff_;
    resonance_ = pots_[2];
    envDepth_ = pots_[3];
    volume_ = pots_[0];
    chorusMix_ = .5f;
    return true;
}

void JunoSynth::setPots(const std::array<float, 8>& pots)
{
    for(unsigned int i = 0; i < pots_.size(); ++i)
        pots_[i] = std::isfinite(pots[i]) ? clamp(pots[i], 0, 1) : 0;
    targetCutoff_ = 40 * std::pow(400.0f, pots_[1]);
    attackStep_ = 1 / (sampleRate_ * .002f * std::pow(2000.0f, pots_[4]));
    decayMultiplier_ = std::exp(-6.907755f / (sampleRate_ * .02f * std::pow(150.0f, pots_[5])));
    releaseMultiplier_ = std::exp(-6.907755f / (sampleRate_ * .02f * std::pow(400.0f, pots_[7])));
}

void JunoSynth::panic()
{
    voices_ = {};
    sustain_.fill(false);
    bend_.fill(1);
    modulation_.fill(0);
    chorus_.fill(0);
    chorusWrite_ = controlCounter_ = 0;
    hpInput_ = hpOutput_ = 0;
}

void JunoSynth::pressButton(unsigned int button)
{
    if(button == 1) waveform_ = (waveform_ + 1) % 3;
    if(button == 2) chorusMode_ = (chorusMode_ + 1) % 3;
    if(button == 3) panic();
}

void JunoSynth::updatePitch(Voice& voice)
{
    voice.increment = std::min(.45f, frequencies_[voice.note] * bend_[voice.channel] / sampleRate_);
}

void JunoSynth::noteOn(unsigned int note, unsigned int velocity, unsigned int channel)
{
    if(note > 127 || velocity > 127 || channel > 15) return;
    if(!velocity) { noteOff(note, channel); return; }
    Voice* selected = nullptr;
    // Repeated Note On retriggers the same key; channel ownership is preserved.
    for(auto& v : voices_)
        if(v.stage != Idle && v.note == note && v.channel == channel) { selected = &v; break; }
    if(!selected)
        for(auto& v : voices_) if(v.stage == Idle) { selected = &v; break; }
    if(!selected) {
        // Prefer the quietest releasing voice, otherwise the oldest held voice.
        for(auto& v : voices_)
            if(v.stage == Release && (!selected || v.envelope < selected->envelope)) selected = &v;
    }
    if(!selected) {
        selected = &voices_[0];
        for(auto& v : voices_) if(v.age < selected->age) selected = &v;
    }
    const float tail = selected->previous;
    *selected = Voice{};
    auto& v = *selected;
    v.stage = Attack;
    v.note = note;
    v.channel = channel;
    v.age = ++age_;
    v.held = true;
    v.velocity = .25f + .75f * velocity / 127;
    v.stealTail = tail;
    v.stealSamples = static_cast<unsigned int>(sampleRate_ * .002f);
    updatePitch(v);
    controlCounter_ = 0;
}

void JunoSynth::noteOff(unsigned int note, unsigned int channel)
{
    if(note > 127 || channel > 15) return;
    for(auto& v : voices_) {
        if(v.stage == Idle || v.note != note || v.channel != channel) continue;
        v.held = false;
        if(!sustain_[channel]) v.stage = Release;
    }
}

void JunoSynth::pitchBend(unsigned int value, unsigned int channel)
{
    if(value > 16383 || channel > 15) return;
    const float normalized = (static_cast<int>(value) - 8192) / (value >= 8192 ? 8191.0f : 8192.0f);
    bend_[channel] = std::exp2(normalized * 2 / 12);
    for(auto& v : voices_) if(v.channel == channel) updatePitch(v);
}

void JunoSynth::controlChange(unsigned int controller, unsigned int value, unsigned int channel)
{
    if(controller > 127 || value > 127 || channel > 15) return;
    if(controller == 1) modulation_[channel] = value / 127.0f;
    if(controller == 64 || controller == 121) {
        sustain_[channel] = controller == 64 && value >= 64;
        if(!sustain_[channel])
            for(auto& v : voices_)
                if(v.channel == channel && !v.held && v.stage != Idle) v.stage = Release;
    }
    if(controller == 121) { modulation_[channel] = 0; pitchBend(8192, channel); }
    if(controller == 123) {
        for(auto& v : voices_) if(v.channel == channel && v.stage != Idle) {
            v.held = false;
            v.stage = Release;
        }
    }
    if(controller == 120) {
        for(auto& v : voices_) if(v.channel == channel) v = Voice{};
        sustain_[channel] = false;
        // Chorus is shared; clear its tail for immediate all-sound-off.
        chorus_.fill(0);
        hpInput_ = hpOutput_ = 0;
    }
}

unsigned int JunoSynth::activeVoices() const
{
    unsigned int count = 0;
    for(const auto& v : voices_) if(v.stage != Idle) ++count;
    return count;
}

float JunoSynth::readChorus(float delay) const
{
    float pos = static_cast<float>(chorusWrite_) - delay;
    if(pos < 0) pos += chorus_.size();
    const auto index = static_cast<unsigned int>(pos);
    const float fraction = pos - index;
    return chorus_[index] * (1 - fraction) + chorus_[(index + 1) % chorus_.size()] * fraction;
}

void JunoSynth::process(float& left, float& right)
{
    volume_ += smoothing_ * (pots_[0] - volume_);
    cutoff_ += smoothing_ * (targetCutoff_ - cutoff_);
    resonance_ += smoothing_ * (pots_[2] - resonance_);
    envDepth_ += smoothing_ * (pots_[3] - envDepth_);
    const float lfo = triangle(lfoPhase_);
    lfoPhase_ += .35f / sampleRate_;
    if(lfoPhase_ >= 1) lfoPhase_ -= 1;
    noise_ ^= noise_ << 13; noise_ ^= noise_ >> 17; noise_ ^= noise_ << 5;
    const float noise = noise_ / 2147483648.0f - 1;
    float sum = 0;
    for(auto& v : voices_) {
        if(v.stage == Idle) continue;
        switch(v.stage) {
        case Attack:
            v.envelope = std::min(1.0f, v.envelope + attackStep_);
            if(v.envelope >= 1) v.stage = Decay;
            break;
        case Decay:
            v.envelope = pots_[6] + (v.envelope - pots_[6]) * decayMultiplier_;
            if(std::abs(v.envelope - pots_[6]) < 1e-4f) v.stage = Sustain;
            break;
        case Sustain: v.envelope += smoothing_ * (pots_[6] - v.envelope); break;
        case Release:
            v.envelope *= releaseMultiplier_;
            if(v.envelope < 1e-5f) { v = Voice{}; continue; }
            break;
        case Idle: break;
        }
        if(controlCounter_ == 0) {
            // Half keyboard tracking, up to five octaves of envelope sweep.
            const float hz = cutoff_ * std::exp2((static_cast<float>(v.note) - 60) / 24 + 5 * envDepth_ * v.envelope);
            v.g = std::tan(pi * clamp(hz, 20, sampleRate_ * .4f) / sampleRate_);
        }
        const float saw = 2 * v.phase - 1 - blep(v.phase, v.increment);
        const float width = .5f + (.04f + .3f * modulation_[v.channel]) * lfo;
        const float square = pulse(v.phase, v.increment, width);
        const float sub = pulse(v.subPhase, v.increment * .5f, .5f);
        const float wave = waveform_ == 1 ? saw : waveform_ == 2 ? square : .5f * (saw + square);
        const float oscillator = .65f * wave + .45f * sub + .015f * noise;
        v.phase += v.increment;
        if(v.phase >= 1) v.phase -= 1;
        v.subPhase += v.increment * .5f;
        if(v.subPhase >= 1) v.subPhase -= 1;
        float filtered = v.filter1.process(oscillator, v.g, 2 - 1.75f * resonance_);
        filtered = v.filter2.process(filtered, v.g, 1.41421356f);
        float sample = filtered * v.envelope * v.velocity;
        if(v.stealSamples) {
            sample += v.stealTail * v.stealSamples / std::max(1.0f, sampleRate_ * .002f);
            --v.stealSamples;
        }
        v.previous = sample;
        sum += sample;
    }
    controlCounter_ = (controlCounter_ + 1) % 16;
    // Fixed headroom independent of voice count, followed by a DC/high-pass stage.
    sum *= .22f;
    const float highpass = sum - hpInput_ + hpPole_ * hpOutput_;
    hpInput_ = sum;
    hpOutput_ = std::abs(highpass) < 1e-20f ? 0 : highpass;
    chorus_[chorusWrite_] = hpOutput_;
    const float rate = chorusMode_ == 2 ? .83f : .47f;
    chorusPhase_ += rate / sampleRate_;
    if(chorusPhase_ >= 1) chorusPhase_ -= 1;
    const float mod = triangle(chorusPhase_);
    const float depth = chorusMode_ == 2 ? .003f : .0018f;
    const float wetL = readChorus(sampleRate_ * (.012f + depth * mod));
    const float wetR = readChorus(sampleRate_ * (.012f - depth * mod));
    chorusMix_ += smoothing_ * ((chorusMode_ ? .5f : 0) - chorusMix_);
    chorusWrite_ = (chorusWrite_ + 1) % chorus_.size();
    // Keep the dry voice centered and widen only the chorus side signal.
    const float wetMid = .5f * (wetL + wetR);
    const float wetSide = .5f * (wetL - wetR) * (chorusMode_ == 2 ? 1.8f : 1.5f);
    const float center = (1 - chorusMix_) * hpOutput_ + chorusMix_ * wetMid;
    const float l = volume_ * (center + chorusMix_ * wetSide);
    const float r = volume_ * (center - chorusMix_ * wetSide);
    left = clamp(l / (1 + .2f * std::abs(l)), -1, 1);
    right = clamp(r / (1 + .2f * std::abs(r)), -1, 1);
}
