#include "gtest/gtest.h"
#include "ol_synthlib.h"

using namespace ol::synthlib;
TEST(BasicTestSuite, BasicTest) {
    EXPECT_EQ(1, 1);
}

TEST(Voice, TestBasics) {
    t_sample sample_rate = 441000;
    Voice voice = Voice();
    voice.Init(sample_rate);
    t_sample value = voice.Process();
    EXPECT_EQ(value, 0);

    value = voice.Process();
    EXPECT_NE(value, 0);
}
