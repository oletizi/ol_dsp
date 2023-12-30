#ifdef DAISY_NATIVE
//#include "daisy/daisy_dummy.h"
//#include "hid/logger.h"
#else


#endif

#include <cstdio>
#include "daisy.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

#include "corelib/ol_corelib.h"

#define AUDIO_BLOCK_SIZE 4
#define DISPLAY_ON true
#define DISPLAY_UPDATE_FREQUENCY 100
#define CHANNEL_COUNT 2
#define VOICE_COUNT 1
#define MAX_CONTROLS 5
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

    int counter = 0;
    char strbuff[128];
    int direction = 1;


    int line_number;
    auto font = Font_11x18;//Font_7x10;
    while (true) {
        hw.SetLed(counter < 500);

        if (DISPLAY_ON && (counter % DISPLAY_UPDATE_FREQUENCY == 0)) {
            line_number = 0;
            display.Fill(false);
            display.SetCursor(0, 24);
            sprintf(strbuff, "counter: %d", counter);
            display.WriteString(strbuff, font, true);

            display.Update();
        }
        counter += direction;
        if (counter % 1000 == 0) {
            direction *= -1;
        }
        System::Delay(1);

    }

}