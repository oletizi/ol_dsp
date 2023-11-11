cmake_minimum_required(VERSION 3.26)
project(ol_dsp VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)
add_subdirectory(DaisySP)
add_library(fxlib ./fxlib/ol_fxlib.h
        fxlib/FxControlPanel.h
        fxlib/FxControlPanel.cpp
        fxlib/ol_fxlib_core.h)
add_library(synthlib ./synthlib/Voice.cpp
        synthlib/Control.cpp
        synthlib/ControlPanel.h)
target_include_directories(synthlib PUBLIC ./synthlib)

add_subdirectory(test/googletest)


# Workout executables
juce_add_console_app(fx-workout
        PRODUCT_NAME "FX Workout")
target_sources(fx-workout PRIVATE fx_main.cpp)
target_compile_definitions(fx-workout PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(fx-workout
        PRIVATE
        juce::juce_core
        juce::juce_audio_devices
        PUBLIC
        fxlib
        DaisySP
        juce::juce_recommended_config_flags
        juce::juce_recommended_config_flags)
target_include_directories(fxlib PUBLIC fxlib DaisySP/Source)

juce_add_console_app(synth-workout
        PRODUCT_NAME "Synth Workout")
target_sources(synth-workout
        PRIVATE
        synth_main.cpp
        MyCallback.h)
target_compile_definitions(synth-workout
        PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)    # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_console_app` call
target_link_libraries(synth-workout
        PRIVATE
        # ConsoleAppData            # If you'd created a binary data target, you'd link to it here
        juce::juce_core
        juce::juce_audio_devices
        PUBLIC
        synthlib
        DaisySP
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
include_directories(synth-workout DaisySP/Source)
#add_executable(juce_test main.cpp)
