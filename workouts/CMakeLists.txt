
## Workout library
#add_library(workout workout_buddy.cpp)
#target_link_libraries(workout ol_corelib)
#target_include_directories(workout PUBLIC . ../libs/miniaudio ../libs/rtmidi)

add_library(workout-buddy workout_buddy.cpp)
target_link_libraries(workout-buddy ol_corelib ol_fxlib ol_synthlib rtmidi)
target_include_directories(workout-buddy PUBLIC ../libs/rtmidi ../libs/miniaudio)

add_executable(drum-workout drum_main.cpp
        PatchLoader.cpp
        PatchLoader.h
        SamplePool.h
)
target_link_libraries(drum-workout workout-buddy ryml ol_synthlib)
target_include_directories(drum-workout PUBLIC ../libs/miniaudio ../libs/sfizz/src/sfizz)

add_executable(sample-workout sample_main.cpp)
target_link_libraries(sample-workout workout-buddy ol_synthlib)
target_include_directories(sample-workout PUBLIC ../libs/miniaudio)

## Saturator Workout
add_executable(saturator-workout saturator_main.cpp)
target_link_libraries(saturator-workout workout-buddy ol_fxlib)

## Multi-effects Workout
add_executable(fx-workout fx_main.cpp)
target_link_libraries(fx-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP)
target_include_directories(fx-workout PUBLIC . ../libs/miniaudio)

## Filter Workout
add_executable(filter-workout filter_main.cpp)
target_link_libraries(filter-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP)
target_include_directories(filter-workout PUBLIC . ../libs/miniaudio)

## Delay Workout
# This one uses miniaudio (ant rtmidi) which has much less goop.
add_executable(delay-workout delay_main.cpp)
target_link_libraries(delay-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP)
target_include_directories(delay-workout PUBLIC . ../libs/miniaudio)

## Reverb Workout
juce_add_console_app(reverb-workout
        PRODUCT_NAME "Reverb Workout"
)

target_sources(reverb-workout
        PRIVATE
        reverb_main.cpp
        SynthAudioCallback.h
)
target_include_directories(reverb-workout PUBLIC .)
target_compile_definitions(reverb-workout
        PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)
target_link_libraries(reverb-workout
        PRIVATE
        juce::juce_core
        juce::juce_audio_devices
        juce::juce_dsp
        ol_corelib
        ol_fxlib
        ol_synthlib
        DaisySP
        juce::juce_recommended_config_flags
        juce::juce_recommended_config_flags
)

juce_add_console_app(convolution-workout
        PRODUCT_NAME "Convolution Workout"
)

target_sources(convolution-workout
        PRIVATE
        convolution_main.cpp
        SynthAudioCallback.h
)
target_include_directories(convolution-workout PUBLIC .)
target_compile_definitions(convolution-workout
        PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)
target_link_libraries(convolution-workout
        PRIVATE
        juce::juce_core
        juce::juce_audio_devices
        juce::juce_dsp
        ol_corelib
        ol_fxlib
        ol_synthlib
        DaisySP
        juce::juce_recommended_config_flags
        juce::juce_recommended_config_flags
)


juce_add_console_app(synth-workout
        PRODUCT_NAME "Synth Workout")
target_sources(synth-workout
        PRIVATE
        synth_main.cpp
        SynthAudioCallback.h)
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
        ol_corelib
        ol_ctllib
        ol_synthlib
        DaisySP
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

#juce_add_console_app(player-workout
#        PRODUCT_NAME "Player Workout")
#target_sources(player-workout
#        PRIVATE
#        player_main.cpp
#        SpyAudioSource.cpp
#        FxAudioCallback.cpp
#        FxMidiCallback.cpp)
#target_include_directories(player-workout PUBLIC .)
#target_compile_definitions(player-workout
#        PRIVATE
#        JUCE_WEB_BROWSER=0
#        JUCE_USE_CURL=0)
#target_link_libraries(player-workout
#        PRIVATE
#        juce::juce_core
#        juce::juce_audio_devices
#        juce::juce_audio_formats
#        PUBLIC
#        ol_corelib
#        ol_fxlib
#        DaisySP
#        juce::juce_recommended_config_flags
#        juce::juce_recommended_config_flags)