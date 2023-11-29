cmake_minimum_required(VERSION 3.26)
project(ol_dsp VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(libs/JUCE)
add_subdirectory(libs/DaisySP)
add_subdirectory(libs/libDaisy)
add_subdirectory(libs/dattorro-verb)
add_subdirectory(libs/rtmidi)
add_subdirectory(modules/corelib)
add_subdirectory(modules/ctllib)
add_subdirectory(modules/daisy)
add_subdirectory(modules/fxlib)
add_subdirectory(modules/synthlib)


add_library(stk libs/stk/src/RtMidi.cpp)
target_include_directories(stk PUBLIC stk/include)

add_subdirectory(workouts)
add_subdirectory(test)



#add_executable(juce_test main.cpp)
