#include "../Sampler.hpp"
#ifdef USE_NO_MIDI
#include <gtest/gtest.h>


TEST(SamplerTest, kjipe_samples_start_at_02_because_of_capo)
{
    Sampler s;
    int fs = 44100;
    s.init(fs);
    float out[2];
    s.setAnalogIns({
        .input_0 = s.convertFromProgram(2), //Third program
        .input_1 = 1.0f,
        .input_3 = 1.0f
    });

    for (int i = 0; i < 10; ++i)
        s.process(out);
    EXPECT_GT(fabs(out[0]), 0); // Because of drone

    s.playNewVoice(1, 1);
    s.process(out);
    s.releaseVoice(1);
    EXPECT_GT(fabs(out[0]), 0);

    s.playNewVoice(3, 1);
    s.process(out);
    s.releaseVoice(3);
    EXPECT_GT(fabs(out[0]), 0);

    s.playNewVoice(2, 1);
    s.process(out);
    s.releaseVoice(2);
    EXPECT_GT(fabs(out[0]), 0);
}

TEST(SamplerTest, melotron) {
    Sampler s;
    int fs = 44100;
    s.init(fs);
    float out[2];
    s.setAnalogIns({
        .input_0 = s.convertFromProgram(3), //Forth program
        .input_1 = 1.0f,
    });

    s.process(out);
    EXPECT_EQ(out[0], 0);

    s.playNewVoice(1, 1);
    s.process(out);
    s.releaseVoice(1);
    EXPECT_GT(fabs(out[0]), 0);
}

TEST(SamplerTest, test_that_drone_sample_loops)
{
    Sampler s;
    int fs = 44100;
    s.init(fs);
    float out[2];
    s.setAnalogIns({
        .input_0 = s.convertFromProgram(2), //Third program
        .input_1 = 1.0f,
        .input_3 = 1.0f,
    });

    for (int i = 0; i < 1217427/*num frames in drone*/ +8732/*release time*/; i++)
        s.process(out);

    EXPECT_GT(fabs(out[0]), 0);
}

TEST(SamplerTest, input_1_controls_the_global_amplitude)
{
    Sampler s;
    int fs = 44100;
    s.init(fs);
    float out[2];
    s.setAnalogIns({
        .input_0 = s.convertFromProgram(2), //Third program
        .input_1 = 1.0f,
        .input_3 = 1.0f,
    });

    for (int i = 0; i < 10; ++i)
        s.process(out);
    EXPECT_GT(fabs(out[0]), 0);

    s.setAnalogIns({
        .input_0 = s.convertFromProgram(2), //Third program
        .input_1 = 0.0f,
        .input_3 = 1.0f,
    });

    s.process(out);
    EXPECT_EQ(out[0], 0);
}

TEST(SamplerTest, input_3_controls_the_volume_of_drone)
{
    Sampler s;
    int fs = 44100;
    s.init(fs);
    float out[2];
    s.setAnalogIns({
        .input_0 = s.convertFromProgram(2),
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
    });

    s.process(out);
    EXPECT_EQ(out[0], 0);

    s.setAnalogIns({
        .input_0 = s.convertFromProgram(2),
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 1.0f,
    });

    s.process(out);
    EXPECT_GT(fabs(out[0]), 0);
}

TEST(SamplerTest, program_0_is_a_drum_drone)
{
    Sampler s;
    int fs = 44100;
    s.init(fs);
    float out[2];
    s.setAnalogIns({
        .input_0 = s.convertFromProgram(0),
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
    });

    for (int i = 0; i < 100; ++i) {
        s.process(out);
    }

    EXPECT_EQ(out[0], 0);

    s.setAnalogIns({
        .input_0 = s.convertFromProgram(0),
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 1.0f,
    });

    s.process(out);
    EXPECT_GT(fabs(out[0]), 0);
}

TEST(SamplerTest, program_1_is_a_drum_drone)
{
    Sampler s;
    int fs = 44100;
    s.init(fs);
    float out[2];
    s.setAnalogIns({
        .input_0 = s.convertFromProgram(1),
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
    });

    for (int i = 0; i < 100; ++i) {
        s.process(out);
    }
    EXPECT_EQ(out[0], 0);

    s.setAnalogIns({
        .input_0 = s.convertFromProgram(1),
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 1.0f,
    });

    s.process(out);
    EXPECT_GT(fabs(out[0]), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
