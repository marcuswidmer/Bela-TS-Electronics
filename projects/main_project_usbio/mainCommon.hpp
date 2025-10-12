#ifndef MAIN_COMMON_H
#define MAIN_COMMON_H

#define AUDIOSAMPLERATE_HZ 44100
#define PYTHON_MOD(n,M) (((n % M) + M) % M)

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
