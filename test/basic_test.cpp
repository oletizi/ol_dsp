#include "gtest/gtest.h"
#include <ol_synthlib.h>
#include <ol_fxlib.h>

using namespace ol::synthlib;
TEST(BasicTestSuite, BasicTest) {
    EXPECT_EQ(1, 1);
}

TEST(Voice, TestBasics) {
    ControlPanel cp;
    t_sample sample_rate = 441000;
    Voice voice = Voice(&cp);
    voice.Init(sample_rate);
    t_sample value = voice.Process();
    EXPECT_EQ(value, 0);


}
