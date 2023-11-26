//
// Created by Orion Letizi on 11/25/23.
//
#include "gtest/gtest.h"
#include "fxlib/Reverb.h"

TEST(Reverb, ReverbSc) {
    ol::fx::reverb::ReverbFx reverbfx;
    daisysp::ReverbSc reverbsc;
    t_sample sample_rate = 1;

    ol::fx::reverb::ReverbSc_Init(&reverbfx, &reverbsc, sample_rate);
    EXPECT_TRUE(reverbfx.reverb);
    EXPECT_TRUE(reverbfx.Process);
    EXPECT_TRUE(reverbfx.Update);
}

TEST(Reverb, Dottorro) {
    ol::fx::reverb::ReverbFx fx;
    sDattorroVerb *verb = DattorroVerb_create();
    t_sample sample_rate = 1;

    EXPECT_FALSE(fx.reverb);
    EXPECT_FALSE(fx.Process);
    EXPECT_FALSE(fx.Update);
    ol::fx::reverb::Dattorro_Init(&fx, verb, sample_rate);
    EXPECT_TRUE(fx.reverb);
    EXPECT_TRUE(fx.Process);
    EXPECT_TRUE(fx.Update);

}

