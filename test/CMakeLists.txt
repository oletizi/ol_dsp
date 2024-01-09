enable_testing()

add_subdirectory(googletest)
add_subdirectory(FakeIt)
find_package(GTest REQUIRED)
#find_package(FakeIt REQUIRED)
add_executable(gtest_run
        basic_test.cpp
        fx_test.cpp
        gui_test.cpp
        workout_test.cpp
        synth_test.cpp
        patch_test.cpp
        serializer_test.cpp
)
message("GTEST_BOTH_LIBRARIES: ${GTEST_BOTH_LIBRARIES}")
target_link_libraries(gtest_run ${GTEST_BOTH_LIBRARIES} gmock FakeIt::FakeIt-gtest ol_corelib ol_ctllib ol_fxlib ol_guilib ol_iolib ol_synthlib DaisySP)
target_include_directories(gtest_run PUBLIC FakeIt/single_header)
add_test(gtest_run gtest_run)