#ifndef LO_FI_MACHINE_HPP
#define LO_FI_MACHINE_HPP

#include <Bela.h>
#include <cmath>
#include <libraries/AudioFile/AudioFile.h>
#include <libraries/Biquad/Biquad.h>

#include <string>

#define LO_FI_MACHINE_MAX_DELAY (10 * 44100)
#define LO_FI_MACHINE_N_VOLUMES 100
#define LO_FI_MACHINE_DIP_LENGTH (2 * 44100)
#define LO_FI_MACHINE_RISE_LENGTH (2 * 44100)

struct AnalogIns;

class LoFiMachine
{
public:
    LoFiMachine();

    void process(float in, float out[2]);
    void setSampleRateAndFilterSettings(const float sampleRate);
    void setAnalogIns(AnalogIns ins);
private:
	void tapeStartStop();
	float tapeHiss(float amp);

	float fs;

    float amplitude;
    float wear;
    float vibDepth;
    float vibSpeed;
    float blend;
    float saturation;

    Biquad lpFilter;
    Biquad hpFilter;
    BiquadCoeff::Settings hpSettings;
    BiquadCoeff::Settings lpSettings;

    float lpCutoff;
    float hpCutoff;

    float gPhase;

    float delayLine[LO_FI_MACHINE_MAX_DELAY];
    int delayWriteCnt;

    bool doTapeStop;
    float dipper;

    bool dipStarted;
    int dipCounter;
    float dipVolume;
    float dipVolumes[LO_FI_MACHINE_N_VOLUMES];
	float dipValues[LO_FI_MACHINE_DIP_LENGTH];

	bool riseStarted;
	int riseCounter;
	float riseVolume;
	float riseVolumes[LO_FI_MACHINE_N_VOLUMES];
	float riseValues[LO_FI_MACHINE_RISE_LENGTH];

 	float startSampleOut;
	bool startSampleStarted;
 	int startSampleCounter;
 	float stopSampleOut;
 	bool stopSampleStarted;
    int stopSampleCounter;

	int numFramesInTapeStopFile;
	int numFramesInTapeStartFile;
	int numFramesInHissFile;

    int globCounter;
    float wearComp;
    int hissCounter;

	std::vector<float> gHissData;
	std::vector<float> gTapeStopData;
	std::vector<float> gTapeStartData;
};

#endif
