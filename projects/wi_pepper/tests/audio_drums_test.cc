#include "WiAudioDrivenDrumMachine.hpp"
#include "WiPepper.hpp"
#include "MidiInput.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
namespace { bool watch = false; size_t allocations = 0;
constexpr unsigned int expected[7][7] = {{4,2,2,2,2,2,8},{4,2,2,2,2,2,4},{5,2,4,3,2,2,10},{4,2,2,2,2,2,9},{5,2,2,2,2,2,5},{5,2,4,3,2,2,9},{1,1,1,0,5,0,3}}; }
void* operator new(size_t size) { if(watch) ++allocations; if(void* p=std::malloc(size)) return p; throw std::bad_alloc(); }
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,size_t) noexcept { std::free(p); }
void operator delete[](void* p,size_t) noexcept { std::free(p); }
int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(WiAudioDrivenDrumMachine::StepCount==16, "Original one-bar patterns");
    std::array<bool,WiAudioDrivenDrumMachine::SampleCount> used{};
    for(unsigned int pattern=0;pattern<WiAudioDrivenDrumMachine::PatternCount;++pattern) {
        if(pattern<6) assert(std::strstr(WiAudioDrivenDrumMachine::sampleFilename(WiAudioDrivenDrumMachine::patternSample(0,pattern)),"Kick"));
        assert(WiAudioDrivenDrumMachine::patternStepCount(pattern)==(pattern==6 ? 12 : 16));
        for(unsigned int v=0;v<7;++v) {
            for(unsigned int step=0;step<WiAudioDrivenDrumMachine::patternStepCount(pattern);++step) {
                if(!WiAudioDrivenDrumMachine::patternVelocity(v,step,pattern)) continue;
                for(unsigned int variation=0;variation<(pattern==6 ? 6u : 1u);++variation) {
                    const auto sample=WiAudioDrivenDrumMachine::patternSample(v,pattern,step,variation);
                    assert(sample<used.size()); used[sample]=true;
                    if(pattern==6) assert(sample>=8); // Only real-kit WAVs in the 6/8 groove.
                }
            }
        }
    }
    for(bool sample : used) assert(sample);
    assert(WiAudioDrivenDrumMachine::patternStepCount(7)==0);
    assert(WiAudioDrivenDrumMachine::patternVelocity(0,8,6)==1);
    assert(WiAudioDrivenDrumMachine::patternVelocity(1,8,6)>0);
    assert(WiAudioDrivenDrumMachine::patternVelocity(0,12,6)==0);
    for(unsigned int step=0;step<12;++step) {
        for(unsigned int voice=0;voice<7;++voice) {
            const bool expectedHit = (voice==2 && step==0) || (voice==6 && step%4==0) ||
                (step==8 ? (voice==0 || voice==1) : step%2==0 && voice==4);
            assert((WiAudioDrivenDrumMachine::patternVelocity(voice,step,6)>0)==expectedHit);
        }
    }
    assert(WiAudioDrivenDrumMachine::patternSample(7,0)==WiAudioDrivenDrumMachine::SampleCount);
    for(float rate : {22050.f,44100.f,48000.f,96000.f}) {
        WiAudioDrivenDrumMachine drums;
        assert(drums.setup(rate) && drums.loadSamples(argv[1]));
        assert(drums.loadedSamples()==WiAudioDrivenDrumMachine::SampleCount);
        assert(!drums.loadSamples("/nonexistent/drum-kits") && drums.loadedSamples()==WiAudioDrivenDrumMachine::SampleCount);
        float l,r;
        auto render=[&](unsigned int frames,float input=0) {
            float peak=0;
            for(unsigned int n=0;n<frames;++n) {
                drums.process(input,l,r);
                assert(std::isfinite(l) && std::isfinite(r) && std::abs(l)<=1 && std::abs(r)<=1);
                peak=std::max(peak,std::max(std::abs(l),std::abs(r)));
            }
            return peak;
        };
        watch=true;
        for(unsigned int pattern=0;pattern<WiAudioDrivenDrumMachine::PatternCount;++pattern) {
            for(float filter : {0.f,1.f}) {
                assert(drums.selectPattern(pattern)); assert(drums.currentKit()==(pattern==6 ? 1 : 0));
                drums.setControls(.8f,filter); render(rate/4); drums.silence(); drums.seekStep(0);
                auto before=drums.triggerCounts(); float peak=0;
                for(unsigned int step=0;step<WiAudioDrivenDrumMachine::patternStepCount(pattern);++step) {
                    drums.advanceStep(); assert(drums.currentStep()==int(step));
                    peak=std::max(peak,render(std::lround(rate*.125)));
                }
                assert(peak>.001f); // Actual samples play with silent input for every palette.
                for(unsigned int v=0;v<7;++v) {
                    const bool baseVoice = pattern==6 ? (v==1 || v==4) : v==0;
                    assert(drums.triggerCounts()[v]-before[v] == (baseVoice || filter>0 ? expected[pattern][v] : 0));
                    if(filter==0 && !baseVoice) assert(drums.intensities()[v]==0);
                    if(filter==1 && (pattern!=6 || (v!=3 && v!=5))) assert(drums.intensities()[v]>.999f);
                }
                auto count=drums.triggerCounts(); render(rate*4);
                assert(drums.triggerCounts()==count && l==0 && r==0); // No step calls: no new hits.
            }
        }
        // The real-kit fader adds triangle, kick, then snare while tambourine stays present.
        drums.selectPattern(6);
        for(float opening : {0.f,.25f,.5f,.75f,1.f}) {
            drums.setControls(.8f,opening); render(rate/4);
            const auto& gains=drums.intensities();
            assert(gains[1]==1 && gains[4]==1);
            assert(gains[3]==0 && gains[5]==0);
            assert(gains[6]>=gains[2] && gains[2]>=gains[0]);
            if(opening==0) assert(gains[6]==0 && gains[2]==0 && gains[0]==0);
            if(opening==.25f) assert(gains[6]>0 && gains[6]<1 && gains[2]==0 && gains[0]==0);
            if(opening==.5f) assert(gains[6]==1 && gains[2]>0 && gains[2]<1 && gains[0]==0);
            if(opening==.75f) assert(gains[6]==1 && gains[2]==1 && gains[0]>0 && gains[0]<1);
            if(opening==1) assert(gains[6]==1 && gains[2]==1 && gains[0]==1);
        }
        drums.selectPattern(0);
        std::array<float,7> previous{};
        for(float opening : {0.f,.25f,.5f,.75f,1.f}) {
            drums.setControls(.8f,opening); render(rate/4);
            const auto& gains=drums.intensities();
            assert(gains[0]==1);
            for(unsigned int v=0;v<7;++v) { assert(gains[v]+.0001f>=previous[v]); previous[v]=gains[v]; }
            for(unsigned int v=1;v<7;++v) assert(gains[v-1]>=gains[v]);
        }
        drums.selectPattern(0); drums.seekStep(5); drums.advanceStep();
        const int position=drums.currentStep();
        for(unsigned int i=1;i<=7;++i) { drums.nextPattern(); assert(drums.currentPattern()==i%7); assert(drums.currentStep()==position); }
        drums.advanceStep(); assert(drums.currentStep()==6);
        assert(!drums.selectPattern(7));
        drums.seekStep(14); drums.advanceStep();
        drums.selectPattern(6); assert(drums.currentStep()==2);
        drums.advanceStep(); assert(drums.currentStep()==3);
        drums.seekStep(23); drums.advanceStep(); assert(drums.currentStep()==11);
        drums.advanceStep(); assert(drums.currentStep()==0); // Exactly twelve sixteenths per 6/8 bar.
        drums.selectPattern(0);
        drums.setControls(.8f,0); render(rate/4); drums.silence(); drums.seekStep(0);
        drums.advanceStep(); render(rate/20); drums.selectPattern(2);
        assert(render(rate/10)>.00001f); // Lo-fi tail keeps its original sample across the switch.
        WiAudioDrivenDrumMachine reference, changed;
        watch=false;
        assert(reference.setup(rate) && reference.loadSamples(argv[1]));
        assert(changed.setup(rate) && changed.loadSamples(argv[1]));
        watch=true;
        reference.setControls(.8f,1); changed.setControls(.8f,1);
        reference.seekStep(4); changed.seekStep(4);
        reference.advanceStep(); changed.advanceStep(); changed.selectPattern(6);
        for(unsigned int n=0;n<rate/4;++n) {
            float a,b,c,d; reference.process(0,a,b); changed.process(0,c,d);
            assert(a==c && b==d);
        }
        drums.process(NAN,l,r); assert(std::isfinite(l) && std::isfinite(r));
        drums.setControls(NAN,NAN); render(rate/2);
        assert(std::abs(l)<.00001f && std::abs(r)<.00001f);
        watch=false; assert(allocations==0); assert(!drums.setup(0));
    }
    WiPepper pepper; assert(pepper.setup(44100,22050) && pepper.loadDrumSamples(argv[1]));
    std::array<float,8> pots{{(8-.4f)/14,.5f,0,.5f,.8f,1,0,0}}; pepper.setPots(pots);
    MidiInput input; unsigned int sample=0;
    auto render=[&](unsigned int frames,unsigned int clocksEvery=0) {
        for(unsigned int n=0;n<frames;++n,++sample) {
            if(clocksEvery && n%clocksEvery==0) input.process(0xf8,pepper);
            if(sample%2==0) pepper.processControls();
            float in[]={0,0},out[2]; pepper.process(in,out);
            assert(std::isfinite(out[0]) && std::isfinite(out[1]));
        }
    };
    render(44100*4); assert(std::abs(pepper.tempoBpm()-120)<.2f && pepper.drums().triggerCounts()[0]>0);
    input.process(0xfa,pepper); render(2,1225); assert(pepper.drums().currentStep()==0);
    render(44100,1225); assert(pepper.externalMidiClock() && std::abs(pepper.tempoBpm()-90)<.01f);
    pots[0]=0; pepper.setPots(pots); render(44100,1225); assert(std::abs(pepper.tempoBpm()-90)<.01f);
    auto count=pepper.drums().triggerCounts(); int step=pepper.drums().currentStep();
    input.process(0xfc,pepper); render(44100,1225);
    assert(pepper.drums().triggerCounts()==count && pepper.drums().currentStep()==step);
    input.process(0xfb,pepper); render(88200,1225); assert(pepper.drums().triggerCounts()[0]>count[0]);
    count=pepper.drums().triggerCounts(); step=pepper.drums().currentStep(); render(44100);
    assert(pepper.drums().triggerCounts()==count && pepper.drums().currentStep()==step);
    pepper.pressButton(1); pepper.pressButton(3); assert(pepper.drums().currentPattern()==1);
    render(44100,1225); assert(pepper.drums().triggerCounts()==count);
    pepper.pressButton(1); render(44100,1225); assert(pepper.drums().triggerCounts()[0]>count[0]);
    for(uint8_t byte : {0xf2,14,0}) input.process(byte,pepper);
    render(2,1225); assert(pepper.drums().currentStep()==14);
    pepper.pressButton(2); pepper.pressButton(3); assert(pepper.drums().currentPattern()==2 && pepper.sequenceLocked());
    pepper.pressButton(3); assert(pepper.drums().currentPattern()==3 && pepper.rimDrumPattern());
    for(int i=0;i<3;++i) pepper.pressButton(3);
    assert(pepper.drums().currentPattern()==6 && !pepper.rimDrumPattern());
    for(uint8_t byte : {0xf2,23,0}) input.process(byte,pepper);
    render(2,1225); assert(pepper.drums().currentStep()==11);
    input.process(0xfa,pepper);
    count=pepper.drums().triggerCounts();
    render(88200,1225); // One 6/8 bar at 90 quarter-note BPM: 72 clock pulses in two seconds.
    assert(pepper.drums().currentStep()==11);
    for(unsigned int v=0;v<7;++v) assert(pepper.drums().triggerCounts()[v]-count[v]==expected[6][v]);
    render(2,1225); assert(pepper.drums().currentStep()==0);
    pepper.pressButton(0); count=pepper.drums().triggerCounts(); render(44100,1225);
    assert(pepper.drums().triggerCounts()==count);
    puts("Seven grooves including 6/8 real kit, all tambourines, filter sweep, audio, transport and allocation tests passed");
}
