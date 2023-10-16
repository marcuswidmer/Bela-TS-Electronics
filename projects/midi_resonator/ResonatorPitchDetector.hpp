#ifndef RESONATORPITCHDETECTOR_HPP
#define RESONATORPITCHDETECTOR_HPP

#define NUM_NOTES 5
#define NUM_OCTAVES 1
#define NUM_VOICES 1

#define n_A (440.00 / 2)
#define n_As (466.16 / 2)
#define n_B (493.88 / 2)
#define n_C (523.25 / 2)
#define n_Cs (554.37 / 2)
#define n_D (587.33 / 2)
#define n_Ds (622.25 / 2)
#define n_E (659.26 / 2)
#define n_F (698.46 / 2)
#define n_Fs (739.99 / 2)
#define n_G (783.99 / 2)
#define n_Gs (830.60 / 2)

//static float f0[NUM_NOTES] = {n_A, n_As, n_B, n_C, n_Cs, n_D, n_Ds, n_E, n_F, n_Fs, n_G, n_Gs};
static float f0[NUM_NOTES] = {n_E, n_G, n_A, n_B, n_D};
static int note_idx[NUM_NOTES] = {7, 10, 0, 2, 5};


#endif
