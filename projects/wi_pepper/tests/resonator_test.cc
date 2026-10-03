#include "KarplusResonator.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    KarplusResonator r;
    assert(!r.setup(0));
    assert(!r.setup(std::numeric_limits<float>::quiet_NaN()));
    for(float rate : {22050.f, 44100.f, 48000.f, 96000.f}) {
        assert(r.setup(rate));
        std::array<float,8> pots{{1,1,1,1,0,1,1,1}};
        r.setPots(pots);
        float left=0,right=0;
        for(int i=0;i<1000;++i) {
            r.process(0,left,right);
            assert(left==0 && right==0);
        }
        r.pressButton(0);
        double energy=0, difference=0;
        for(int i=0;i<static_cast<int>(rate);++i) {
            r.process(0,left,right);
            assert(std::isfinite(left) && std::isfinite(right));
            assert(std::abs(left)<=1 && std::abs(right)<=1);
            energy+=left*left+right*right;
            difference+=std::abs(left-right);
        }
        assert(energy>0.00001 && difference>0.001);
        // Change tuning and note sets during sustained full-level excitation.
        for(int mode=0;mode<4;++mode) {
            r.pressButton(1);
            r.pressButton(2);
            pots[4]=mode%2;
            pots[3]=mode%2;
            r.setPots(pots);
            for(int i=0;i<static_cast<int>(rate);++i) {
                r.process(std::sin(0.02f*i),left,right);
                assert(std::isfinite(left) && std::isfinite(right));
                assert(std::abs(left)<=1 && std::abs(right)<=1);
            }
        }
        r.pressButton(3);
        for(int i=0;i<3000;++i) {
            r.process(0,left,right);
            assert(left==0 && right==0);
        }
        // Dry signal is duplicated at unity after the smoothing settles.
        pots[5]=0;
        r.setPots(pots);
        for(int i=0;i<static_cast<int>(rate);++i) r.process(0.25f,left,right);
        assert(std::abs(left-0.25f)<0.0001f && std::abs(right-0.25f)<0.0001f);
    }
    std::cout << "Resonator tests passed\n";
}
