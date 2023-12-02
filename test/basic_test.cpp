#include "gtest/gtest.h"
#include <ol_synthlib.h>

using namespace ol::synthlib;
TEST(BasicTestSuite, BasicTest) {
    EXPECT_EQ(1, 1);
}

TEST(Voice, TestBasics) {
    t_sample sample_rate = 441000;
    Voice voice;
    Voice_Config(&voice);
    voice.Init(&voice, sample_rate);
    t_sample value = voice.Process(&voice);
    EXPECT_EQ(value, 0);
}
