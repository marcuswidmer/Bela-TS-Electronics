#include <gtest/gtest.h>
#include "../Sampler.hpp"

TEST(SamplerTest, DefaultConstructor) {
    Sampler obj;
    EXPECT_EQ(obj.getValue(), 0); // Example assertion
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
