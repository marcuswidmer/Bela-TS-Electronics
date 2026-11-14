#include "WiAudioDrivenDrumMachine.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <utility>

namespace {
constexpr float centers[] = {110, 220, 440, 880, 1760, 3520, 7040};
constexpr float sampleOutputGain = .7f; // Twice the previous .35 gain (+6 dB), before the soft limiter.
// Seven lo-fi voices, ordered from kick/body to bright percussion. The three
// added variations replace only voice 2 with the Yamaha rim sample.
constexpr unsigned int rimSample = 7;
constexpr unsigned int realSnareSample = 8, firstHitSample = 9, firstShakeSample = 12, triangleSample = 18, realKickSample = 19;
const char* const names[] = {"Kick", "Tom", "Snare", "Conga", "Claves", "Stab", "Hi-hat", "Rim"};
const char* const realVoiceNames[] = {"Snare", "Tamb hit", "Kick", "", "Shake", "", "Triangle"};
const char* const files[] = {
    "use_ALF_Vintage_Kit_Kick.wav",
    "use_ALF_Raw_Analog_Kit_Mid_Tom.wav",
    "use_ALF_Raw_Analog_Kit_Snare.wav",
    "use_ALF_Raw_Analog_Kit_Mid_Conga.wav",
    "use_ALF_Vintage_Kit_Claves.wav",
    "use_ALF_Raw_Analog_Kit_Perc_Stab.wav",
    "use_ALF_Raw_Analog_Kit_Closed_Hats.wav",
    "use_HS5 - SnareDrum Rim.wav",
    "snare.wav",
    "EAR_CANDY_tamourine_hit_var_1.wav",
    "EAR_CANDY_tamourine_hit_var_2.wav",
    "EAR_CANDY_tamourine_hit_var_3.wav",
    "EAR_CANDY_tamourine_shake_var_1.wav",
    "EAR_CANDY_tamourine_shake_var_2.wav",
    "EAR_CANDY_tamourine_shake_var_3.wav",
    "EAR_CANDY_tamourine_shake_var_4.wav",
    "EAR_CANDY_tamourine_shake_var_5.wav",
    "EAR_CANDY_tamourine_shake_var_6.wav",
    "EAR_CANDY_light_triangle.wav",
    "kick_marius.wav"
};
const char* const patternNames[] = {"Lo-fi Pocket", "Lo-fi Four", "Lo-fi Broken", "Rim Pocket", "Rim Four", "Rim Broken", "Real Kit 6/8"};
// Three originals, three rim variations, then the 6/8 real-kit groove, all one bar.
// First six patterns: kick, tom, snare/rim, conga, claves, stab, hi-hat.
constexpr float patterns[WiAudioDrivenDrumMachine::PatternCount][7][16] = {
  {
    {1,0,0,0, 0,0,.65f,0, .9f,0,0,0, 0,0,.55f,0},
    {0,0,0,0, 0,0,0,0, 0,0,.55f,0, 0,0,0,.45f},
    {0,0,0,0, 1,0,0,0, 0,0,0,0, .9f,0,0,0},
    {0,0,0,.5f, 0,0,0,0, 0,0,0,.6f, 0,0,0,0},
    {0,0,0,0, 0,0,0,.5f, 0,0,0,0, 0,0,0,.45f},
    {0,0,0,0, 0,0,.55f,0, 0,0,0,0, 0,0,.65f,0},
    {1,0,.45f,0, .7f,0,.45f,0, .9f,0,.45f,0, .7f,0,.5f,0}
  }, {
    {1,0,0,0, .85f,0,0,0, 1,0,0,0, .85f,0,0,0},
    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,.5f,.7f},
    {0,0,0,0, .95f,0,0,0, 0,0,0,0, 1,0,0,0},
    {0,0,0,0, 0,0,.55f,0, 0,0,.45f,0, 0,0,0,0},
    {0,0,0,.5f, 0,0,0,0, 0,0,0,.6f, 0,0,0,0},
    {0,0,0,0, 0,0,0,.4f, 0,0,0,0, 0,0,0,.65f},
    {0,0,.9f,0, 0,0,.7f,0, 0,0,.9f,0, 0,0,.75f,0}
  }, {
    {1,0,0,.55f, 0,0,0,.8f, 0,0,.9f,0, 0,0,.65f,0},
    {0,0,0,0, 0,0,.55f,0, 0,0,0,0, 0,.65f,0,0},
    {0,0,0,0, 1,0,0,0, 0,0,0,.25f, .9f,0,0,.3f},
    {0,0,.4f,0, 0,0,0,0, 0,.65f,0,.4f, 0,0,0,0},
    {0,0,0,0, 0,.55f,0,0, 0,0,0,0, 0,0,0,.65f},
    {0,0,0,0, 0,0,.4f,0, 0,0,0,0, 0,0,.7f,0},
    {.8f,0,.4f,0, .7f,0,.4f,.25f, .85f,0,.4f,0, .7f,0,.45f,.3f}
  }, { // Rim Pocket
    {1, 0, 0, 0, 0, 0, 0.6f, 0, 0.9f, 0, 0, 0, 0, 0, 0, 0.45f},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.45f, 0, 0, 0, 0.35f, 0},
    {0, 0, 0, 0, 0.85f, 0, 0, 0, 0, 0, 0, 0, 0.78f, 0, 0, 0},
    {0, 0, 0, 0.45f, 0, 0, 0, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0.4f, 0, 0, 0, 0, 0, 0, 0, 0.35f},
    {0, 0, 0, 0, 0, 0, 0.45f, 0, 0, 0, 0, 0, 0, 0, 0.5f, 0},
    {0.9f, 0, 0.4f, 0, 0.65f, 0, 0.4f, 0, 0.85f, 0, 0.4f, 0, 0.65f, 0, 0.45f, 0.18f},
  }, { // Rim Four
    {1, 0, 0, 0, 0.85f, 0, 0, 0, 1, 0, 0, 0, 0.85f, 0, 0, 0.3f},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.4f, 0.55f},
    {0, 0, 0, 0, 0.8f, 0, 0, 0, 0, 0, 0, 0, 0.85f, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0.45f, 0, 0, 0, 0.4f, 0, 0, 0, 0, 0},
    {0, 0, 0, 0.4f, 0, 0, 0, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0.35f, 0, 0, 0, 0, 0, 0, 0, 0.5f},
    {0, 0, 0.85f, 0, 0, 0, 0.65f, 0, 0, 0, 0.85f, 0, 0, 0, 0.7f, 0.18f},
  }, { // Rim Broken
    {1, 0, 0, 0.5f, 0, 0, 0.7f, 0, 0, 0, 0.85f, 0, 0, 0, 0.6f, 0},
    {0, 0, 0, 0, 0, 0, 0.45f, 0, 0, 0, 0, 0, 0, 0.55f, 0, 0},
    {0, 0, 0, 0, 0.85f, 0, 0, 0, 0, 0, 0, 0.18f, 0.78f, 0, 0, 0.22f},
    {0, 0, 0.35f, 0, 0, 0, 0, 0, 0, 0.55f, 0, 0.35f, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0.45f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.55f},
    {0, 0, 0, 0, 0, 0, 0.35f, 0, 0, 0, 0, 0, 0, 0, 0.6f, 0},
    {0.75f, 0, 0.35f, 0, 0.65f, 0, 0.35f, 0, 0.8f, 0, 0.35f, 0, 0.65f, 0, 0.4f, 0.22f},
  }, { // Real Kit 6/8: six eighth notes, twelve sixteenths.
    {0,0,0,0, 0,0,0,0, 1,0,0,0}, // Eighth note 5: snare and tambourine hit.
    {0,0,0,0, 0,0,0,0, .95f,0,0,0},
    {1,0,0,0, 0,0,0,0, 0,0,0,0}, // Kick on eighth note 1.
    {},
    {.65f,0,.55f,0, .6f,0,.65f,0, 0,0,.7f,0}, // Shakes on eighth notes 1, 2, 3, 4 and 6.
    {},
    {.65f,0,0,0, .65f,0,0,0, .65f,0,0,0}, // Triangle on eighth notes 1, 3 and 5.
  }
};
float unit(float x) { return std::isfinite(x) ? std::max(0.0f, std::min(1.0f, x)) : 0; }
uint16_t u16(const unsigned char* p) { return p[0] | uint16_t(p[1]) << 8; }
uint32_t u32(const unsigned char* p) { return u16(p) | uint32_t(u16(p + 2)) << 16; }
}
float WiAudioDrivenDrumMachine::bandFrequency(unsigned int voice) { return voice < VoiceCount ? centers[voice] : 0; }
unsigned int WiAudioDrivenDrumMachine::patternSample(unsigned int voice, unsigned int pattern, unsigned int step, unsigned int variation) {
    if(pattern >= PatternCount || voice >= VoiceCount) return SampleCount;
    if(pattern == RealKitPattern) {
        if(voice == 0) return realSnareSample;
        if(voice == 1) return firstHitSample + variation % 3;
        if(voice == 2) return realKickSample;
        if(voice == 4) {
            const auto shake = step / 2 - (step >= 8 ? 1 : 0);
            return firstShakeSample + (5 * (variation % 6) + shake) % 6;
        }
        if(voice == 6) return triangleSample;
        return SampleCount;
    }
    return voice == 2 && pattern >= 3 ? rimSample : voice;
}
unsigned int WiAudioDrivenDrumMachine::patternStepCount(unsigned int pattern) {
    return pattern < PatternCount ? (pattern == RealKitPattern ? 12 : StepCount) : 0;
}
const char* WiAudioDrivenDrumMachine::voiceName(unsigned int voice, unsigned int pattern) {
    if(pattern == RealKitPattern && voice < VoiceCount) return realVoiceNames[voice];
    const auto sample = patternSample(voice, pattern); return sample < SampleCount ? names[sample] : "";
}
const char* WiAudioDrivenDrumMachine::sampleFilename(unsigned int sample) { return sample < SampleCount ? files[sample] : ""; }
const char* WiAudioDrivenDrumMachine::kitName(unsigned int kit) { return kit < KitCount ? (kit == 1 ? "Real kit" : "Lo-fi") : ""; }
const char* WiAudioDrivenDrumMachine::patternName(unsigned int pattern) { return pattern < PatternCount ? patternNames[pattern] : ""; }
float WiAudioDrivenDrumMachine::patternVelocity(unsigned int voice, unsigned int step, unsigned int pattern) {
    return voice < VoiceCount && step < patternStepCount(pattern) ? patterns[pattern][voice][step] : 0;
}
bool WiAudioDrivenDrumMachine::setup(float sampleRate) {
    if(!std::isfinite(sampleRate) || sampleRate < 22050 || sampleRate > 96000) return false;
    rate_ = sampleRate; smoothing_ = 1-std::exp(-1/(.02f*rate_));
    voices_ = {}; triggers_ = {}; lastVelocities_ = {};
    gain_ = 0; pattern_ = 0; controlsInitialized_ = false;
    filterSmoothed_ = filterTarget_; updateIntensities(); seekStep(0);
    return true;
}
void WiAudioDrivenDrumMachine::setControls(float volume, float filter) {
    volume_ = unit(volume); filterTarget_ = unit(filter);
    if(!controlsInitialized_) { filterSmoothed_ = filterTarget_; controlsInitialized_ = true; updateIntensities(); }
}
void WiAudioDrivenDrumMachine::updateIntensities() {
    if(pattern_ == RealKitPattern) {
        intensities_ = {};
        intensities_[1] = intensities_[4] = 1; // Tambourine hit and shakes are always present.
        const auto fadeIn = [this](float start) {
            const float x = unit((filterSmoothed_ - start) / .28f);
            return x*x*(3-2*x);
        };
        intensities_[6] = fadeIn(.04f); // Triangle first, then kick, then snare.
        intensities_[2] = fadeIn(.36f);
        intensities_[0] = fadeIn(.68f);
        return;
    }
    intensities_[0] = 1;
    for(unsigned int i=1; i<VoiceCount; ++i) {
        const float x = unit((filterSmoothed_-(.08f+.16f*(i-1)))/.12f);
        intensities_[i] = x*x*(3-2*x);
    }
}
void WiAudioDrivenDrumMachine::silence() {
    for(auto& voice : voices_) { voice.playback = {}; voice.nextPlayback = 0; }
    lastVelocities_ = {};
}
void WiAudioDrivenDrumMachine::seekStep(unsigned int position) {
    const auto steps = patternStepCount(pattern_);
    nextStep_ = position % steps;
    nextVariation_ = pattern_ == RealKitPattern ? (position / steps) % 6 : 0;
    currentStep_ = -1; lastVelocities_ = {};
}
bool WiAudioDrivenDrumMachine::selectPattern(unsigned int pattern) {
    if(pattern >= PatternCount) return false;
    if(pattern != pattern_) nextVariation_ = 0;
    pattern_ = pattern;
    const auto steps = patternStepCount(pattern_);
    nextStep_ %= steps;
    if(currentStep_ >= 0) currentStep_ %= steps;
    updateIntensities();
    lastVelocities_ = {}; return true;
}
void WiAudioDrivenDrumMachine::advanceStep() {
    const auto variation = nextVariation_;
    currentStep_ = nextStep_; nextStep_ = (nextStep_+1)%patternStepCount(pattern_);
    // Five shakes per bar cycle through all six takes; hits cycle through three.
    if(pattern_ == RealKitPattern && nextStep_ == 0) nextVariation_ = (nextVariation_ + 1) % 6;
    for(unsigned int i=0; i<VoiceCount; ++i) {
        const float accent = patterns[pattern_][i][currentStep_];
        lastVelocities_[i] = accent*intensities_[i];
        if(lastVelocities_[i] <= 0) continue;
        ++triggers_[i];
        const auto sample = patternSample(i,pattern_,currentStep_,variation);
        if(samples_[sample].stereo.empty()) continue;
        auto& voice = voices_[i];
        auto& hit = voice.playback[voice.nextPlayback++%voice.playback.size()];
        hit.position = 0; hit.gain = accent; hit.sample = sample; hit.active = true;
    }
}
unsigned int WiAudioDrivenDrumMachine::loadedSamples() const {
    unsigned int count=0; for(const auto& sample : samples_) count += !sample.stereo.empty(); return count;
}
bool WiAudioDrivenDrumMachine::loadSamples(const std::string& projectDirectory) {
    std::array<Sample,SampleCount> replacement;
    for(unsigned int i=0; i<SampleCount; ++i) {
        const char* directory = i >= realSnareSample ? "real_kit" :
            i == rimSample ? "Yamaha_Electone_HS5" : "minimal_lofi_sounds";
        if(!readWave(projectDirectory+"/"+directory+"/"+files[i], replacement[i], loadError_)) return false;
    }
    samples_ = std::move(replacement); silence(); loadError_.clear(); return true;
}
bool WiAudioDrivenDrumMachine::readWave(const std::string& path, Sample& sample, std::string& error)
{
    auto fail = [&](const char* why) { error = path + ": " + why; return false; };
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if(!stream) return fail("cannot open sample");
    const auto length = stream.tellg();
    if(length < 12 || length > 16 * 1024 * 1024) return fail("invalid WAV length");
    std::vector<unsigned char> bytes(static_cast<size_t>(length));
    stream.seekg(0);
    if(!stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) return fail("cannot read sample");
    if(std::memcmp(bytes.data(), "RIFF", 4) || std::memcmp(bytes.data()+8, "WAVE", 4)) return fail("expected RIFF/WAVE");
    unsigned int format = 0, channels = 0, rate = 0, bits = 0, align = 0;
    size_t data = 0, dataSize = 0;
    for(size_t offset = 12; offset + 8 <= bytes.size();) {
        const auto* p = bytes.data() + offset;
        const size_t size = u32(p + 4);
        offset += 8;
        if(size > bytes.size() - offset) return fail("truncated WAV chunk");
        if(!std::memcmp(p, "fmt ", 4) && size >= 16) {
            const auto* f = bytes.data()+offset;
            format = u16(f); channels = u16(f+2); rate = u32(f+4);
            align = u16(f+12); bits = u16(f+14);
            if(format == 0xfffe && size >= 40) format = u16(f+24);
        } else if(!std::memcmp(p, "data", 4)) { data = offset; dataSize = size; }
        offset += size + (size & 1);
    }
    if(!data || !dataSize || (channels != 1 && channels != 2) || rate < 8000 || rate > 192000 ||
       !((format == 3 && bits == 32) || (format == 1 && (bits == 16 || bits == 24 || bits == 32))) ||
       align != channels * (bits / 8) || dataSize % align) return fail("unsupported WAV format");
    sample.rate = rate;
    const size_t frames = dataSize / align;
    sample.stereo.resize(frames * 2);
    for(size_t frame = 0; frame < frames; ++frame) {
        for(unsigned int channel = 0; channel < 2; ++channel) {
            const auto* p = bytes.data() + data + frame*align + std::min(channel, channels-1)*(bits/8);
            float value;
            if(format == 3) {
                const uint32_t raw = u32(p);
                std::memcpy(&value, &raw, sizeof(value));
            } else {
                const uint32_t raw = bits == 16 ? uint32_t(u16(p)) << 16 :
                    bits == 24 ? uint32_t(p[0]) << 8 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 24 : u32(p);
                int32_t signedValue;
                std::memcpy(&signedValue, &raw, sizeof(raw));
                value = static_cast<float>(signedValue / 2147483648.0);
            }
            if(!std::isfinite(value)) return fail("non-finite sample data");
            sample.stereo[2*frame+channel] = std::max(-1.0f, std::min(1.0f, value));
        }
    }
    return true;
}

void WiAudioDrivenDrumMachine::process(float input, float& left, float& right) {
    (void)input; left = right = 0;
    if(!rate_) return;
    gain_ += smoothing_*(volume_-gain_);
    filterSmoothed_ += smoothing_*(filterTarget_-filterSmoothed_);
    updateIntensities();
    for(unsigned int i=0; i<VoiceCount; ++i) {
        for(auto& hit : voices_[i].playback) {
            if(!hit.active) continue;
            const auto& sample = samples_[hit.sample];
            const size_t frames = sample.stereo.size()/2, frame = static_cast<size_t>(hit.position);
            if(frame >= frames) { hit.active = false; continue; }
            const size_t next = std::min(frame+1,frames-1);
            const float fraction = hit.position-frame;
            const float onset = std::min(1.0,hit.position/(sample.rate*.001));
            const float level = hit.gain*intensities_[i]*onset;
            left += level*(sample.stereo[2*frame]+fraction*(sample.stereo[2*next]-sample.stereo[2*frame]));
            right += level*(sample.stereo[2*frame+1]+fraction*(sample.stereo[2*next+1]-sample.stereo[2*frame+1]));
            hit.position += sample.rate/rate_;
        }
    }
    left = std::tanh(sampleOutputGain*gain_*left); right = std::tanh(sampleOutputGain*gain_*right);
}
