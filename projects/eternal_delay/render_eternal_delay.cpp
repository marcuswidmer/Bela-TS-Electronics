// // /*
//  ____  _____ _        _
// | __ )| ____| |      / \
// |  _ \|  _| | |     / _ \
// | |_) | |___| |___ / ___ \
// |____/|_____|_____/_/   \_\

// */

// #include <Bela.h>
// #include <cmath>
// #include <libraries/AudioFile/AudioFile.h>

// #include "EternalDelay.hpp"
// #include "mainCommon.hpp"

// #define NUM_CHANNELS 1    // NUMBER OF CHANNELS IN THE FILE

// std::string gFilename = "../low-fi-machine/jazz_lick.wav";

// int gNumFramesInFile;

// // INPUT MAPPING!!! Wierd because I destroyed the Bela
// int gSensorInput0 = 5;
// int gSensorInput1 = 6;
// int gSensorInput2 = 1;
// int gSensorInput3 = 4;
// int gSensorInput4 = 2;
// int gSensorInput5 = 3;
// int gSensorInput6 = 7;
// int gSensorInput7 = 0;

// int gAudioFramesPerAnalogFrame = 0;
// float gInverseSampleRate;
// float amplitude = 1.0f;

// std::vector<float> gSampleData;

// EternalDelay eternalDelay;

// static float readAudioTestInput()
// {
//     // Increment read pointer and reset to 0 when end of file is reached
//     static int readCounter;
//     float out = gSampleData[readCounter];
//     readCounter++;
//     if (readCounter >= gNumFramesInFile)
//         readCounter = 0;

//     return out;
// }

// static void abortIfLoud(float y)
// {
//     if (fabs(y) > 3) {
//         printf("Too loud! out: %f\n", y);
//         abort();
//     }
// }

// bool setup(BelaContext *context, void *userData)
// {
//     Bela_setADCLevel(-6.0); //This used to be -6. So changing to the old value to deal with guitar output level.

//     gInverseSampleRate = 1.0 / context->audioSampleRate;

//     if(context->audioFrames)
//         gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

//     gNumFramesInFile = AudioFileUtilities::getNumFrames(gFilename);
//     gSampleData = AudioFileUtilities::load(gFilename, gNumFramesInFile, 0)[0];

//     return true;
// }

// void render(BelaContext *context, void *userData)
// {
//     for (unsigned int n = 0; n < context->audioFrames; n++) {
//         if (gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
//             struct AnalogIns ins = {
//                 .input_0 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput0),
//                 .input_1 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput1),
//                 .input_2 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput2),
//                 .input_3 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput3),
//                 .input_4 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput4),
//                 .input_5 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput5),
//                 .input_6 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput6),
//                 .input_7 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInput7)
//             };

//             eternalDelay.setAnalogIns(ins);
//             amplitude = map(ins.input_0, 0, 1, 0, 1);
//         }


//         //float out = readAudioTestInput();
//         float out = audioRead(context, n, 0);
//         float eOut = eternalDelay.process(out);
//         float yOut = amplitude * eOut;

//         abortIfLoud(yOut);

//         audioWrite(context, n, 0, yOut);
//         audioWrite(context, n, 1, yOut);

//     }
// }


// void cleanup(BelaContext *context, void *userData)
// {
// }
