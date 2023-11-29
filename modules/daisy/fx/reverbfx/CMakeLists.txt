add_executable(reverbfx main.cpp)
target_link_libraries(reverbfx DaisySP libDaisy)
target_include_directories(reverbfx PUBLIC
        ../../../libDaisy/src
        ../../ol_dsp/corelib
        ../../ol_dsp/ctllib
        ../../ol_dsp/fxlib
        ../../ol_dsp/synthlib
)