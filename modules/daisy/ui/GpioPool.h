//
// Created by Orion Letizi on 12/25/23.
//

#ifndef OL_DSP_GPIOPOOL_H
#define OL_DSP_GPIOPOOL_H

#include <vector>
#include "daisy.h"
#include "daisy_seed.h"

namespace ol_daisy::ui {
    struct InputHandle {
    public:
        daisy::AdcChannelConfig channel_config{};
        int channel_index = 0;
    };

    template<int MAX_SIZE>
    class GpioPool {
    private:

        daisy::DaisySeed &hw_;
        int pin_number_ = 15;
        int channel_cursor_ = 0;
        int switch_cursor_ = 0;
        InputHandle input_pool_[MAX_SIZE]{};
        daisy::Switch switch_pool_[MAX_SIZE]{};
        std::vector <InputHandle> inputs_;
        std::vector <daisy::Switch> switches_;

    public:
        explicit GpioPool(daisy::DaisySeed &hardware) : hw_(hardware) {
            for (int i = 0; i < MAX_SIZE; i++) {
                input_pool_[i].channel_index = i;
            }
        }

        float GetFloat(const InputHandle &input) {
            return hw_.adc.GetFloat(input.channel_index);
        }

        daisy::Switch &AddSwitch() {

            auto &sw = switch_pool_[switch_cursor_];
            sw.Init(daisy::DaisySeed::GetPin(pin_number_), 1000);
            switches_.push_back(sw);

            switch_cursor_++;
            pin_number_++;
            return sw;
        }

        InputHandle &AddInput() {
            InputHandle &input_handle = input_pool_[channel_cursor_];
            input_handle.channel_config.InitSingle(daisy::DaisySeed::GetPin(pin_number_));
            inputs_.push_back(input_handle);

            channel_cursor_++;
            pin_number_++;
            return input_handle;
        }

        void Start() {
            daisy::AdcChannelConfig configs[MAX_SIZE]{};
            for (int i = 0; i < inputs_.size(); i++) {
                auto h = inputs_.at(i);
                configs[i] = h.channel_config;
            }

            hw_.adc.Init(configs, inputs_.size());
            hw_.adc.Start();
        }
    };
}

#endif //OL_DSP_GPIOPOOL_H
