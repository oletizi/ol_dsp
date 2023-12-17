juce_add_plugin(ol-workout-plugin
#        COMPANY_NAME chowdsp
#        PLUGIN_MANUFACTURER_CODE Chow
#        PLUGIN_CODE Jdox
#        FORMATS ${JUCE_FORMATS}
#        ProductName "CHOWTapeModel"

        PRODUCT_NAME "ol-workout-plugin"
#        VERSION 0.1.0
#        BUNDLE_ID "com.orionletizi.ol-workout-plugin"
        COMPANY_NAME oletizi
#        COMPANY_WEBSITE "https://orionletizi.com"
#        COMPANY_EMAIL "orion@orionletizi.com"
        FORMATS AU Standalone
        PLUGIN_NAME "OL Workout"
        PLUGIN_MANUFACTURER_CODE Olet
        PLUGIN_CODE Olwo
        DESCRIPTION "My Rockin Plugin."
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        COPY_PLUGIN_AFTER_BUILD TRUE
        AU_COPY_DIR /Users/orion/Library/Audio/Plug-Ins/Components
)
target_sources(ol-workout-plugin PUBLIC src/ol_workout_plugin.cpp src/PluginProcessor.cpp)
target_link_libraries(ol-workout-plugin PRIVATE
        juce::juce_core
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_dsp
        ol_fxlib
        ol_synthlib)
target_include_directories(ol-workout-plugin PUBLIC src)
juce_generate_juce_header(ol-workout-plugin)
#add_subdirectory(src)