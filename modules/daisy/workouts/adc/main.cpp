#include <vector>
#include "daisy.h"
#include "daisy_seed.h"
#include "daisy/ui/ui.h"

#define VOICE_COUNT 4
#define CHANNEL_COUNT 2
using namespace daisy;
DaisySeed hw;


class InputListener : public ol_daisy::ui::VoiceInputListener {
    void GateOn(float pitch) override {
        hw.SetLed(true);
    }

    void GateOff(float pitch) override {
        hw.SetLed(false);
    };
};

InputListener input_listener;

int main() {
    hw.Configure();
    hw.Init();

    int counter = 0;
    int direction = 1;

    ol_daisy::ui::GpioPool<VOICE_COUNT> gpio(hw);
    ol_daisy::ui::PolyvoiceInputs<VOICE_COUNT> polyvoice_inputs(gpio, input_listener);
    gpio.Start();

    bool led_state = false;
    while (true) {
//        led_state = (counter % 500 == 0);
//        hw.SetLed(led_state);

        polyvoice_inputs.Process();

        counter += direction;
        if (counter % 1000 == 0) {
            direction *= -1;
        }
        System::Delay(1);

    }

}