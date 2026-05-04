#include "../WiModular.hpp"
#include <cassert>
#include <cstdlib>
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

TEST(WiModularTest, create_init)
{
    WiModular m;
    m.init(1000);
    m.analogIO.selector = PgmRandomOctave;
    //m.analogIO.pot0 = 0.1666;
    m.analogIO.pot0 = 0.1;
    m.analogIO.pot1 = 1.0f; // 5 octaves
    m.analogIO.pot2 = 0.4f;

    std::vector<float> interleaved;
    for (int i = 0; i < 10000; ++i) { // 10s
        m.analogIO.button = (i > 5000 && i < 5500) or (i > 6000 && i < 6500);

        m.process();
        interleaved.push_back(m.analogIO.cvOut0);
        interleaved.push_back(m.analogIO.cvOut1);
    }

    writeWavFile("cvOut0.wav", interleaved, 1000);
}

TEST(WiModularTest, test_bench)
{
    WiModular m;
    m.init(1000);
    m.analogIO.selector = PgmOctaveSelector;
    m.analogIO.pot1 = 0.2; // Should be one octave up

    std::vector<float> interleaved;
    for (int i = 0; i < 1000; ++i) { // 10s
        m.process();
        interleaved.push_back(m.analogIO.cvOut0);
        interleaved.push_back(m.analogIO.cvOut1);
    }
    //ASSERT_NEAR(m.analogIO.cvOut0, 0.2, 1e-06);

    for (int i = 0; i < 1000; ++i) { // 10s
        m.analogIO.pot0 = i / 1000.0f;
        m.process();
        interleaved.push_back(m.analogIO.cvOut0);
        interleaved.push_back(m.analogIO.cvOut1);
    }

    writeWavFile("cvOut0Test.wav", interleaved, 1000);
}

TEST(WiModularTest, sequencer)
{
    WiModular m;
    m.init(1000);
    m.analogIO.selector = PgmSequencer;
    m.analogIO.pot0 = 0.4;

    //Assign mode
    m.analogIO.button = true;
    m.process();
    m.analogIO.button = false;
    m.process();

    m.assignNote(13);
    m.assignNote(40);

    //Leave Assign mode
    m.analogIO.button = true;
    m.process();
    m.analogIO.button = false;
    m.process();


    std::vector<float> interleaved;
    for (int i = 0; i < 50000; ++i) { // 10s
        // if (i > 1000)
        //     m.analogIO.pot0 = 0.9;

        m.process();
        interleaved.push_back(m.analogIO.cvOut0);
        interleaved.push_back(m.analogIO.cvOut1);
    }

    writeWavFile("cvOut0Seq.wav", interleaved, 1000);
}

TEST(WiModularTest, sampler)
{
    WiModular m;
    m.init(1000, 44100);
    m.analogIO.selector = PgmSampler;
    m.analogIO.pot0 = 0.0f;

    m.process();
    float out[2] = {};
    m.processAudio(out);
    assert(m.analogIO.led);

    m.analogIO.selector = PgmSequencer;
    m.process();
}

TEST(WiModularTest, test_sequencer_default_sequence)
{
    WiModular m;
    m.init(1000, 44100);
    m.analogIO.selector = PgmSequencer;
    m.analogIO.button = false;
    m.process();

    m.analogIO.pot0 = 0.3f; // Speed
    m.analogIO.pot1 = 1.0f; // Include all parts of default sequence
    m.analogIO.pot2 = 0.8f; //randomness

    std::vector<float> interleaved;
    for (int i = 0; i < 10000; ++i) { // 10s
        m.process();
        interleaved.push_back(m.analogIO.cvOut0);
        interleaved.push_back(m.analogIO.cvOut2);
    }

    writeWavFile("cvOut0DefSeq.wav", interleaved, 1000);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
