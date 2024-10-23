add_compile_definitions()
set(TEENSY_ROOT "$ENV{HOME}/Library/Arduino15/packages/teensy/hardware/avr/1.58.1")
set(TEENSY_LIBS "${TEENSY_ROOT}/libraries")
set(ARDUINO_LIBS "$ENV{HOME}/Documents/Arduino/libraries")
set(ARDUINO_INCLUDES ${TEENSY_ROOT}/cores/teensy4
        ${TEENSY_LIBS}/Audio
        ${TEENSY_LIBS}/MIDI/src
        ${TEENSY_LIBS}/SD/src
        ${TEENSY_LIBS}/SerialFlash
        ${TEENSY_LIBS}/SPI
        ${TEENSY_LIBS}/Wire
        ${TEENSY_LIBS}/ILI9341_t3
        ${ARDUINO_LIBS}/ILI9341_T4/src/
        ${ARDUINO_LIBS}/Adafruit_GFX_Library
        ${ARDUINO_LIBS}/Adafruit_SSD1306)

add_library(ol_teensy STATIC ol_teensy.h io/TeensySerial.h)
set_target_properties(ol_teensy PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(ol_teensy ol_corelib)
target_compile_definitions(ol_teensy PUBLIC TEENSY_LOCAL)
target_include_directories(ol_teensy PUBLIC
        . ..
        ${TEENSY_ROOT}/cores/teensy4
        ${TEENSY_LIBS}/Audio
        ${TEENSY_LIBS}/MIDI/src
        ${TEENSY_LIBS}/SD/src
        ${TEENSY_LIBS}/SerialFlash
        ${TEENSY_LIBS}/SPI
        ${TEENSY_LIBS}/Wire
        ${TEENSY_LIBS}/ILI9341_t3
        ${ARDUINO_LIBS}/Adafruit_GFX_Library
        ${ARDUINO_LIBS}/Adafruit_SSD1306
)

add_executable(ol_teensy_synth app/synth/loop.h ol_teensy.h)
target_link_libraries(ol_teensy_synth PUBLIC ol_teensy ol_guilib)
target_include_directories(ol_teensy_synth PUBLIC . .. ${ARDUINO_INCLUDES})
set_target_properties(ol_teensy_synth PROPERTIES LINKER_LANGUAGE CXX)

add_executable(ol_teensy_workouts_uart workouts/uart/loop.h workouts/dummy.cpp)
target_link_libraries(ol_teensy_workouts_uart PUBLIC ol_teensy ol_guilib)
target_include_directories(ol_teensy_workouts_uart PUBLIC . .. ${ARDUINO_INCLUDES})
set_target_properties(ol_teensy_workouts_uart PROPERTIES LINKER_LANGUAGE CXX)

add_executable(ol_teensy_workouts_tft workouts/tft/loop.h workouts/tft_dummy.cpp)
target_link_libraries(ol_teensy_workouts_tft PUBLIC tgx ol_teensy ol_guilib ol_lvgl)
target_include_directories(ol_teensy_workouts_tft PUBLIC . .. ${ARDUINO_INCLUDES})
set_target_properties(ol_teensy_workouts_tft PROPERTIES LINKER_LANGUAGE CXX)

add_executable(ol_teensy_workouts_midi workouts/midi/loop.h workouts/midi_dummy.cpp)
target_link_libraries(ol_teensy_workouts_midi PUBLIC tgx ol_teensy)
target_include_directories(ol_teensy_workouts_midi PUBLIC . .. ${ARDUINO_INCLUDES})
set_target_properties(ol_teensy_workouts_midi PROPERTIES LINKER_LANGUAGE CXX)