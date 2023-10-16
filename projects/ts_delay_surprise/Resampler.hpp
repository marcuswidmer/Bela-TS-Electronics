#ifndef RESAMPLER_HPP
#define RESAMPLER_HPP

#include <libraries/ne10/NE10.h> // NEON FFT library
#include <Bela.h>

#define RESAMPLER_FFTSIZE (65536) // 1 seconds ish

class Resampler
{
public:
    Resampler();
    ~Resampler();
    void process(float *inBuffer, float *outBuffer, int size);

private:
	ne10_fft_cpx_float32_t* timeDomainIn_;
	ne10_fft_cpx_float32_t* timeDomainOut_;
	ne10_fft_cpx_float32_t* frequencyDomain_;
	ne10_fft_cfg_float32_t cfg_;
};

#endif
