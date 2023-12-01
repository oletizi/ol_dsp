set(DUINO_ROOT "$ENV{HOME}/Library/Arduino15/packages/arduino/hardware/avr/1.8.6")
set(DUINO_LIB "../../../libs/du-ino/src")
set(DUINO_INSTALLED_LIBS "${DUINO_ROOT}/libraries")
set(ARDUINO_LIBS "$ENV{HOME}/Documents/Arduino/libraries")
add_executable(duino-demo loop.h loop.cpp)
target_include_directories(duino-demo PUBLIC
        ${DUINO_ROOT}/cores/arduino
        ${DUINO_LIB}
        ${ARDUINO_LIBS}/Adafruit_GFX_Library
        ${ARDUINO_LIBS}/Adafruit_SSD1306
)
#include_directories(
#        ${DUINO_ROOT}/cores/arduino
#        ${DUINO_LIB}
#        ${ARDUINO_LIBS}/Adafruit_GFX_Library
#        ${ARDUINO_LIBS}/Adafruit_SSD1306
#)