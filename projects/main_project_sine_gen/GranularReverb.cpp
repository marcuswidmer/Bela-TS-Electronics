#include "GranularReverb.hpp"

#include <Bela.h>
#include <cmath>
#include <stdlib.h>
#include "../main_project/mainCommon.hpp"

static void initialize_delay_array(int random_delays[GRANULAR_NUM_GRAINS][GRANULAR_MAX_REPS_PER_GRAIN])
{
    for (unsigned int k = 0; k < GRANULAR_NUM_GRAINS; k++) {
        for (unsigned int l = 0; l < GRANULAR_MAX_REPS_PER_GRAIN; l++) {
            random_delays[k][l] = (int)(rand() % GRANULAR_GRAIN_CANVAS_SIZE);
        }
    }
}

static void initialize_amp_array(float random_amps[GRANULAR_NUM_GRAINS][GRANULAR_MAX_REPS_PER_GRAIN])
{
    for (unsigned int k = 0; k < GRANULAR_NUM_GRAINS; k++) {
        for (unsigned int l = 0; l < GRANULAR_MAX_REPS_PER_GRAIN; l++) {
            random_amps[k][l] = (float)(rand() % GRANULAR_GRAIN_CANVAS_SIZE) / GRANULAR_GRAIN_CANVAS_SIZE;
        }
    }
}

static float hann_function(int n, int total_length)
{
    return ((cos((float(n) / total_length) * 2 * M_PI - M_PI) + 1) / 2);
}

GranularReverb::GranularReverb()
{
    initialize_delay_array(random_delays_l);
    initialize_delay_array(random_delays_r);
    initialize_amp_array(random_amps_l);
    initialize_amp_array(random_amps_r);
}

void GranularReverb::process(float in, float out[2])
{
    // Increment grain write pointer and grain_number
    grain_write_ptr++;
    if (grain_write_ptr >= grain_size) {
        if (freeze != true_freeze) {
            if (freeze) {
                true_freeze = true;
                feedback_ramp.setValue(feedback_knob);
                feedback_ramp.rampTo(1.0f, 22050);
            } else {
                true_freeze = false;
                defrosting = true;
                feedback_ramp.rampTo(feedback_knob, 22050);
            }
        }

        if (feedback_ramp.finished() && defrosting) {
            defrosting = false;
        }

        grain_write_ptr = 0;
        grain_number++;

        if (grain_number >= GRANULAR_NUM_GRAINS) {
            grain_number = 0;
        }
    }

    // Incement main canvas writer pointer
    canvas_write_ptr++;
    if (canvas_write_ptr >= GRANULAR_GRAIN_CANVAS_SIZE) {
        canvas_write_ptr = 0;
    }

    // Read audio inputs
    if (!true_freeze) {
        float grain_sample = hann_function(grain_write_ptr, grain_size) * in;
        grain_array[grain_number][grain_write_ptr] = grain_sample;
    }

    // grain_array_buffer[grain_write_ptr][grain_number] = hann_function(grain_write_ptr, grain_size) * in;

    for (unsigned int i = 0; i < gain_reps; i++) {
        delay_samples_l = random_delays_l[grain_number][i];
        grain_canvas_l[(canvas_write_ptr + delay_samples_l) % GRANULAR_GRAIN_CANVAS_SIZE] += random_amps_l[grain_number][i] * grain_array[grain_number][grain_write_ptr];

        delay_samples_r = random_delays_r[grain_number][i];
        grain_canvas_r[(canvas_write_ptr + delay_samples_r) % GRANULAR_GRAIN_CANVAS_SIZE] += random_amps_r[grain_number][i] * grain_array[grain_number][grain_write_ptr];
    }


    out[0] = amplitude * (level * grain_canvas_l[canvas_write_ptr] + in);
    out[1] = amplitude * (level * grain_canvas_r[canvas_write_ptr] + in);

    float feedback = true_freeze || defrosting ? feedback_ramp.process() : feedback_knob;

    grain_canvas_l[(canvas_write_ptr) % GRANULAR_GRAIN_CANVAS_SIZE] *= feedback;
    grain_canvas_r[(canvas_write_ptr) % GRANULAR_GRAIN_CANVAS_SIZE] *= feedback;
}

void GranularReverb::setAnalogIns(AnalogIns ins)
{

    amplitude = ins.input_1;

    if (!true_freeze)
        grain_size = map(ins.input_2, 0, 1, 500, GRANULAR_MAX_GRAIN_SIZE);

    if (!true_freeze)
        feedback_knob = map(ins.input_3, 0, 1, 0, 1.5);

    gain_reps = map(ins.input_4, 0, 1, 2, GRANULAR_MAX_REPS_PER_GRAIN);
    freeze = ins.input_7 > 0.4;
    level = map(ins.input_5, 0, 1, 0, 1.5);
}




