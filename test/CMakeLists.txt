enable_testing()

add_subdirectory(googletest)
find_package(GTest REQUIRED)
add_executable(gtest_run
        basic_test.cpp
        fx_test.cpp
        reverb_test.cpp
        workout_test.cpp
        synth_test.cpp
)
message("GTEST_BOTH_LIBRARIES: ${GTEST_BOTH_LIBRARIES}")
target_link_libraries(gtest_run ${GTEST_BOTH_LIBRARIES} gmock ol_corelib ol_ctllib ol_fxlib ol_synthlib DaisySP soundpipe)
add_test(gtest_run gtest_run)