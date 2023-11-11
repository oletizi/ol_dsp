cmake_minimum_required(VERSION 3.26)
project(ol_dsp VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)
add_subdirectory(DaisySP)
add_subdirectory(fxlib)
add_subdirectory(synthlib)


add_subdirectory(workouts)
add_subdirectory(test)



#add_executable(juce_test main.cpp)
