#include <Bela.h>
#include "WiPepper.hpp"
#include "ProgramIndicator.hpp"
#include <libraries/Midi/Midi.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
WiPepper wiPepper;
ProgramIndicator programIndicator;
Midi midi;
constexpr const char* kMidiPort = "hw:1,0,0";
bool midiInputReady = false, midiOutputReady = false;

void sendMidiByte(uint8_t byte)
{
    if(midiOutputReady) midi.writeOutput(byte);
}

void readMidi()
{
    if(!midiInputReady) return;
    auto* parser = midi.getParser();
    // Consume a bounded snapshot on the render thread, so note assignment and
    // sequence processing never modify the sequence concurrently.
    const int available = parser->numAvailableMessages();
    for(int i = 0; i < available; ++i) {
        const auto message = parser->getNextChannelMessage();
        if(message.getType() == kmmNoteOn)
            wiPepper.midiNoteOn(message.getDataByte(0), message.getDataByte(1));
    }
}

// Pepper LED pins in left-to-right order.
constexpr unsigned int kLedPins[] = {6, 7, 10, 2, 3, 0, 1, 4, 5, 8};
constexpr unsigned int kButtonPins[] = {15, 14, 13, 12};
constexpr unsigned int kLedCount = sizeof(kLedPins) / sizeof(kLedPins[0]);

std::array<float, 8> pots{{0, 0.5f, 0.5f, 0.5f, 0.5f, 1, 0, 0}};
struct Button {
    bool initialized = false, candidate = false, stable = false;
    unsigned int samples = 0;
};
std::array<Button, 4> buttons{};
unsigned int controlCountdown = 0, controlInterval = 44, debounceSamples = 221;
float ledPeakThresholds[kLedCount] = {};
float inputPeak = 0.0f;

static void abortIfLoud(float y)
{
    if (std::fabs(y) > 1.4) {
        std::printf("Too loud! out: %f\n", y);
        std::abort();
    }
}

void readControls(BelaContext* context, unsigned int frame)
{
    if(controlCountdown == 0) {
        if(context->analogFrames) {
            const unsigned int analogFrame = frame * context->analogFrames / context->audioFrames;
            for(unsigned int pot=0; pot<pots.size() && pot<context->analogInChannels; ++pot)
                pots[pot] = analogRead(context, analogFrame, pot);
        }
        wiPepper.setPots(pots);
        controlCountdown = controlInterval;
    }
    --controlCountdown;
    if(!context->digitalFrames) return;
    const unsigned int digitalFrame = frame * context->digitalFrames / context->audioFrames;
    for(unsigned int i=0; i<buttons.size(); ++i) {
        if(kButtonPins[i] >= context->digitalChannels) continue;
        const bool pressed = digitalRead(context, digitalFrame, kButtonPins[i]) != 0;
        auto& button = buttons[i];
        if(!button.initialized) {
            button.initialized = true;
            button.candidate = button.stable = pressed;
        }
        if(pressed != button.candidate) {
            button.candidate = pressed;
            button.samples = 0;
        }
        if(button.samples < debounceSamples) ++button.samples;
        if(button.samples >= debounceSamples && button.stable != button.candidate) {
            button.stable = button.candidate;
            if(button.stable) {
                wiPepper.pressButton(i);
                if(i == 0) programIndicator.select(wiPepper.program());
            }
        }
    }
}
} // namespace

bool setup(BelaContext* context, void* userData)
{
    const float controlSampleRate = context->analogFrames ? context->analogSampleRate : context->audioSampleRate;
    if(!wiPepper.setup(context->audioSampleRate, controlSampleRate, sendMidiByte) || context->audioInChannels < 2 || context->audioOutChannels < 2)
        return false;
    midi.enableParser(true);
    midiInputReady = midi.readFrom(kMidiPort) > 0;
    midiOutputReady = midi.writeTo(kMidiPort) > 0;
    if(!midiInputReady || !midiOutputReady)
        std::printf("MIDI %s: input %s, output %s\n", kMidiPort,
                    midiInputReady ? "ready" : "unavailable", midiOutputReady ? "ready" : "unavailable");
    programIndicator.setup(context->digitalSampleRate);
    programIndicator.select(wiPepper.program());
    controlInterval = std::max(1u, static_cast<unsigned int>(context->audioSampleRate/1000));
    debounceSamples = std::max(1u, static_cast<unsigned int>(context->audioSampleRate*0.005f));
    controlCountdown = 0;
    buttons = {};
    for(auto pin : kButtonPins)
        if(context->digitalFrames && pin < context->digitalChannels)
            pinMode(context, 0, pin, INPUT);
    Bela_setAudioInputGain(0, 0.0f); // Left input
    Bela_setAudioInputGain(1, 0.0f); // Right input
    inputPeak = 0.0f;
    for (unsigned int led = 0; led < kLedCount; ++led) {
        const float thresholdDb = -48.0f + 5.0f * led;
        ledPeakThresholds[led] = std::pow(10.0f, thresholdDb / 20.0f);
        if (context->digitalFrames && kLedPins[led] < context->digitalChannels) {
            pinMode(context, 0, kLedPins[led], OUTPUT);
            digitalWrite(context, 0, kLedPins[led], LOW);
        }
    }

    int analogIOSampleRate = (int)context->analogSampleRate;

    std::printf("Fs: %f. Analog fs: %d Buffersize: %d\n", context->audioSampleRate, analogIOSampleRate, context->audioFrames);

    return true;
}

void render(BelaContext* context, void* userData)
{
    readMidi();
    inputPeak = 0.0f;
    unsigned int analogFrame = 0;
    for (unsigned int n = 0; n < context->audioFrames; n++) {

        readControls(context, n);
        // Write each analog frame exactly once, including when the analog and
        // audio rates differ. Keep the sequencer at its configured analog rate.
        if(!context->analogFrames) wiPepper.processControls();
        while(analogFrame < context->analogFrames &&
              analogFrame * context->audioFrames < (n + 1) * context->analogFrames) {
            wiPepper.processControls();
            const auto& cv = wiPepper.cvOutputs();
            for(unsigned int channel = 0; channel < context->analogOutChannels; ++channel)
                analogWriteOnce(context, analogFrame, channel, channel < cv.size() ? cv[channel] : 0.0f);
            ++analogFrame;
        }
        float in[2] = {};
        float out[2] = {};

        in[0] = audioRead(context, n, 0);
        in[1] = audioRead(context, n, 1);

        // Track both inputs before volume control for the LED meter.
        inputPeak = std::max(inputPeak, std::max(std::abs(in[0]), std::abs(in[1])));
        wiPepper.process(in, out);
        abortIfLoud(out[0]);

        audioWrite(context, n, 0, out[0]);
        audioWrite(context, n, 1, out[1]);
    }

    for (unsigned int frame = 0; frame < context->digitalFrames; ++frame) {
        for (unsigned int led = 0; led < kLedCount; ++led) {
            if (kLedPins[led] < context->digitalChannels) {
                bool on = wiPepper.program() == WiPepper::PgmKarplusResonator
                    && inputPeak >= ledPeakThresholds[led];
                if(wiPepper.program() == WiPepper::PgmSequencer && led == kLedCount - 1)
                    on = wiPepper.sequencerLed();
                if(programIndicator.active())
                    on = programIndicator.ledOn(led);
                digitalWriteOnce(context, frame, kLedPins[led], on ? HIGH : LOW);
            }
        }
        programIndicator.advance();
    }
}

void cleanup(BelaContext* context, void* userData)
{
    if(wiPepper.program() == WiPepper::PgmSequencer) sendMidiByte(0xFC);
}
