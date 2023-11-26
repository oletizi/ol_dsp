//
// Created by Orion Letizi on 11/13/23.
//
#include "gtest/gtest.h"
#include "daisysp.h"
#include "ol_fxlib.h"

TEST(FX, Delay) {
    ol::fx::DelayControlPanel control_panel;
    daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> delay_line;
    ol::fx::Delay delay(&control_panel, &delay_line);

    delay.Init(128);
    t_sample in = 1;
    t_sample out = delay.Process(in);
    t_sample sample_rate = 48000;
    t_sample freq = 20000;
    //delay.UpdateDelayTime(sample_rate / 4);
    control_panel.time.UpdateValueHardware(0.5);
    daisysp::Oscillator osc;
    delay.Init(sample_rate);
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

TEST(FX, FxChain) {

    ol::fx::DelayControlPanel delay_control_panel;
    ol::fx::LpfControlPanel lpf_control_panel;
    ol::fx::FxControlPanel control_panel = ol::fx::FxControlPanel(&delay_control_panel,
                                                                  &lpf_control_panel);
    daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> delay_line1;
    daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> delay_line2;

    ol::fx::Delay delay1 = ol::fx::Delay(&delay_control_panel, &delay_line1);
    ol::fx::Delay delay2 = ol::fx::Delay(&delay_control_panel, &delay_line2);
    ol::fx::LPF lpf1(&lpf_control_panel);
    ol::fx::LPF lpf2(&lpf_control_panel);
    daisysp::ReverbSc reverbSc;
    ol::fx::FxChain chain(&control_panel, &delay1, &delay2, &lpf1, &lpf2);
}