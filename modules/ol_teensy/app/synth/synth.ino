/*
    Arduino .ino stub for arduino-cli. This moves all the logic out to cpp files where they can be part of
    a CMake project for CLion.
 */
// Defining NO_HACKS turns off the hacks to make CLion happy.
#define NO_HACKS
#include "loop.h"

void setup() {
    doSetup();
}
void loop() {
    doLoop();
}
