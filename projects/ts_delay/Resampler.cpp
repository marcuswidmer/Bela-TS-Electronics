#include "Resampler.hpp"
#include <stdio.h>
#include <math.h>
#include <Bela.h>

// float tukeyWindowFunction(int index, int N)
// {
//     float alpha = 0.1f;
//     int n;

//     if (index > N / 2)
//         n = N - index;
//     else
//         n = index;

//     if (n >= 0 && n < (alpha * N / 2))
//         return 0.5f * (1.0f - cosf(2.0f * M_PI * index / (alpha * N)));
//     else if (n >= (alpha * N / 2) && n <= N / 2)
//         return 1.0f;
// }

Resampler::Resampler()
{
    timeDomainIn_ = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESAMPLER_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    timeDomainOut_ = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESAMPLER_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    frequencyDomain_ = (ne10_fft_cpx_float32_t*) NE10_MALLOC (RESAMPLER_FFTSIZE * sizeof (ne10_fft_cpx_float32_t));
    cfg_ = ne10_fft_alloc_c2c_float32_neon (RESAMPLER_FFTSIZE);
}

Resampler::~Resampler()
{
    NE10_FREE(timeDomainIn_);
    NE10_FREE(timeDomainOut_);
    NE10_FREE(frequencyDomain_);
    NE10_FREE(cfg_);
}

void Resampler::process(float *inBuffer, float *outBuffer, int size)
{
    // Copy buffer into FFT input
    printf("Running\n");
    for (int n = 0; n < RESAMPLER_FFTSIZE; n++) {
        if (n < size) {
            timeDomainIn_[n].r = (ne10_float32_t) inBuffer[n];
        } else {
            timeDomainIn_[n].r = 0.0f;
        }

        timeDomainIn_[n].i = 0.0f;
    }

    // Run the FFT
    ne10_fft_c2c_1d_float32_neon(frequencyDomain_, timeDomainIn_, cfg_, 0);

    // Run the iFFT
    ne10_fft_c2c_1d_float32_neon(timeDomainOut_, frequencyDomain_, cfg_, 1);

    for (int n = 0; n < size; n++) {
        outBuffer[n] = timeDomainOut_[n].r;
    }
}
