#include "../Sampler.hpp"
#ifdef USE_NO_MIDI
#include <gtest/gtest.h>

TEST(SamplerTest, DefaultConstructor) {
    Sampler s;
    int fs = 44100;
    s.init(fs);

    s.playNewVoice(1, 1);

    float out[2];
    s.process(out);
    s.releaseVoice(1);

    EXPECT_EQ(out[0], 0);

    s.setAnalogIns({
        .input_0 = 0.3f, //Second program
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
        .input_4 = 0.0f,
        .input_5 = 0.0f,
        .input_6 = 0.0f,
        .input_7 = 0.0f
    });

    s.process(out);

    EXPECT_EQ(out[0], 0);

    s.playNewVoice(1, 1);
    s.process(out);
    s.releaseVoice(1);

    EXPECT_GT(fabs(out[0]), 0);

    s.setAnalogIns({
        .input_0 = 0.0f, //First program
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
        .input_4 = 0.0f,
        .input_5 = 0.0f,
        .input_6 = 0.0f,
        .input_7 = 0.0f
    });

    s.process(out);
    EXPECT_EQ(out[0], 0);

    s.playNewVoice(1, 1);
    s.process(out);
    s.releaseVoice(1);

    EXPECT_GT(fabs(out[0]), 0);

    s.setAnalogIns({
        .input_0 = 0.57f, //Third program
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
        .input_4 = 0.0f,
        .input_5 = 0.0f,
        .input_6 = 0.0f,
        .input_7 = 0.0f
    });

    s.process(out);
    EXPECT_EQ(out[0], 0);

    s.playNewVoice(2, 1);
    s.process(out);
    s.releaseVoice(2);
    EXPECT_GT(fabs(out[0]), 0);
}

TEST(SamplerTest, SetAnalogIns_can_be_called_many_times) {
    Sampler s;
    int fs = 44100;
    s.init(fs);

    s.playNewVoice(1, 1);

    float out[2];
    s.process(out);
    s.releaseVoice(1);

    EXPECT_EQ(out[0], 0);

    s.setAnalogIns({
        .input_0 = 0.3f, //Second program
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
        .input_4 = 0.0f,
        .input_5 = 0.0f,
        .input_6 = 0.0f,
        .input_7 = 0.0f
    });

    s.setAnalogIns({
        .input_0 = 0.3f, //Second program
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
        .input_4 = 0.0f,
        .input_5 = 0.0f,
        .input_6 = 0.0f,
        .input_7 = 0.0f
    });

    s.process(out);
    EXPECT_EQ(out[0], 0);

    s.playNewVoice(1, 1);
    s.process(out);
    s.releaseVoice(1);
    EXPECT_GT(fabs(out[0]), 0);
}

TEST(SamplerTest, kjipe_samples_start_at_02_because_of_capo) {
    Sampler s;
    int fs = 44100;
    s.init(fs);
    float out[2];
    s.setAnalogIns({
        .input_0 = 0.57f, //Third program
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
        .input_4 = 0.0f,
        .input_5 = 0.0f,
        .input_6 = 0.0f,
        .input_7 = 0.0f
    });

    s.process(out);
    EXPECT_EQ(out[0], 0);

    s.playNewVoice(1, 1);
    s.process(out);
    s.releaseVoice(1);
    EXPECT_EQ(fabs(out[0]), 0);

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
        .input_0 = 0.84f, //Third program
        .input_1 = 1.0f,
        .input_2 = 0.0f,
        .input_3 = 0.0f,
        .input_4 = 0.0f,
        .input_5 = 0.0f,
        .input_6 = 0.0f,
        .input_7 = 0.0f
    });

    s.process(out);
    EXPECT_EQ(out[0], 0);

    s.playNewVoice(1, 1);
    s.process(out);
    s.releaseVoice(1);
    EXPECT_EQ(fabs(out[0]), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
