cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_COMPILER "/usr/bin/clang")
project(juce_test VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)
add_subdirectory(DaisySP)
juce_add_console_app(juce-test
        PRODUCT_NAME "Console App Example")
target_sources(juce-test
        PRIVATE
        main.cpp
        MyCallback.h
    )
target_compile_definitions(juce-test
        PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)    # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_console_app` call
target_link_libraries(juce-test
        PRIVATE
        # ConsoleAppData            # If you'd created a binary data target, you'd link to it here
        juce::juce_core
        juce::juce_audio_devices
        PUBLIC
        DaisySP
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
include_directories(juce-test DaisySP/Source)
#add_executable(juce_test main.cpp)
