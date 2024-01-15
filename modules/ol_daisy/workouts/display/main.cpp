#ifdef DAISY_NATIVE
//#include "daisy/daisy_dummy.h"
//#include "hid/logger.h"

#else


#endif

#include <cstdio>
#include "daisy.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

#define DISPLAY_ON true

using namespace daisy;


using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;
static DaisySeed hw;
MyOledDisplay display;


int main() {

    hw.Configure();
    hw.Init();


    /** Configure the Display */
    MyOledDisplay::Config disp_cfg = {};
    disp_cfg.driver_config.transport_config.pin_config.dc = daisy::DaisySeed::GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = daisy::DaisySeed::GetPin(30);
    /** And Initialize */
    display.Init(disp_cfg);
    // printf("showtime\n");

    //DaisySeed::StartLog(false);
    int counter = 0;
    char strbuff[128];
    int direction = 1;

    //Start reading values
    hw.adc.Start();

    auto font = Font_11x18;//Font_7x10;
    bool led_state = true;
    while (true) {

        // Set the onboard LED
        hw.SetLed(led_state);
        led_state = !led_state;

        display.Fill(true);

        display.SetCursor(0, 0);
        sprintf(strbuff, "counter: %d", counter);
        display.WriteString(strbuff, font, false);

        display.Update();

        counter += direction;
        if (counter % 99 == 0) {
            direction *= -1;
        }
        System::Delay(250);

    }

}