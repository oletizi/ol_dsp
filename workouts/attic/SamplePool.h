//
// Created by Orion Letizi on 12/10/23.
//

#ifndef OL_DSP_SAMPLEPOOL_H
#define OL_DSP_SAMPLEPOOL_H

#include "ol_synthlib.h"
#include "iolib/PatchLoader.h"
#include "../workout_buddy.h"

namespace ol::io {
    struct VoiceData {
        ol::synth::SampleDataSource *data_source;
        ol::synth::Voice *voice;
    };

    template<int CHANNEL_COUNT, int POOL_SIZE>
    class SamplePool : public PatchLoader::PatchLoaderCallback {
    private:
        VoiceData *voice_data_[POOL_SIZE] = {};
        synth::VoiceMap<CHANNEL_COUNT> &voice_map_;
        PatchLoader &patch_loader_;
        int pool_index_ = 0;
        t_sample sample_rate_ = 0;

    public:
        SamplePool(VoiceData *voice_data[POOL_SIZE],
                   ol::synth::VoiceMap<CHANNEL_COUNT> &voice_map,
                   PatchLoader &patch_loader) : voice_map_(voice_map),
                                                patch_loader_(patch_loader) {
            for (int i = 0; i < POOL_SIZE; i++) {
                voice_data_[i] = voice_data[i];
            }
        }

        ol::synth::InitStatus LoadSample(ol::synth::Voice::Config config, uint8_t channel, uint8_t note, std::string sample_path) override {
            if (pool_index_ >= POOL_SIZE) {
                printf("Can't load any more samples! Pool size: %d, samples loaded: %d\n", POOL_SIZE, pool_index_);
                return ol::synth::InitStatus::Error;
            }
            printf("I should load %d => %s\n", note, sample_path.c_str());
            // initialize the sample data source
            voice_data_[pool_index_]->data_source->Init(sample_rate_, sample_path.c_str());
            // set the voice in the voice map
            // XXX: Check that note and channel aren't out of bounds.
            // Channel is zero-based internally.
            synth::Voice *voice = voice_data_[pool_index_]->voice;
            voice->UpdateConfig(config);
            voice_map_.SetVoice(channel - 1, note, voice);
            pool_index_++;
            return ol::synth::InitStatus::Ok;
        }

        inline ol::synth::InitStatus Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            auto status = patch_loader_.Load(this);
            for (int i = 0; i < pool_index_; i++) {
                voice_data_[i]->voice->Init(sample_rate);
            }
            return status;
        }
    };
}


#endif //OL_DSP_SAMPLEPOOL_H
