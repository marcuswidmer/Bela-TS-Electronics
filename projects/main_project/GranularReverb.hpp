#ifndef GRANULAR_REVERB_HPP
#define GRANULAR_REVERB_HPP

#include <string>
#include "mainCommon.hpp"
#include "Ramp.h"


#define GRANULAR_MAX_GRAIN_SIZE 22050 // 100 ms
#define GRANULAR_MAX_REPS_PER_GRAIN 120
#define GRANULAR_NUM_GRAINS 10
#define GRANULAR_GRAIN_CANVAS_SIZE 44100 // 0.5 seconds

class GranularReverb
{
public:
    GranularReverb();
    void process(float in, float out[2]);
    void setAnalogIns(AnalogIns ins);

private:
    float grain_canvas_l[GRANULAR_GRAIN_CANVAS_SIZE];
    float grain_canvas_r[GRANULAR_GRAIN_CANVAS_SIZE];

    int random_delays_l[GRANULAR_NUM_GRAINS][GRANULAR_MAX_REPS_PER_GRAIN];
    int random_delays_r[GRANULAR_NUM_GRAINS][GRANULAR_MAX_REPS_PER_GRAIN];

    float random_amps_l[GRANULAR_NUM_GRAINS][GRANULAR_MAX_REPS_PER_GRAIN];
    float random_amps_r[GRANULAR_NUM_GRAINS][GRANULAR_MAX_REPS_PER_GRAIN];

    float grain_array[GRANULAR_NUM_GRAINS][GRANULAR_MAX_GRAIN_SIZE];

    float amplitude;
    float feedback_knob;
    float level;

    unsigned int delay_samples_l;
    unsigned int delay_samples_r;
    unsigned int grain_size;
    unsigned int grain_write_ptr;
    unsigned int grain_number;
    unsigned int gain_reps;

    bool freeze = false;
    bool true_freeze = false;
    bool defrosting = false;

    int canvas_write_ptr;

    Ramp feedback_ramp;
};

#endif
