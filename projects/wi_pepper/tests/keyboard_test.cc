#include <array>
#include <cstdint>
#define private public
#include "JunoSynth.hpp"
#undef private
#include <cassert>
#include <cmath>
#include <iostream>

void run(JunoSynth& synth, unsigned int frames) {
    while(frames--) { float l, r; synth.process(l, r); assert(std::isfinite(l) && std::isfinite(r)); }
}
int main() {
    for(unsigned int fs : {22050u, 44100u, 48000u, 96000u}) {
        JunoSynth synth;
        assert(synth.setup(fs));
        synth.setPots({{.7f, .6f, .2f, .3f, 0, .2f, .8f, 0}});
        synth.noteOn(60, 100, 0);
        synth.noteOn(60, 100, 1);
        assert(synth.activeVoices() == 2); // No chord collection delay.
        synth.noteOff(60, 0);
        run(synth, fs);
        assert(synth.activeVoices() == 1); // Other channel remains held.
        synth.controlChange(64, 127, 1);
        synth.noteOn(60, 0, 1);
        run(synth, fs);
        assert(synth.activeVoices() == 1);
        synth.controlChange(64, 0, 1);
        run(synth, fs);
        assert(synth.activeVoices() == 0);
        for(unsigned int note = 60; note < 66; ++note) synth.noteOn(note, 100);
        synth.noteOn(65, 127); // Retrigger same key without adding a voice.
        assert(synth.activeVoices() == 6);
        synth.noteOn(67, 100);
        assert(synth.activeVoices() == 6);
        bool hasOldest = false, hasNew = false;
        for(const auto& voice : synth.voices_) {
            hasOldest |= voice.note == 60;
            hasNew |= voice.note == 67;
        }
        assert(!hasOldest && hasNew);
        for(unsigned int note = 60; note <= 67; ++note) synth.noteOff(note);
        run(synth, fs);
        assert(synth.activeVoices() == 0);
        // A quick tap still releases, even before an audio frame runs.
        synth.noteOn(72, 100); synth.noteOff(72); run(synth, fs);
        assert(synth.activeVoices() == 0);
    }
    std::cout << "Regular keyboard behavior tests passed\n";
}
