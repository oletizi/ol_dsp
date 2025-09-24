cmake_minimum_required(VERSION 3.30)
project(ol_dsp VERSION 0.1)

# Relax CMake policies for compatibility with older dependencies
if(POLICY CMP0111)
    cmake_policy(SET CMP0111 OLD)  # Allow IMPORTED_LIBNAME to contain paths
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_BUILD_EXTRAS ON)
set(JUCE_BUILD_EXAMPLES ON)
set(BUILD_LLHTTP OFF)
set(BUILD_DAISY OFF)
set(BUILD_TEENSY OFF)

#add_definitions(-DTEENSY_DEBUG)
#add_definitions(-DTEENSY_LOCAL)
# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#include(cmake/CPM.cmake)
include(CPM)
include(JUCEDefaults)

add_subdirectory(../.ol_dsp-deps/DaisySP ${CMAKE_BINARY_DIR}/deps/DaisySP)
add_subdirectory(libs/dattorro-verb)

# FetchContent_Declare(SFML
#         GIT_REPOSITORY https://github.com/SFML/SFML.git
#         GIT_TAG 2.6.x)
# FetchContent_MakeAvailable(SFML)  # TODO: SFML has OpenAL CMake compatibility issues


# JUCE Build Cache Optimization
# Always use add_subdirectory for JUCE to properly use it from source
# The build directory will cache the compiled objects for faster rebuilds
set(JUCE_DEPS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.ol_dsp-deps/libs/JUCE/build")
message(STATUS "Building JUCE from source with cache at: ${JUCE_DEPS_BUILD_DIR}")
add_subdirectory(../.ol_dsp-deps/libs/JUCE ${JUCE_DEPS_BUILD_DIR})
#CPMAddPackage("gh:ImJimmi/JIVE@1.0.4")

if (${BUILD_LLHTTP})
    include(FetchContent)
    FetchContent_Declare(llhttp
            URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v8.1.0.tar.gz")

    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
    FetchContent_MakeAvailable(llhttp)
endif ()

if (${BUILD_DAISY})
    add_subdirectory(libs/libDaisy)
    add_subdirectory(modules/ol_daisy)
endif ()

if (${BUILD_TEENSY})
    add_subdirectory(modules/ol_teensy)
endif ()

#add_subdirectory(libs/rapidyaml)

# Disable JACK support for rtmidi to avoid linking issues
set(RTMIDI_API_JACK OFF CACHE BOOL "Disable JACK support in RtMidi" FORCE)
add_subdirectory(../.ol_dsp-deps/libs/rtmidi ${CMAKE_BINARY_DIR}/deps/rtmidi)
# add_subdirectory(libs/Soundpipe)  # TODO: Soundpipe doesn't have CMakeLists.txt - needs integration work

add_subdirectory(modules/app)
add_subdirectory(modules/corelib)
add_subdirectory(modules/ctllib)
add_subdirectory(modules/fxlib)
add_subdirectory(modules/guilib)
add_subdirectory(modules/iolib)
add_subdirectory(modules/juce)
add_subdirectory(modules/synthlib)
add_subdirectory(workouts)
add_subdirectory(test)
# Test dependencies in ../.ol_dsp-deps/test/ are used by test/ directory

add_library(spline INTERFACE libs/spline/spline.h
        modules/juce/hello/hello.cpp
)
set_target_properties(spline PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(spline INTERFACE libs)

add_library(stmlib
        ../.ol_dsp-deps/libs/stmlib/stmlib/stmlib.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/atan.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/atan.cc
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/cosine_oscillator.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/delay_line.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/dsp.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/filter.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/hysteresis_filter.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/hysteresis_quantizer.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/limiter.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/parameter_interpolator.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/polyblep.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/rsqrt.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/sample_rate_converter.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/units.h
        ../.ol_dsp-deps/libs/stmlib/stmlib/dsp/units.cc
        ../.ol_dsp-deps/libs/stmlib/stmlib/fft/shy_fft.h)
target_include_directories(stmlib PUBLIC ../.ol_dsp-deps/libs/stmlib)
set_target_properties(stmlib PROPERTIES LINKER_LANGUAGE CXX)

if (${BUILD_TEENSY})
    add_library(tgx libs/tgx/src/tgx.h)
    target_include_directories(tgx PUBLIC libs/tgx/src)
    set_target_properties(tgx PROPERTIES LINKER_LANGUAGE CXX)
endif ()
