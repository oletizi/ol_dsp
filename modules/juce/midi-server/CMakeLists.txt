# MIDI Device Tester - CLI tool for device-specific testing
juce_add_console_app(midi_device_tester
    PRODUCT_NAME "OL MIDI Device Tester"
)

target_sources(midi_device_tester
    PRIVATE
    MidiDeviceTester.cpp
)

target_compile_definitions(midi_device_tester
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(midi_device_tester
    PRIVATE
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_events
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Network MIDI Server - Zero-configuration network MIDI mesh (Phase 1)
juce_add_console_app(network_midi_server
    PRODUCT_NAME "Network MIDI Server"
)

# Phase 1: Core files only (NodeIdentity and InstanceManager)
target_sources(network_midi_server
    PRIVATE
    NetworkMidiServer.cpp
    network/core/NodeIdentity.cpp
    network/core/InstanceManager.cpp
)

target_compile_definitions(network_midi_server
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(network_midi_server
    PRIVATE
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_events
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Add include directory for network modules
target_include_directories(network_midi_server
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Network Discovery Test Program (Phase 2)
juce_add_console_app(discovery_test
    PRODUCT_NAME "Network Discovery Test"
)

target_sources(discovery_test
    PRIVATE
    network/discovery/DiscoveryTest.cpp
    network/discovery/ServiceDiscovery.cpp
    network/discovery/FallbackDiscovery.cpp
)

# Platform-specific test sources
if(APPLE)
    target_sources(discovery_test
        PRIVATE
        network/discovery/platform/mdns_macos.cpp
    )
endif()

target_compile_definitions(discovery_test
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_include_directories(discovery_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(discovery_test
    PRIVATE
    juce::juce_core
    juce::juce_events
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Platform-specific mDNS libraries for discovery_test
if(APPLE)
    target_link_libraries(discovery_test
        PRIVATE
        "-framework CoreServices"
    )
elseif(UNIX)
    # Linux - Avahi (when implemented)
elseif(WIN32)
    # Windows - Bonjour (when implemented)
endif()
