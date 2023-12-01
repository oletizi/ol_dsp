#define NO_HACKS
#include "loop.h"

//void setup() {
//    doSetup();
//}
//void loop() {
//    doLoop();
//}

volatile uint8_t gt_state;
volatile bool gt3_retrigger, gt4_retrigger;
volatile unsigned long gt3_retrigger_time, gt4_retrigger_time;
DU_Demo_Function * function;

void setup()
{
    function = new DU_Demo_Function();

    gt_state = 0;
    gt3_retrigger = gt4_retrigger = false;
    gt3_retrigger_time = gt4_retrigger_time = 0;

    function->begin();
}

void loop()
{
    function->function_loop();
}