//
// Created by Orion Letizi on 11/25/23.
//
#include "gtest/gtest.h"
#include "fxlib/Reverb.h"

TEST(Reverb, ReverbSc) {
    ol::fx::reverb::ReverbFx fx;
    daisysp::ReverbSc reverbsc;
    t_sample sample_rate = 1;

    EXPECT_FALSE(fx.reverb);
    EXPECT_FALSE(fx.Init);
    EXPECT_FALSE(fx.Process);
    EXPECT_FALSE(fx.Update);
    ol::fx::reverb::ReverbSc_Config(&fx, &reverbsc);
    EXPECT_TRUE(fx.reverb);
    EXPECT_TRUE(fx.Process);
    EXPECT_TRUE(fx.Update);
}

TEST(Reverb, Dottorro) {
    ol::fx::reverb::ReverbFx fx;
    sDattorroVerb *verb = DattorroVerb_create();
    t_sample sample_rate = 1;

    EXPECT_FALSE(fx.reverb);
    EXPECT_FALSE(fx.Init);
    EXPECT_FALSE(fx.Process);
    EXPECT_FALSE(fx.Update);
    ol::fx::reverb::Dattorro_Config(&fx, verb);
    EXPECT_TRUE(fx.reverb);
    EXPECT_TRUE(fx.Init);
    EXPECT_TRUE(fx.Process);
    EXPECT_TRUE(fx.Update);

}

