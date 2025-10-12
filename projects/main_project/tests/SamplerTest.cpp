#include "../Sampler.hpp"
#ifdef USE_NO_MIDI
#include <gtest/gtest.h>

TEST(SamplerTest, DefaultConstructor) {
    Sampler s;
    int fs = 44100;
    s.init(fs);
    s.playNewVoice(1);

    float out[2];
    s.process(out);
    s.releaseVoice(1);

    EXPECT_GT(fabs(out[0]), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
