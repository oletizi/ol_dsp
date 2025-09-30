cmake_minimum_required(VERSION 3.15)
project(SnoizeMIDISpy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Find required frameworks
find_library(COREMIDI_LIBRARY CoreMIDI)
find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(FOUNDATION_LIBRARY Foundation)

if(NOT COREMIDI_LIBRARY OR NOT COREFOUNDATION_LIBRARY OR NOT FOUNDATION_LIBRARY)
    message(FATAL_ERROR "Required macOS frameworks not found")
endif()

# Build the MIDI Spy Driver Plugin
set(DRIVER_SOURCES
    driver/MIDIDriver.cpp
    driver/SpyingMIDIDriver.cpp
    driver/MessagePortBroadcaster.cpp
    driver/MessageQueue.c
)

add_library(SpyingMIDIDriver MODULE ${DRIVER_SOURCES})

target_include_directories(SpyingMIDIDriver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SpyingMIDIDriver
    ${COREMIDI_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
)

# Set bundle properties for the driver
set_target_properties(SpyingMIDIDriver PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "plugin"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info-SpyingMIDIDriver.plist"
    MACOSX_BUNDLE_BUNDLE_NAME "SpyingMIDIDriver"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.snoize.SpyingMIDIDriver"
)

# Build the SnoizeMIDISpy Framework (static library for now)
set(FRAMEWORK_SOURCES
    framework/MIDISpyClient.c
    framework/MIDISpyDriverInstallation.m
)

add_library(SnoizeMIDISpy STATIC ${FRAMEWORK_SOURCES})

target_include_directories(SnoizeMIDISpy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SnoizeMIDISpy
    ${COREMIDI_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
    ${FOUNDATION_LIBRARY}
)

# Install driver to user's MIDI Drivers directory
install(TARGETS SpyingMIDIDriver
    LIBRARY DESTINATION "$ENV{HOME}/Library/Audio/MIDI Drivers"
)