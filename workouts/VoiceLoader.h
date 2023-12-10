//
// Created by Orion Letizi on 12/9/23.
//

#ifndef OL_DSP_VOICELOADER_H
#define OL_DSP_VOICELOADER_H

#include <string>
#include "ryml.hpp"
#include "VoiceMap.h"
#include "SampleDataSource.h"

#define BUF_LENGTH 128
namespace ol::workout {

    class VoiceLoader {
    private:
        const char *patch_path_;
        const std::string &patch_;
        char buf[BUF_LENGTH] = {};
        ol::synth::SampleDataSource *sources[128] = {};

        inline void fill_buf(const c4::csubstr &s) {
            std::snprintf(buf, BUF_LENGTH, "%.*s", (int) s.len, s.str);
        }

    public:
        typedef ol::synth::SampleDataSource (*data_source_factory)(const char *sample_path);

        VoiceLoader(const char *patch_path, const std::string &patch) :
                patch_path_(patch_path), patch_(patch) {}

        void Load(data_source_factory factory, ol::synth::VoiceMap voice_map);
    };

} // ol
// workout

#endif //OL_DSP_VOICELOADER_H
