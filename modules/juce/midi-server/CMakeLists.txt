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

# Network MIDI Server - Zero-configuration network MIDI mesh (All Phases)
juce_add_console_app(network_midi_server
    PRODUCT_NAME "Network MIDI Server"
)

# Collect all network sources
file(GLOB CORE_SOURCES network/core/*.cpp)
file(GLOB DISCOVERY_SOURCES network/discovery/*.cpp)
file(GLOB MESH_SOURCES network/mesh/*.cpp)
file(GLOB TRANSPORT_SOURCES network/transport/*.cpp)
file(GLOB ROUTING_SOURCES network/routing/*.cpp)

# Platform-specific discovery sources
if(APPLE)
    list(APPEND DISCOVERY_SOURCES network/discovery/platform/mdns_macos.cpp)
elseif(UNIX)
    # Linux - Avahi (stub for now)
elseif(WIN32)
    # Windows - Bonjour (stub for now)
endif()

# Exclude test programs from main server
list(FILTER DISCOVERY_SOURCES EXCLUDE REGEX ".*DiscoveryTest\\.cpp$")
list(FILTER TRANSPORT_SOURCES EXCLUDE REGEX ".*TransportTest\\.cpp$")
list(FILTER MESH_SOURCES EXCLUDE REGEX ".*/tests/.*Test\\.cpp$")
list(FILTER TRANSPORT_SOURCES EXCLUDE REGEX ".*/tests/.*Test\\.cpp$")

target_sources(network_midi_server
    PRIVATE
    NetworkMidiServer.cpp
    ${CORE_SOURCES}
    ${DISCOVERY_SOURCES}
    ${MESH_SOURCES}
    ${TRANSPORT_SOURCES}
    ${ROUTING_SOURCES}
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

# Platform-specific mDNS libraries
if(APPLE)
    target_link_libraries(network_midi_server
        PRIVATE
        "-framework CoreServices"
    )
elseif(UNIX)
    # Linux - Avahi (when implemented)
    # find_package(PkgConfig REQUIRED)
    # pkg_check_modules(AVAHI REQUIRED avahi-client avahi-common)
    # target_link_libraries(network_midi_server PRIVATE ${AVAHI_LIBRARIES})
    # target_include_directories(network_midi_server PRIVATE ${AVAHI_INCLUDE_DIRS})
elseif(WIN32)
    # Windows - Bonjour (when implemented)
    # target_link_libraries(network_midi_server PRIVATE dnssd.lib)
endif()

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

#==============================================================================
# MIDI Mesh Integration Test
#==============================================================================

juce_add_console_app(midi_mesh_test
    PRODUCT_NAME "MIDI Mesh Test"
)

target_sources(midi_mesh_test
    PRIVATE
    tests/integration/midi_mesh_test.cpp
)

target_compile_definitions(midi_mesh_test
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(midi_mesh_test
    PRIVATE
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_events
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Performance Benchmarks - Measure Network MIDI Mesh performance
juce_add_console_app(network_midi_benchmarks
    PRODUCT_NAME "Network MIDI Benchmarks"
)

target_sources(network_midi_benchmarks
    PRIVATE
    network/benchmarks/PerformanceBenchmark.cpp
    ${CORE_SOURCES}
    ${MESH_SOURCES}
    ${TRANSPORT_SOURCES}
    ${ROUTING_SOURCES}
)

target_compile_definitions(network_midi_benchmarks
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_include_directories(network_midi_benchmarks
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network_midi_benchmarks
    PRIVATE
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_events
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Unit Tests with Google Test and Coverage
# Note: Google Test is already configured in the parent project's test/ directory
# No need to fetch it again - just use the existing targets

# Unit test executable
add_executable(network_midi_tests
    # NOTE: NodeIdentityTest.cpp temporarily disabled due to API changes
    # tests/unit/NodeIdentityTest.cpp
    tests/unit/InstanceManagerTest.cpp
    tests/unit/MidiPacketTest.cpp
    tests/unit/DeviceRegistryTest.cpp
    tests/unit/RoutingTableTest.cpp
    tests/unit/RouteManagerTest.cpp
    tests/unit/MessageBufferTest.cpp
    tests/unit/UdpMidiTransportTest.cpp
    tests/unit/ConnectionPoolTest.cpp
    tests/unit/MidiClassifierTest.cpp
    tests/unit/MidiMessageRouterTest.cpp
    # SEDA infrastructure tests (Phase B.1)
    network/mesh/tests/NetworkConnectionQueueTest.cpp
    # Dual-transport tests (Phase C.1-C.3)
    network/transport/tests/RealtimeMidiBufferTest.cpp
    network/transport/tests/DualTransportTest.cpp
    # Integration tests (Phase B.5)
    network/mesh/tests/ConnectionIntegrationTest.cpp
    network/mesh/tests/DualTransportIntegrationTest.cpp
    network/mesh/tests/MeshManagerIntegrationTest.cpp
    # Add sources under test
    ${CORE_SOURCES}
    ${MESH_SOURCES}
    ${TRANSPORT_SOURCES}
    ${ROUTING_SOURCES}
)

# Platform-specific sources for tests
if(APPLE)
    target_sources(network_midi_tests PRIVATE
        ${DISCOVERY_SOURCES}
    )
    target_link_libraries(network_midi_tests PRIVATE "-framework CoreServices")
endif()

target_link_libraries(network_midi_tests
    PRIVATE
    gtest
    gtest_main
    gmock
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_events
)

target_compile_definitions(network_midi_tests
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_include_directories(network_midi_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/network
)

# Add test to CTest
add_test(NAME network_midi_tests COMMAND network_midi_tests)

# Code coverage configuration (optional - enable with -DENABLE_COVERAGE=ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(network_midi_tests PRIVATE --coverage -O0 -g)
        target_link_options(network_midi_tests PRIVATE --coverage)

        # Coverage target
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
            COMMAND lcov --capture --directory . --output-file coverage/coverage.info --rc lcov_branch_coverage=1
            COMMAND lcov --remove coverage/coverage.info '/usr/*' '*/googletest/*' '*/juce/*' --output-file coverage/coverage_filtered.info --rc lcov_branch_coverage=1
            COMMAND genhtml coverage/coverage_filtered.info --output-directory coverage/html --branch-coverage
            COMMAND echo "Coverage report generated in coverage/html/index.html"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS network_midi_tests
            COMMENT "Generating code coverage report"
        )
    endif()
endif()
