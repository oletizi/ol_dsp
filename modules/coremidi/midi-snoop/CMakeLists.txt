cmake_minimum_required(VERSION 3.15)
project(midisnoop)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find CoreMIDI and CoreFoundation frameworks
find_library(COREMIDI_LIBRARY CoreMIDI)
find_library(COREFOUNDATION_LIBRARY CoreFoundation)

if(NOT COREMIDI_LIBRARY)
    message(FATAL_ERROR "CoreMIDI framework not found")
endif()

if(NOT COREFOUNDATION_LIBRARY)
    message(FATAL_ERROR "CoreFoundation framework not found")
endif()

# Build the midisnoop executable
add_executable(midisnoop midisnoop.mm)

target_link_libraries(midisnoop
    ${COREMIDI_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
)

# Set output directory
set_target_properties(midisnoop PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install target
install(TARGETS midisnoop
    RUNTIME DESTINATION bin
)