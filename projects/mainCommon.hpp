#ifndef MAIN_COMMON_H
#define MAIN_COMMON_H

#define AUDIOSAMPLERATE_HZ 44100
#define PYTHON_MOD(n,M) (((n % M) + M) % M)
#define PROGRAM_LEVEL_0 0.000000
#define PROGRAM_LEVEL_1 0.274918
#define PROGRAM_LEVEL_2 0.549789
#define PROGRAM_LEVEL_3 0.825043
#define POT_COMP_FACTOR 0.825577

struct AnalogIns {
	float input_0;
	float input_1;
	float input_2;
	float input_3;
	float input_4;
	float input_5;
	float input_6;
	float input_7;
};

#endif
