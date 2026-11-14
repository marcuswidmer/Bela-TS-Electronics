#include "../WiPepper.hpp"
#include "../ProgramIndicator.hpp"
#include "PepperIO.hpp"
#include "SawInput.hpp"
#include "../MidiInput.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr unsigned int sampleRate = 44100, debounceSamples = 221;
struct Button {
    bool pressed = false, stable = false;
    unsigned int samples = 0;
};
struct Simulator {
    WiPepper pepper;
    ProgramIndicator indicator;
    PepperIO io;
    SawInput source;
    MidiInput midiInput;
    double clocksUntil = 0;
    float clockBpm = 120;
    bool clockRunning = false;
    std::array<float, 8> pots{{0, .5f, .5f, .5f, .5f, 1, 0, 0}};
    std::array<Button, 4> buttons{};
    std::array<float, 10> thresholds{};
    unsigned int frame = 0, ledMask = 0, visibleLeds = 0;
    Simulator() {
        io.audioConnected = true;
        pepper.setup(sampleRate, sampleRate / 2);
        source.setup(sampleRate);
        pepper.setPots(pots);
        indicator.setup(sampleRate);
        indicator.select(pepper.program());
        for(unsigned int i = 0; i < thresholds.size(); ++i)
            thresholds[i] = std::pow(10.0f, (-48.0f + 5.0f * i) / 20.0f);
    }
    void advance(unsigned int frames, float* output = nullptr) {
        for(unsigned int n = 0; n < frames; ++n, ++frame) {
            for(unsigned int i = 0; i < buttons.size(); ++i) {
                auto& b = buttons[i];
                if(b.pressed == b.stable) b.samples = 0;
                else if(++b.samples >= debounceSamples) {
                    b.stable = b.pressed;
                    b.samples = 0;
                    if(b.stable) {
                        pepper.pressButton(i);
                        if(i == 0) indicator.select(pepper.program());
                    }
                }
            }
            if(clockRunning) {
                if(clocksUntil <= 0) {
                    midiInput.process(0xf8, pepper);
                    clocksUntil += sampleRate * 60.0 / (24 * clockBpm);
                }
                --clocksUntil;
            }
            if(frame % 2 == 0) pepper.processControls();
            io.audioIn[0] = io.audioIn[1] = source.process();
            // Browser playback consumes the same stereo frames as the device.
            pepper.process(io.audioIn.data(), io.audioOut.data());
            if(output) { output[2*n] = io.audioOut[0]; output[2*n+1] = io.audioOut[1]; }
            const float peak = std::max(std::abs(io.audioIn[0]), std::abs(io.audioIn[1]));
            ledMask = 0;
            for(unsigned int led = 0; led < 10; ++led) {
                bool on = pepper.program() == WiPepper::PgmKarplusResonator &&
                    peak >= thresholds[led];
                if(pepper.program() == WiPepper::PgmSequencer && led == 9)
                    on = pepper.sequencerLed();
                if(pepper.program() == WiPepper::PgmSequencer && led == 3)
                    on = pepper.directMidiMode();
                if(pepper.program() == WiPepper::PgmSequencer && led == 4)
                    on = pepper.sequenceLocked();
                if(pepper.program() == WiPepper::PgmSequencer && led == 5)
                    on = pepper.rimDrumPattern();
                if(pepper.program() == WiPepper::PgmSequencer && led == 6)
                    on = pepper.externalMidiClock();
                if(pepper.program() == WiPepper::PgmJuno) on = led < pepper.synthVoices();
                if(led < WiPepper::ProgramCount && indicator.active()) on = indicator.ledOn(led);
                if(on) ledMask |= 1u << led;
            }
            // Preserve short 10 ms pulses between browser polls.
            visibleLeds |= ledMask;
            indicator.advance();
        }
    }
};
}

extern "C" {
void* pepper_create() { return new Simulator; }
int pepper_load_samples(void* handle, const char* directory) {
    return static_cast<Simulator*>(handle)->pepper.loadDrumSamples(directory);
}
float pepper_pattern_velocity(unsigned int pattern, unsigned int voice, unsigned int step) {
    return WiAudioDrivenDrumMachine::patternVelocity(voice, step, pattern);
}
const char* pepper_pattern_name(unsigned int pattern) { return WiAudioDrivenDrumMachine::patternName(pattern); }
const char* pepper_voice_name(unsigned int pattern, unsigned int voice) { return WiAudioDrivenDrumMachine::voiceName(voice,pattern); }
unsigned int pepper_drum_pattern_count() { return WiAudioDrivenDrumMachine::PatternCount; }
unsigned int pepper_drum_step_count() { return WiAudioDrivenDrumMachine::StepCount; }
unsigned int pepper_drum_sample_count() { return WiAudioDrivenDrumMachine::SampleCount; }
const char* pepper_sample_error(void* handle) {
    return static_cast<Simulator*>(handle)->pepper.drums().loadError().c_str();
}
void pepper_destroy(void* handle) { delete static_cast<Simulator*>(handle); }
void pepper_advance(void* handle, unsigned int frames) {
    static_cast<Simulator*>(handle)->advance(frames);
}
void pepper_render(void* handle, unsigned int frames, float* output) {
    static_cast<Simulator*>(handle)->advance(frames, output);
}
void pepper_note(void* handle, unsigned int note, unsigned int velocity) {
    auto& pepper = static_cast<Simulator*>(handle)->pepper;
    if(velocity) pepper.midiNoteOn(note, velocity);
    else pepper.midiNoteOff(note);
}
void pepper_panic(void* handle) {
    auto& pepper = static_cast<Simulator*>(handle)->pepper;
    pepper.midiControlChange(120, 0);
}
void pepper_source(void* handle, unsigned int index, float value) {
    if(!std::isfinite(value)) return;
    auto& source = static_cast<Simulator*>(handle)->source;
    if(index == 0) source.enabled = value != 0;
    else if(index == 1) source.frequency = std::max(35.0f, std::min(8000.0f, value));
    else if(index == 2) source.level = std::max(0.0f, std::min(1.0f, value));
    else if(index < 10) {
        source.eq[index-3] = std::max(-24.0f, std::min(24.0f, value));
        source.updateEq();
    } else if(index == 10) source.sine = value != 0;
}
void pepper_clock(void* handle, float bpm, unsigned int transport) {
    if(!std::isfinite(bpm) || bpm < 20 || bpm > 300 || transport > 3) return;
    auto& s = *static_cast<Simulator*>(handle);
    s.clockBpm = bpm;
    if(transport == 1 || transport == 3) {
        s.clockRunning = true;
        s.clocksUntil = 0;
        s.midiInput.process(transport == 1 ? 0xfa : 0xfb, s.pepper);
    } else if(transport == 2) {
        s.clockRunning = false;
        s.midiInput.process(0xfc, s.pepper);
    }
}
void pepper_midi_byte(void* handle, unsigned int byte) {
    if(byte > 255) return;
    auto& s = *static_cast<Simulator*>(handle);
    // A real MIDI source takes over from the test-clock generator.
    if(byte == 0xf8 || byte == 0xfa || byte == 0xfb || byte == 0xfc) s.clockRunning = false;
    s.midiInput.process(static_cast<uint8_t>(byte), s.pepper);
}
// Extra state is separate so the original front-panel ABI stays stable.
void pepper_drum_state(void* handle, float* out) {
    auto& s = *static_cast<Simulator*>(handle);
    const auto& drums = s.pepper.drums();
    out[0] = drums.loadedSamples(); out[1] = drums.currentStep();
    out[2] = s.pepper.tempoBpm(); out[3] = s.pepper.externalMidiClock();
    out[4] = s.clockBpm; out[5] = s.clockRunning;
    out[6] = s.source.enabled; out[7] = s.source.frequency; out[8] = s.source.level;
    for(unsigned int i = 0; i < 7; ++i) {
        out[9+i] = s.source.eq[i];
        out[16+i] = drums.intensities()[i];
        out[23+i] = drums.triggerCounts()[i];
        out[30+i] = drums.intensities()[i];
        out[37+i] = drums.lastVelocities()[i];
    }
    out[44] = s.source.sine;
    out[45] = drums.currentPattern(); out[46] = drums.filterOpening();
}
void pepper_pot(void* handle, unsigned int index, float value) {
    if(index >= 8 || !std::isfinite(value)) return;
    auto& s = *static_cast<Simulator*>(handle);
    s.pots[index] = std::max(0.0f, std::min(1.0f, value));
    s.pepper.setPots(s.pots);
}
void pepper_button(void* handle, unsigned int index, int pressed) {
    if(index >= 4) return;
    auto& b = static_cast<Simulator*>(handle)->buttons[index];
    if(b.pressed != (pressed != 0)) b.samples = 0;
    b.pressed = pressed != 0;
}
// Stable C ABI: program, 8 pots, 4 buttons, 10 LEDs, selection-active.
void pepper_state(void* handle, float* out) {
    auto& s = *static_cast<Simulator*>(handle);
    out[0] = s.pepper.program();
    for(unsigned int i = 0; i < 8; ++i) out[1+i] = s.pots[i];
    for(unsigned int i = 0; i < 4; ++i) out[9+i] = s.buttons[i].stable;
    for(unsigned int i = 0; i < 10; ++i) out[13+i] = ((s.ledMask | s.visibleLeds) >> i) & 1;
    out[23] = s.indicator.active();
    s.visibleLeds = 0;
}
}
