cmake_minimum_required(VERSION 3.26)
project(juce_test)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

add_executable(juce_test main.cpp)
