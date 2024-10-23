
set(BUILD_WORKOUT_BUDDY OFF)

## Workout library
if (${BUILD_WORKOUT_BUDDY})
    add_library(workout-buddy attic/workout_buddy.cpp)
    target_link_libraries(workout-buddy ol_corelib ol_fxlib ol_synthlib rtmidi)
    target_include_directories(workout-buddy PUBLIC ../libs/rtmidi ../libs/miniaudio)
endif ()

add_executable(gui-workout attic/gui_main.cpp)
target_link_libraries(gui-workout sfml-graphics ol_corelib ol_guilib)
target_include_directories(gui-workout PUBLIC .)

add_executable(player-workout player_main.cpp)
#target_link_libraries(player-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP)
target_include_directories(player-workout PUBLIC . ../libs/miniaudio)



#juce_add_gui_app(gui-workout)
#target_sources(gui-workout
#        PRIVATE
#        gui_main.cpp
#)
#target_compile_definitions(gui-workout
#        PRIVATE
#        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
#        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
#        JUCE_USE_CURL=0
#        JIVE_GUI_ITEMS_HAVE_STYLE_SHEETS=1
#)
#target_link_libraries(gui-workout
#        PRIVATE
#        DaisySP
#        juce::juce_core
#        juce::juce_gui_basics
#        juce::juce_audio_devices
#        juce::juce_dsp
#        juce::juce_recommended_config_flags
#        juce::juce_recommended_config_flags
##        jive::jive_layouts
##        jive::jive_style_sheets
#        ol_corelib
#        ol_fxlib
#        ol_synthlib
#        ol_guilib
#)


if (${BUILD_WORKOUT_BUDDY})

    add_executable(stmlib-workout attic/stmlib_main.cpp)
    target_link_libraries(stmlib-workout rtmidi stmlib workout-buddy ol_corelib ol_fxlib ol_synthlib)

    add_executable(drum-workout attic/drum_main.cpp
            ../modules/iolib/PatchLoader.cpp
            ../modules/iolib/PatchLoader.h
            attic/SamplePool.h
    )
    target_link_libraries(drum-workout workout-buddy ryml ol_synthlib)
    target_include_directories(drum-workout PUBLIC ../libs/miniaudio ../libs/sfizz/src/sfizz)


    add_executable(sample-workout attic/sample_main.cpp)
    target_link_libraries(sample-workout workout-buddy ol_synthlib)
    target_include_directories(sample-workout PUBLIC ../libs/miniaudio)

    ## Saturator Workout
    add_executable(saturator-workout attic/saturator_main.cpp)
    target_link_libraries(saturator-workout workout-buddy ol_fxlib)

    ## Multi-effects Workout
    add_executable(fx-workout attic/fx_main.cpp)
    target_link_libraries(fx-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP)
    target_include_directories(fx-workout PUBLIC . ../libs/miniaudio)

    ## Filter Workout
    add_executable(filter-workout attic/filter_main.cpp)
    target_link_libraries(filter-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP)
    target_include_directories(filter-workout PUBLIC . ../libs/miniaudio)

    ## Delay Workout
    # This one uses miniaudio (ant rtmidi) which has much less goop.
    add_executable(delay-workout attic/delay_main.cpp)
    target_link_libraries(delay-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP)
    target_include_directories(delay-workout PUBLIC . ../libs/miniaudio)
endif ()

## Reverb Workout
#juce_add_console_app(reverb-workout
#        PRODUCT_NAME "Reverb Workout"
#)
#
#target_sources(reverb-workout
#        PRIVATE
#        reverb_main.cpp
#        SynthAudioCallback.h
#)
#target_include_directories(reverb-workout PUBLIC .)
#target_compile_definitions(reverb-workout
#        PRIVATE
#        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
#        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
#        JUCE_USE_CURL=0)
#target_link_libraries(reverb-workout
#        PRIVATE
#        juce::juce_core
#        juce::juce_audio_devices
#        juce::juce_dsp
#        ol_corelib
#        ol_fxlib
#        ol_synthlib
#        DaisySP
#        juce::juce_recommended_config_flags
#        juce::juce_recommended_config_flags
#)
#
#juce_add_console_app(convolution-workout
#        PRODUCT_NAME "Convolution Workout"
#)
#
#target_sources(convolution-workout
#        PRIVATE
#        convolution_main.cpp
#        SynthAudioCallback.h
#)
#target_include_directories(convolution-workout PUBLIC .)
#target_compile_definitions(convolution-workout
#        PRIVATE
#        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
#        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
#        JUCE_USE_CURL=0)
#target_link_libraries(convolution-workout
#        PRIVATE
#        juce::juce_core
#        juce::juce_audio_devices
#        juce::juce_dsp
#        ol_corelib
#        ol_fxlib
#        ol_synthlib
#        DaisySP
#        juce::juce_recommended_config_flags
#        juce::juce_recommended_config_flags
#)

if (${BUILD_WORKOUT_BUDDY})
    juce_add_console_app(synth-workout
            PRODUCT_NAME "Synth Workout")
    target_sources(synth-workout
            PRIVATE
            attic/synth_main.cpp
            attic/SynthAudioCallback.h)
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
            workout-buddy
            PUBLIC
            ol_corelib
            ol_ctllib
            ol_synthlib
            DaisySP
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif ()