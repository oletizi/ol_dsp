cmake_minimum_required(VERSION 3.26)
project(ol_dsp VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_BUILD_EXTRAS OFF)
set(JUCE_BUILD_EXAMPLES OFF)
set(BUILD_LLHTTP OFF)
set(BUILD_DAISY OFF)
set(BUILD_TEENSY OFF)

add_subdirectory(libs/DaisySP)
add_subdirectory(libs/dattorro-verb)
add_subdirectory(libs/JUCE)

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
    add_subdirectory(modules/daisy)
endif ()

if (${BUILD_TEENSY})
    add_subdirectory(modules/ol_teensy)
endif ()

add_subdirectory(libs/rapidyaml)
add_subdirectory(libs/rtmidi)
add_subdirectory(libs/Soundpipe)

add_subdirectory(modules/corelib)
add_subdirectory(modules/ctllib)
add_subdirectory(modules/fxlib)
add_subdirectory(modules/guilib)
add_subdirectory(modules/iolib)
#add_subdirectory(modules/juce/plugin/workout)
add_subdirectory(modules/synthlib)
#add_subdirectory(workouts)
add_subdirectory(test)




add_library(stmlib
        libs/stmlib/stmlib/stmlib.h
        libs/stmlib/stmlib/dsp/atan.h
        libs/stmlib/stmlib/dsp/atan.cc
        libs/stmlib/stmlib/dsp/cosine_oscillator.h
        libs/stmlib/stmlib/dsp/delay_line.h
        libs/stmlib/stmlib/dsp/dsp.h
        libs/stmlib/stmlib/dsp/filter.h
        libs/stmlib/stmlib/dsp/hysteresis_filter.h
        libs/stmlib/stmlib/dsp/hysteresis_quantizer.h
        libs/stmlib/stmlib/dsp/limiter.h
        libs/stmlib/stmlib/dsp/parameter_interpolator.h
        libs/stmlib/stmlib/dsp/polyblep.h
        libs/stmlib/stmlib/dsp/rsqrt.h
        libs/stmlib/stmlib/dsp/sample_rate_converter.h
        libs/stmlib/stmlib/dsp/units.h
        libs/stmlib/stmlib/dsp/units.cc
        libs/stmlib/stmlib/fft/shy_fft.h)
target_include_directories(stmlib PUBLIC libs/stmlib)
set_target_properties(stmlib PROPERTIES LINKER_LANGUAGE CXX)