#include "../WiSustainer.hpp"
#ifdef USE_NO_MIDI
#include <gtest/gtest.h>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdint>

static void writeWavFile(const std::string& filename,
                         const std::vector<float>& interleaved,
                         int sampleRate,
                         int numChannels = 2)
{
    // Simple 16-bit PCM WAV writer
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

struct WavData {
    int sampleRate = 0;
    int numChannels = 0;
    std::vector<float> interleaved; // Stereo interleaved if mono input (duplicated).
};

static WavData readWavFile(const std::string& filename)
{
    std::ifstream f(filename, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Failed to open wav file for reading: " + filename);
    }

    auto read_u32 = [&](uint32_t& v) {
        f.read(reinterpret_cast<char*>(&v), 4);
        if (!f) throw std::runtime_error("Unexpected EOF while reading uint32.");
    };
    auto read_u16 = [&](uint16_t& v) {
        f.read(reinterpret_cast<char*>(&v), 2);
        if (!f) throw std::runtime_error("Unexpected EOF while reading uint16.");
    };

    char riff[4];
    f.read(riff, 4);
    if (std::string(riff, 4) != "RIFF") {
        throw std::runtime_error("Not a RIFF file.");
    }

    uint32_t riffSize = 0;
    read_u32(riffSize);

    char wave[4];
    f.read(wave, 4);
    if (std::string(wave, 4) != "WAVE") {
        throw std::runtime_error("Not a WAVE file.");
    }

    // Parse chunks until we find fmt and data.
    bool haveFmt = false;
    bool haveData = false;

    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;

    std::streampos dataPos{};
    uint32_t dataSize = 0;

    while (f && !(haveFmt && haveData)) {
        char cid[4];
        f.read(cid, 4);
        if (!f) break;
        uint32_t csize = 0;
        read_u32(csize);

        std::string id(cid, 4);
        if (id == "fmt ") {
            haveFmt = true;
            read_u16(audioFormat);
            read_u16(numChannels);
            read_u32(sampleRate);
            uint32_t byteRate; read_u32(byteRate);
            uint16_t blockAlign; read_u16(blockAlign);
            read_u16(bitsPerSample);

            // Skip any remaining bytes in fmt chunk beyond the 16 we read.
            if (csize > 16) {
                f.seekg(static_cast<std::streamoff>(csize - 16), std::ios::cur);
            }
        } else if (id == "data") {
            haveData = true;
            dataPos = f.tellg();
            dataSize = csize;
            // Skip data for now; we'll seek back to read it later.
            f.seekg(static_cast<std::streamoff>(csize), std::ios::cur);
        } else {
            // Skip unknown chunk (+ pad byte if size is odd).
            std::streamoff skip = static_cast<std::streamoff>(csize + (csize & 1));
            f.seekg(skip, std::ios::cur);
        }

        // Align to even byte boundary if needed.
        if (csize & 1) {
            f.seekg(1, std::ios::cur);
        }
    }

    if (!haveFmt) throw std::runtime_error("WAV fmt chunk not found.");
    if (!haveData) throw std::runtime_error("WAV data chunk not found.");
    if (audioFormat != 1) throw std::runtime_error("Only PCM WAV (format 1) is supported.");
    if (!(bitsPerSample == 16)) throw std::runtime_error("Only 16-bit PCM WAV is supported.");
    if (!(numChannels == 1 || numChannels == 2)) throw std::runtime_error("Only mono or stereo WAV is supported.");

    // Read audio data.
    f.clear();
    f.seekg(dataPos);
    const uint32_t bytesPerSample = bitsPerSample / 8;
    const uint32_t frameSize = bytesPerSample * numChannels;
    if (frameSize == 0) throw std::runtime_error("Invalid WAV frame size.");
    const uint32_t totalFrames = dataSize / frameSize;

    std::vector<int16_t> raw(totalFrames * numChannels);
    f.read(reinterpret_cast<char*>(raw.data()), dataSize);
    if (!f) throw std::runtime_error("Failed to read WAV sample data.");

    WavData out;
    out.sampleRate = static_cast<int>(sampleRate);

    if (numChannels == 2) {
        out.numChannels = 2;
        out.interleaved.reserve(raw.size());
        for (size_t i = 0; i < raw.size(); i += 2) {
            auto toFloat = [](int16_t s) {
                float v = static_cast<float>(s) / 32767.0f;
                return std::clamp(v, -1.0f, 1.0f);
            };
            out.interleaved.push_back(toFloat(raw[i + 0]));
            out.interleaved.push_back(toFloat(raw[i + 1]));
        }
    } else { // mono -> duplicate to stereo
        out.numChannels = 2;
        out.interleaved.reserve(static_cast<size_t>(totalFrames) * 2);
        for (size_t i = 0; i < raw.size(); ++i) {
            float v = std::clamp(static_cast<float>(raw[i]) / 32767.0f, -1.0f, 1.0f);
            out.interleaved.push_back(v);
            out.interleaved.push_back(v);
        }
    }

    return out;
}

TEST(WiSustainerTest, singen_the_original_440)
{
    int fs = 44100;
    WiSustainer s(fs);
    WavData data = readWavFile("/Users/marwidme/Documents/Personal/Bela/Bela-TS-Electronics-new/projects/wi_sustainer/tests/jazz_lick.wav");
    printf("Input data channels: %d\n", data.numChannels);
    constexpr float durationSec = 5.0f;
    const int totalFrames = static_cast<int>(fs * durationSec);
    std::vector<float> interleaved;
    interleaved.reserve(totalFrames * 2);

    std::vector<float> rmsVec;
    rmsVec.reserve(totalFrames);

    float in;
    float out[2];
    float rms;
    for (int n = 0; n < totalFrames; ++n)
    {
        in = data.interleaved.at(2 * n);
        s.process(in, out, &rms);
        interleaved.push_back(out[0]);
        interleaved.push_back(out[1]);
        rmsVec.push_back(rms);
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
    writeWavFile("singen_the_original_rms.wav", rmsVec, fs, 1);

    // Sanity check
    EXPECT_TRUE(std::filesystem::exists("singen_the_original.wav"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
