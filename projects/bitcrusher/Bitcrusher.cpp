#include "Bitcrusher.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

Bitcrusher::Bitcrusher()
{
}

float Bitcrusher::process(float inSamp)
{
    int scaledInSamp = 127 * inSamp;

    return scaledInSamp / (float)(scale_);
}

void Bitcrusher::setAnalogIns(AnalogIns ins)
{
    levels_ = map(ins.input_1, 0, 1, 1, 10);
    scale_ = map(ins.input_2, 0, 1, 1, 300);
}
