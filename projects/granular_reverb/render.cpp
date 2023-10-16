/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
    Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
    Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/

// Simple Delay on Audio Input with Low Pass Filter

#include <Bela.h>
#include <cmath>
#include <stdlib.h>
#include "../main_project/mainCommon.hpp"

#define MAX_GRAIN_SIZE 44100 // 100 ms

#define MAX_REPS_PER_GRAIN 120
#define NUM_GRAINS 10

#define GRAIN_CANVAS_SIZE 22050 // 0.5 seconds

//
float grain_canvas_l[GRAIN_CANVAS_SIZE] = {};
float grain_canvas_r[GRAIN_CANVAS_SIZE] = {};

// Buffer containing random delays
int random_delays_l[NUM_GRAINS][MAX_REPS_PER_GRAIN] = {};
int random_delays_r[NUM_GRAINS][MAX_REPS_PER_GRAIN] = {};

// Buffer containing random amps
float random_amps_l[NUM_GRAINS][MAX_REPS_PER_GRAIN] = {};
float random_amps_r[NUM_GRAINS][MAX_REPS_PER_GRAIN] = {};

float grain_array[NUM_GRAINS][MAX_GRAIN_SIZE] = {};

float grain_sample = 0;
float amplitude = 0;
float feedback = 0;

unsigned int grain_index = 0;
unsigned int delay_samples_l = 0;
unsigned int delay_samples_r = 0;
unsigned int grain_size;
unsigned int grain_write_ptr = 0;
unsigned int freeze_fade_ptr = 0;
unsigned int grain_number = 0;
unsigned int gain_reps = 100;

bool freeze = false;
bool true_freeze = false;

// Marcus destroyed again
int gSensorInput0 = 6;
int gSensorInput1 = 7;
int gSensorInput2 = 3;
int gSensorInput3 = 5;
int gSensorInput4 = 0;
int gSensorInput5 = 2;
int gSensorInput6 = 4;
int gSensorInput7 = 1;

int gAudioFramesPerAnalogFrame = 0;
int canvas_write_ptr = -1;

// Butterworth coefficients for low-pass filter @ 8000Hz
float gDel_a0 = 0.1772443606634904;
float gDel_a1 = 0.3544887213269808;
float gDel_a2 = 0.1772443606634904;
float gDel_a3 = -0.5087156198145868;
float gDel_a4 = 0.2176930624685485;

// Previous two input and output values for each channel (required for applying the filter)
float gDel_x1_l = 0;
float gDel_x2_l = 0;
float gDel_y1_l = 0;
float gDel_y2_l = 0;
float gDel_x1_r = 0;
float gDel_x2_r = 0;
float gDel_y1_r = 0;
float gDel_y2_r = 0;

static void initialize_delay_array(int random_delays[NUM_GRAINS][MAX_REPS_PER_GRAIN])
{
    for (unsigned int k = 0; k < NUM_GRAINS; k++) {
        for (unsigned int l = 0; l < MAX_REPS_PER_GRAIN; l++) {
            random_delays[k][l] = (int)(rand() % GRAIN_CANVAS_SIZE);
        }
    }
}

static void initialize_amp_array(float random_amps[NUM_GRAINS][MAX_REPS_PER_GRAIN])
{
    for (unsigned int k = 0; k < NUM_GRAINS; k++) {
        for (unsigned int l = 0; l < MAX_REPS_PER_GRAIN; l++) {
            random_amps[k][l] = (float)(rand() % GRAIN_CANVAS_SIZE) / GRAIN_CANVAS_SIZE;
        }
    }
}

static float hann_function(int n, int total_length)
{
    return ((cos((float(n) / total_length) * 2 * M_PI - M_PI) + 1) / 2);
}

bool setup(BelaContext *context, void *userData)
{
    initialize_delay_array(random_delays_l);
    initialize_delay_array(random_delays_r);
    initialize_amp_array(random_amps_l);
    initialize_amp_array(random_amps_r);

    grain_size = 4410;

    // Useful calculations
    if(context->analogFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    return true;
}

void render(BelaContext *context, void *userData)
{
    for(unsigned int n = 0; n < context->audioFrames; n++) {
        float out_l = 0.0f;
        float out_r = 0.0f;
        float input_sample = 0.0f;

        if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            struct AnalogIns ins = {
                .input_0 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0),
                .input_1 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1),
                .input_2 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2),
                .input_3 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3),
                .input_4 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4),
                .input_5 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5),
                .input_6 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6),
                .input_7 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7)
            };

            amplitude = ins.input_1;
            
            if (!true_freeze)
                grain_size = map(ins.input_2, 0, 1, 500, MAX_GRAIN_SIZE);
            
            feedback = map(ins.input_3, 0, 1, 0, 1.5);
            gain_reps = map(ins.input_4, 0, 1, 2, MAX_REPS_PER_GRAIN);
            freeze = ins.input_7 > 0.4;
            
            // if (true_freeze)
            //     feedback = 1.0f;

        }

        // Increment grain write pointer and grain_number
        grain_write_ptr++;
        if (grain_write_ptr >= grain_size) {
            true_freeze = freeze;

            grain_write_ptr = 0;
            grain_number++;
            
            if (grain_number >= NUM_GRAINS) {
                grain_number = 0;
            }
        }

        // Incement main canvar writer pointer
        canvas_write_ptr++;
        if (canvas_write_ptr >= GRAIN_CANVAS_SIZE) {
            //printf("Max sample lately: %f\n", out_l_prev);
            out_l_prev = 0;
            canvas_write_ptr = 0;
        }

        // Read audio inputs
        if (!true_freeze) {
            input_sample = audioRead(context,n,0);
            grain_sample = hann_function(grain_write_ptr, grain_size) * input_sample;
            grain_array[grain_number][grain_write_ptr] = grain_sample;
        }

        // grain_array_buffer[grain_write_ptr][grain_number] = hann_function(grain_write_ptr, grain_size) * input_sample;

        for (unsigned int i = 0; i < gain_reps; i++) {
            delay_samples_l = random_delays_l[grain_index][i]; // BUG!
            grain_canvas_l[(canvas_write_ptr + delay_samples_l) % GRAIN_CANVAS_SIZE] += random_amps_l[grain_number][i] * grain_array[grain_number][grain_write_ptr];
       
            delay_samples_r = random_delays_r[grain_index][i];
            grain_canvas_r[(canvas_write_ptr + delay_samples_r) % GRAIN_CANVAS_SIZE] += random_amps_r[grain_number][i] * grain_array[grain_number][grain_write_ptr];
        }

        out_l = amplitude * (grain_canvas_l[canvas_write_ptr] + input_sample);
        out_r = amplitude * (grain_canvas_r[canvas_write_ptr] + input_sample);

        grain_canvas_l[(canvas_write_ptr) % GRAIN_CANVAS_SIZE] *= feedback;
        grain_canvas_r[(canvas_write_ptr) % GRAIN_CANVAS_SIZE] *= feedback;
        
        audioWrite(context, n, 0, out_l);
        audioWrite(context, n, 1, out_r);
    }   
}



void cleanup(BelaContext *context, void *userData)
{

}
