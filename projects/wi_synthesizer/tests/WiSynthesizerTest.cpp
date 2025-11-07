#include "../WiSynthesizer.hpp"
#ifdef USE_NO_MIDI
#include <gtest/gtest.h>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdint>

static void writeWavFile(const std::string& filename,
                         const std::vector<float>& interleaved,
                         int sampleRate)
{
    // Simple 16-bit PCM WAV writer
    int numChannels = 2;
    int bitsPerSample = 16;
    int byteRate = sampleRate * numChannels * bitsPerSample / 8;
    int blockAlign = numChannels * bitsPerSample / 8;
    int dataSize = interleaved.size() * bitsPerSample / 8;
    int chunkSize = 36 + dataSize;

    std::ofstream f(filename, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open wav file for writing.");

    // RIFF header
    f.write("RIFF", 4);
    uint32_t val32 = chunkSize; f.write((char*)&val32, 4);
    f.write("WAVE", 4);

    // fmt chunk
    f.write("fmt ", 4);
    val32 = 16; f.write((char*)&val32, 4); // fmt chunk size
    uint16_t val16 = 1; f.write((char*)&val16, 2); // PCM
    val16 = numChannels; f.write((char*)&val16, 2);
    val32 = sampleRate; f.write((char*)&val32, 4);
    val32 = byteRate; f.write((char*)&val32, 4);
    val16 = blockAlign; f.write((char*)&val16, 2);
    val16 = bitsPerSample; f.write((char*)&val16, 2);

    // data chunk
    f.write("data", 4);
    val32 = dataSize; f.write((char*)&val32, 4);

    // write samples
    for (float s : interleaved) {
        int16_t v = std::clamp<int>(static_cast<int>(s * 32767.0f), -32768, 32767);
        f.write((char*)&v, 2);
    }
}

TEST(WiSynthesizerTest, singen_the_original_440)
{
    int fs = 44100;
    WiSynthesizer s(fs);

    constexpr float durationSec = 2.0f;
    const int totalSamples = static_cast<int>(fs * durationSec);
    std::vector<float> interleaved;
    interleaved.reserve(totalSamples * 2);

    float out[2];
    int noteOffset = -12;
    s.playNewVoice(69 + noteOffset, 1);
    for (int n = 0; n < totalSamples / 2; ++n) {
        s.process(out);
        interleaved.push_back(out[0]);
        interleaved.push_back(out[1]);
        //EXPECT_NEAR(out[0], out[1], 1e-7);
    }

    s.playNewVoice(74 + noteOffset, 1);
    for (int n = 0; n < totalSamples / 2; ++n) {
        s.process(out);
        interleaved.push_back(out[0]);
        interleaved.push_back(out[1]);
    }

    s.playNewVoice(72 + noteOffset, 1);
    for (int n = 0; n < totalSamples / 2; ++n) {
        s.process(out);
        interleaved.push_back(out[0]);
        interleaved.push_back(out[1]);
    }


    for (int n = 0; n < totalSamples; ++n) {
        s.process(out);
        interleaved.push_back(out[0]);
        interleaved.push_back(out[1]);
    }
    s.releaseVoice(69 + noteOffset);
    s.releaseVoice(74 + noteOffset);
    s.releaseVoice(72 + noteOffset);

    for (int n = 0; n < totalSamples; ++n) {
        s.process(out);
        interleaved.push_back(out[0]);
        interleaved.push_back(out[1]);
    }

    // Compute absolute max (peak)
    float absMax = 0.0f;
    for (float v : interleaved)
        absMax = std::max(absMax, std::abs(v));

    std::cout << "🔊 Peak amplitude: " << absMax
              << " (" << absMax * 100.0f << "% of full scale)\n";

    if (absMax > 1.0f)
        std::cerr << "⚠️ WARNING: Clipping detected!\n";
    else if (absMax > 0.5f)
        std::cerr << "⚠️ Volume is high — be careful with playback.\n";
    writeWavFile("singen_the_original.wav", interleaved, fs);

    // Sanity check
    EXPECT_TRUE(std::filesystem::exists("singen_the_original.wav"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
