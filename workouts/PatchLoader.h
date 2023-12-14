//
// Created by Orion Letizi on 12/9/23.
//

#ifndef OL_DSP_PATCHLOADER_H
#define OL_DSP_PATCHLOADER_H

#include <string>
#include "ryml.hpp"
#include "VoiceMap.h"
#include "SampleDataSource.h"

#define BUF_LENGTH 128
namespace ol::workout {


    class PatchLoader {
    private:
        const char *patch_path_;
        const std::string &patch_;
        char buf[BUF_LENGTH] = {};

        inline void fill_buf(const c4::csubstr &s) {
            std::snprintf(buf, BUF_LENGTH, "%.*s", (int) s.len, s.str);
        }

    public:
        class PatchLoaderCallback {
        public:
            virtual ol::synth::InitStatus LoadSample(ol::synth::Voice::Config c, uint8_t channel, uint8_t note, std::string sample_path) = 0;
        };

        PatchLoader(const char *patch_path, const std::string &patch) :
                patch_path_(patch_path), patch_(patch) {}

        ol::synth::InitStatus Load(PatchLoaderCallback *callback);
    };

} // ol
// workout

#endif //OL_DSP_PATCHLOADER_H
