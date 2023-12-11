add_library(ol_synthlib
        SampleSoundSource.cpp
        OscillatorSoundSource.cpp
        SampleDataSource.h
        SampleSoundSource.h
        SoundSource.h
        OscillatorSoundSource.h
        SynthVoice.h
        SynthVoice.cpp
        Polyvoice.h
        MultiChannelSample.cpp
        MultiChannelSample.h
        VoiceMap.h)
target_link_libraries(ol_synthlib ol_corelib ol_ctllib DaisySP)
target_include_directories(ol_synthlib PUBLIC . ..)