juce_add_console_app(midi_server
    PRODUCT_NAME "OL MIDI Server"
)

target_sources(midi_server
    PRIVATE
    MidiServer.cpp
)

target_compile_definitions(midi_server
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(midi_server
    PRIVATE
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_events
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)