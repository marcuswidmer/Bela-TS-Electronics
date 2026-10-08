#include "../WiPepper.hpp"
#include "../ProgramIndicator.hpp"
#include "PepperIO.hpp"
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
    std::array<float, 8> pots{{0, .5f, .5f, .5f, .5f, 1, 0, 0}};
    std::array<Button, 4> buttons{};
    std::array<float, 10> thresholds{};
    unsigned int frame = 0, ledMask = 0, visibleLeds = 0;
    Simulator() {
        io.audioConnected = true;
        pepper.setup(sampleRate, sampleRate / 2);
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
            if(frame % 2 == 0) pepper.processControls();
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
