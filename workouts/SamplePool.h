//
// Created by Orion Letizi on 12/10/23.
//

#ifndef OL_DSP_SAMPLEPOOL_H
#define OL_DSP_SAMPLEPOOL_H

#include "ol_synthlib.h"
#include "PatchLoader.h"
#include "workout_buddy.h"

namespace ol::workout {
    template< int CHANNEL_COUNT, int POOL_SIZE>
    class SamplePool : public PatchLoader::PatchLoaderCallback {
    private:
        struct voice_data {
            ol::synth::SampleDataSource *data_source;
            ol::synth::Voice *voice;
        };
        voice_data *voices[POOL_SIZE] = {};
        synth::VoiceMap<CHANNEL_COUNT> &voice_map_;
        PatchLoader &patch_loader_;
        int pool_index_ = 0;
        t_sample sample_rate_ = 0;

    public:
        SamplePool(ol::synth::VoiceMap<CHANNEL_COUNT> &voice_map, ol::synth::SampleDataSource *sources[POOL_SIZE],
                              PatchLoader &patch_loader) : voice_map_(voice_map),
                                                           patch_loader_(patch_loader) {
            for (int i = 0; i < POOL_SIZE; i++) {
                auto data_source = sources[i];
                auto sample = new ol::synth::MultiChannelSample(*data_source);
                auto sample_sound_source = new ol::synth::SampleSoundSource<CHANNEL_COUNT>(sample);
                daisysp::Svf *filters[CHANNEL_COUNT] = {};
                for (int j=0; j<CHANNEL_COUNT; j++) {
                    filters[j] = new daisysp::Svf();
                }
                auto f_env = daisysp::Adsr();
                auto a_env = daisysp::Adsr();
                auto port = daisysp::Port();
                auto voice = new ol::synth::SynthVoice<CHANNEL_COUNT>(sample_sound_source, filters, f_env, a_env, port);
                voices[i] = new voice_data{data_source, voice};
            }
        }

        ol::synth::InitStatus LoadSample(uint8_t note, std::string sample_path) override {
            if (pool_index_ >= POOL_SIZE) {
                printf("Can't load any more samples! Pool size: %d, samples loaded: %d\n", POOL_SIZE, pool_index_);
                return ol::synth::InitStatus::Error;
            }
            printf("I should load %d => %s\n", note, sample_path.c_str());
            // initialize the sample data source
            voices[pool_index_]->data_source->Init(sample_rate_, sample_path.c_str());
            // set the voice in the voice map
            voice_map_.SetVoice(note, voices[pool_index_]->voice);
            pool_index_++;
            return ol::synth::InitStatus::Ok;
        }

        inline ol::synth::InitStatus Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            auto status = patch_loader_.Load(this);
            for (int i=0;i<pool_index_; i++) {
                voices[i]->voice->Init(sample_rate);
            }
            return status;
        }
    };
}


#endif //OL_DSP_SAMPLEPOOL_H
