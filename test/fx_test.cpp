//
// Created by Orion Letizi on 11/13/23.
//
#include "gtest/gtest.h"
#include "daisysp.h"
#include "fxlib/Fx.h"
using namespace ol::fx;
TEST(FX, Saturator) {
    SaturatorFx saturator;
    saturator.Init( 48000);

    t_sample in = 0;
    t_sample out = 1;
    saturator.Process( &in, &out);
    EXPECT_EQ(in, out);

    in = 1;
    out = 1;
    saturator.Process( &in, &out);
    EXPECT_NE(in, out);
}

TEST(FX, Delay) {
    daisysp::Svf svf;
    auto filter = ol::fx::FilterFx(svf);
    daisysp::DelayLine<t_sample, MAX_DELAY> delay_line;
    auto delay = DelayFx(delay_line, filter);

    delay.Init(128);
    t_sample in = 1;
    t_sample out = 0;
    delay.Process( &in, &out);
    t_sample sample_rate = 48000;
    t_sample freq = 20000;

    //delay.time = 0.5;
    delay.UpdateHardwareControl(CC_DELAY_TIME, 0.5);
    delay.Update();
    daisysp::Oscillator osc;
    delay.Init(sample_rate);
    osc.Init(sample_rate);
    osc.SetFreq(freq);
    for (int i = 0; i < sample_rate; i++) {
        in = osc.Process();
        delay.Process(&in, &out);
        if (isnan(out)) {
            std::cout << "NaN! sample: " << i + 1 << "sample rate: " << sample_rate << "; freq: " << freq << "; in: "
                      << in << "; out: " << std::endl;
        }
        EXPECT_FALSE(isnan(out));
        //std::cout << "in: " << in << "; out: " << out << std::endl;
    }
}

//TEST(FX, FxRack) {
//
//    daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> delay_line1;
//    daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> delay_line2;
//
//    ol::fx::DelayFx delay1;
//    ol::fx::DelayFx delay2;
//
//    ol::fx::Delay_Config(&delay1, &delay_line1);
//    ol::fx::Delay_Config(&delay2, &delay_line2);
//
//    daisysp::ReverbSc reverbSc;
//    ol::fx::ReverbFx reverb;
//    ol::fx::ReverbSc_Config(&reverb, &reverbSc);
//}