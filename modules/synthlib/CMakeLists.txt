add_library(ol_synthlib
        SampleSoundSource.cpp
        SampleDataSource.h
        SampleSoundSource.h
        SoundSource.h
        OscillatorSoundSource.h
        SynthVoice.h
        Polyvoice.h
        MultiChannelSample.cpp
        MultiChannelSample.h
        VoiceMap.h)
target_link_libraries(ol_synthlib ol_corelib ol_ctllib DaisySP)
target_include_directories(ol_synthlib PUBLIC . ..)