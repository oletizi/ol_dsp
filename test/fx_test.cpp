//
// Created by Orion Letizi on 11/13/23.
//
#include "gtest/gtest.h"
#include "daisysp.h"
#include "ol_fxlib.h"

TEST(FX, Delay) {
    ol::fx::Delay delay;
    t_sample in = 1;
    t_sample out = delay.Process(in);
    t_sample sample_rate = 48000;
    t_sample freq = 20000;
    delay.UpdateDelayTime(sample_rate / 4);
    daisysp::Oscillator osc;
    osc.Init(sample_rate);
    osc.SetFreq(freq);
    for (int i = 0; i < sample_rate; i++) {
        in = osc.Process();
        out = delay.Process(in);
        if (isnan(out)) {
            std::cout << "NaN! sample: " << i + 1 << "sample rate: " << sample_rate << "; freq: " << freq << "; in: "
                      << in << "; out: " << std::endl;
        }
        EXPECT_FALSE(isnan(out));
        //std::cout << "in: " << in << "; out: " << out << std::endl;
    }
}