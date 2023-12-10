//
// Created by Orion Letizi on 12/9/23.
//

#ifndef OL_DSP_VOICELOADER_H
#define OL_DSP_VOICELOADER_H
#include <string>
#include "VoiceMap.h"

namespace ol {
    namespace workout {

        class VoiceLoader {

        private:
            const std::string &patch_path_;
            const std::string &patch_;

        public:
            VoiceLoader(const std::string &patch_path, const std::string &patch) :
                    patch_path_(patch_path), patch_(patch) {}

            void Load(ol::synth::VoiceMap voice_map);

        };

    } // ol
} // workout

#endif //OL_DSP_VOICELOADER_H
