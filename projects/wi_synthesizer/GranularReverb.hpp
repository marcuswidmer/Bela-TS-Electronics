#ifndef GRANULAR_REVERB_HPP
#define GRANULAR_REVERB_HPP

#include <string>
#include "Ramp.h"


#define GRANULAR_GRAIN_SIZE 5000 // 100 ms
#define GRANULAR_REPS_PER_GRAIN 220
#define GRANULAR_NUM_GRAINS 9
#define GRANULAR_GRAIN_CANVAS_SIZE 22050 // 0.5 seconds

class GranularReverb
{
public:
    GranularReverb();
    void process(float in, float out[2]);

private:
    float grain_canvas_l[GRANULAR_GRAIN_CANVAS_SIZE] = {};
    float grain_canvas_r[GRANULAR_GRAIN_CANVAS_SIZE] = {};

    float random_amps_l[GRANULAR_NUM_GRAINS][GRANULAR_REPS_PER_GRAIN];
    float random_amps_r[GRANULAR_NUM_GRAINS][GRANULAR_REPS_PER_GRAIN];

    float grain_array[GRANULAR_NUM_GRAINS][GRANULAR_GRAIN_SIZE] = {};
    float speed[GRANULAR_NUM_GRAINS][GRANULAR_REPS_PER_GRAIN] = {};
    int random_delays[GRANULAR_NUM_GRAINS][GRANULAR_REPS_PER_GRAIN];

    float feedback_knob = 0.3;
    float level = 0.3;

    unsigned int grain_write_ptr = 0;
    unsigned int grain_number = 0;

    bool freeze = false;
    bool true_freeze = false;
    bool defrosting = false;

    int canvas_write_ptr = 0;

    Ramp feedback_ramp;
};

#endif
