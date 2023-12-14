#ifdef DAISY_NATIVE

#include "daisy/daisy_dummy.h"
#include "hid/logger.h"

#else

#include "daisy.h"
#include "daisy_seed.h"

#endif
#include <cstdio>
#include "dev/lcd_hd44780.h"

#define AUDIO_BLOCK_SIZE 4

#define PIN_LCD_RS 15 // LCD: pin 8
#define PIN_LCD_EN 16 // LCD: pin 9
#define PIN_LCD_D4 20 // LCD: D4
#define PIN_LCD_D5 19 // LCD: D5
#define PIN_LCD_D6 18 // LCD: D6
#define PIN_LCD_D7 17 // LCD: D7

using namespace daisy;
using Log = Logger<LOGGER_SEMIHOST>;
static DaisySeed hardware;


//static void Println(const char* format) {
//    Logger<LOGGER_SEMIHOST>::PrintLine(format);
//}
//
//static void Println(const char* format, __builtin_va_list v) {
//    Logger<LOGGER_SEMIHOST>::PrintLine(format, v);
//}

int main() {

    hardware.Configure();
    hardware.Init();

    // LCD
    LcdHD44780 lcd;
    LcdHD44780::Config lcd_config;
    lcd_config.cursor_on = true;
    lcd_config.cursor_blink = false;
    lcd_config.rs = hardware.GetPin(PIN_LCD_RS);
    lcd_config.en = hardware.GetPin(PIN_LCD_EN);
    lcd_config.d4 = hardware.GetPin(PIN_LCD_D4);
    lcd_config.d5 = hardware.GetPin(PIN_LCD_D5);
    lcd_config.d6 = hardware.GetPin(PIN_LCD_D6);
    lcd_config.d7 = hardware.GetPin(PIN_LCD_D7);
    lcd.Init(lcd_config);

    // printf("showtime\n");
    Log::StartLog(true);
    Log::PrintLine("I should be printing to LOGGER_SEMIHOST");
    DaisySeed::StartLog(false);
    bool led_state = false;
    int counter = 0;
    while (true) {
        // Set the onboard LED
        hardware.SetLed(led_state);
        // Toggle the LED state for the next time around.
        led_state = !led_state;

        lcd.Clear();
        lcd.SetCursor(0, 0);
        lcd.Print("Counter: ");
        lcd.PrintInt(counter++);

        System::Delay(250);
    }
}