#include "GranularReverb.hpp"

#include <cmath>
#include <cstdio>
#include <stdlib.h>

static void initialize_delay_array(int random_delays[GRANULAR_NUM_GRAINS][GRANULAR_REPS_PER_GRAIN])
{
    for (unsigned int k = 0; k < GRANULAR_NUM_GRAINS; k++) {
        for (unsigned int l = 0; l < GRANULAR_REPS_PER_GRAIN; l++) {
            random_delays[k][l] = (int)(rand() % GRANULAR_GRAIN_CANVAS_SIZE);
        }
    }
}

static void initialize_amp_array(float random_amps[GRANULAR_NUM_GRAINS][GRANULAR_REPS_PER_GRAIN])
{
    for (unsigned int k = 0; k < GRANULAR_NUM_GRAINS; k++) {
        for (unsigned int l = 0; l < GRANULAR_REPS_PER_GRAIN; l++) {
            random_amps[k][l] = (float)(rand() % GRANULAR_GRAIN_CANVAS_SIZE) / GRANULAR_GRAIN_CANVAS_SIZE;
        }
    }
}

static void init_speed_array(float speed[GRANULAR_NUM_GRAINS][GRANULAR_REPS_PER_GRAIN])
{
    srand((unsigned int)time(NULL)); // new seed each run

    for (int i = 0; i < GRANULAR_NUM_GRAINS; i++) {
        for (int j = 0; j < GRANULAR_REPS_PER_GRAIN; j++) {

            float r = (float)rand() / (RAND_MAX + 1.0f); // random between 0–1

            if (r < 0.50f)
                speed[i][j] = 1.0f;
            else if (r < 0.70f)
                speed[i][j] = 0.5f;
            else if (r < 0.90f)
                speed[i][j] = 2.0f;
            else if (r < 0.95f)
                speed[i][j] = 0.25f;
            else
                speed[i][j] = 4.0f;
        }
    }
}

static float hann_function(int n, int total_length)
{
    return ((cos((float(n) / total_length) * 2 * M_PI - M_PI) + 1) / 2);
}

static float square_with_ramps(int n, int total_length, float ramp_fraction)
{
    float ramp_length = total_length * ramp_fraction;

    if (n < 0) return 0.0f;
    if (n >= total_length) return 0.0f;

    if (n < ramp_length) {
        // Ramp in
        return n / ramp_length;
    } else if (n < total_length - ramp_length) {
        // Flat top
        return 1.0f;
    } else {
        // Ramp out
        return (total_length - n) / ramp_length;
    }
}

GranularReverb::GranularReverb()
{
    srand((unsigned int)time(NULL));
    initialize_delay_array(random_delays);
    initialize_amp_array(random_amps_l);
    initialize_amp_array(random_amps_r);
    init_speed_array(speed);
}

void GranularReverb::process(float in, float out[2])
{
    // Increment grain write pointer and grain_number
    grain_write_ptr++;
    if (grain_write_ptr >= GRANULAR_GRAIN_SIZE) {
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
        int r = (int)((float)rand() / (RAND_MAX + 1.0f) * GRANULAR_NUM_GRAINS);
        grain_number = r;

        if (grain_number == GRANULAR_NUM_GRAINS - 1) {
            grain_number = 0;
            initialize_delay_array(random_delays);
            initialize_amp_array(random_amps_l);
            initialize_amp_array(random_amps_r);
            printf("Reinit\n");
        }

    }

    // Incement main canvas writer pointer
    canvas_write_ptr++;
    if (canvas_write_ptr >= GRANULAR_GRAIN_CANVAS_SIZE) {
        canvas_write_ptr = 0;
    }

    // Read audio inputs
    if (!true_freeze) {
        float feedback = true_freeze || defrosting ? feedback_ramp.process() : feedback_knob;
        grain_array[grain_number][grain_write_ptr] *= feedback;

        // To allow slow playback (0.5x), the grain array needs to be zero padded because it loops halfway through the grain array
        // Its nice to have one common grain write pointer for all the speeds.
        float grain_sample = grain_write_ptr < (GRANULAR_GRAIN_SIZE / 2) ? hann_function(grain_write_ptr, GRANULAR_GRAIN_SIZE / 2/*, 0.005*/) * in : 0.0f;
        grain_array[grain_number][grain_write_ptr] += grain_sample;
    }

    // grain_array_buffer[grain_write_ptr][grain_number] = hann_function(grain_write_ptr, GRANULAR_GRAIN_SIZE) * in;

    for (unsigned int i = 0; i < GRANULAR_NUM_GRAINS; i++) {
        unsigned int delay_samples = random_delays[grain_number][i];
        float long_delay_comp = (float)(GRANULAR_GRAIN_CANVAS_SIZE - delay_samples) / GRANULAR_GRAIN_CANVAS_SIZE;
        //printf("Delay comp: %f\n", long_delay_comp);
        float playback_speed = speed[grain_number][i];
        grain_canvas_l[(canvas_write_ptr + delay_samples) % GRANULAR_GRAIN_CANVAS_SIZE] += random_amps_l[grain_number][i] * grain_array[(grain_number) % GRANULAR_NUM_GRAINS][(int)(playback_speed * grain_write_ptr) % GRANULAR_GRAIN_SIZE] * long_delay_comp;
        grain_canvas_r[(canvas_write_ptr + delay_samples) % GRANULAR_GRAIN_CANVAS_SIZE] += random_amps_r[grain_number][i] * grain_array[(grain_number) % GRANULAR_NUM_GRAINS][(int)(playback_speed * grain_write_ptr) % GRANULAR_GRAIN_SIZE] * long_delay_comp;
    }


    out[0] = level * grain_canvas_l[canvas_write_ptr] + in;
    out[1] = level * grain_canvas_r[canvas_write_ptr] + in;

    // CLear canvas (circular buffer) after sample is played. Since the grain is still in the grain_array it is not needed on the canvas anymore.
    // It will be redrawn in the future.
    grain_canvas_l[(canvas_write_ptr) % GRANULAR_GRAIN_CANVAS_SIZE] = 0;
    grain_canvas_r[(canvas_write_ptr) % GRANULAR_GRAIN_CANVAS_SIZE] = 0;
}

// void GranularReverb::setAnalogIns(AnalogIns ins)
// {

//     amplitude = ins.input_1;

//     if (!true_freeze)
//         grain_size = map(ins.input_2, 0, 1, 500, GRANULAR_GRAIN_SIZE);

//     if (!true_freeze)
//         feedback_knob = map(ins.input_3, 0, 1, 0, 1.5);

//     gain_reps = map(ins.input_4, 0, 1, 2, GRANULAR_REPS_PER_GRAIN);
//     freeze = ins.input_7 > 0.4;
//     level = map(ins.input_5, 0, 1, 0, 1.5);
// }




