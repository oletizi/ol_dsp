//
// Created by Orion Letizi on 11/13/23.
//
#include "gtest/gtest.h"
#include "daisysp.h"
#include "fxlib/Fx.h"
using namespace ol::fx;
TEST(FX, Saturator) {
    SaturatorFx saturator;
    sp_saturator spsaturator;
    sp_data spdata;
    Saturator_Config(&saturator, &spsaturator, &spdata);
    saturator.Init(&saturator, 48000);

    t_sample in = 0;
    t_sample out = 1;
    saturator.Process(&saturator, in, &out);
    EXPECT_EQ(in, out);

    in = 1;
    out = 1;
    saturator.Process(&saturator, in, &out);
    EXPECT_NE(in, out);
}

TEST(FX, Delay) {
    daisysp::Svf svf;
    ol::fx::FilterFx filter;
    ol::fx::Filter_Svf_Config(&filter, &svf);

    daisysp::DelayLine<t_sample, MAX_DELAY> delay_line;
    ol::fx::DelayFx delay;
    ol::fx::Delay_Config(&delay, &delay_line, &filter);

    delay.Init(&delay, 128);
    t_sample in = 1;
    t_sample out = 0;
    delay.Process(&delay, in, &out);
    t_sample sample_rate = 48000;
    t_sample freq = 20000;
    delay.time = 0.5;
    delay.Update(&delay);
    daisysp::Oscillator osc;
    delay.Init(&delay, sample_rate);
    osc.Init(sample_rate);
    osc.SetFreq(freq);
    for (int i = 0; i < sample_rate; i++) {
        in = osc.Process();
        delay.Process(&delay, in, &out);
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