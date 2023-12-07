add_library(ol_synthlib
        SynthVoice.cpp
        Polyvoice.cpp
        SampleSoundSource.cpp
        OscillatorSoundSource.cpp
        SampleDataSource.h
        SampleSoundSource.h
        PitchedSoundSource.h
        OscillatorSoundSource.h
        SynthVoice.h
        Polyvoice.h
        MultiChannelSample.cpp
        MultiChannelSample.h)
target_link_libraries(ol_synthlib ol_corelib ol_ctllib DaisySP)
target_include_directories(ol_synthlib PUBLIC . ..)