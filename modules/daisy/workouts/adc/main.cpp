#include <vector>
#include "daisy.h"
#include "daisy_seed.h"
#include "daisy/ui/ui.h"

#define VOICE_COUNT 4
#define CHANNEL_COUNT 2
using namespace daisy;
DaisySeed hw;


int main() {

    hw.Configure();
    hw.Init();

//    AnalogControl cv_1;
//    AdcChannelConfig cv_1_config{};;
//    cv_1_config.InitSingle(hw.GetPin(15));
//
//    hw.adc.Init(&cv_1_config, 1);
//    cv_1.Init(hw.adc.GetPtr(0), hw.AudioCallbackRate());
//    hw.adc.Start();
    ol_daisy::io::GpioPool<12> gpio(hw);
    daisy::AnalogControl cv_1;
    gpio.AddInput(&cv_1);
    gpio.Start();

    int counter = 0;
    int direction = 1;
    float cv_1_value = 0;

    bool led_state = false;
    while (true) {
        cv_1_value = cv_1.Process();
        led_state = (counter < 250 || counter > 750);
        hw.SetLed(led_state);

        counter += direction;
        if (counter % 1000 == 0) {
            direction *= -1;
        }
        System::Delay(1);

    }

}