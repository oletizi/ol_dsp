//
// Created by Orion Letizi on 12/9/23.
//
#include "VoiceLoader.h"
#include "ryml.hpp"

namespace ol {
    namespace workout {
        static void print_substr(const c4::csubstr &s) {
            char buf[s.len + 1];
            std::snprintf(buf, s.len + 1, "%.*s", (int) s.len, s.str);
            printf("%s\n", buf);
        }

        void VoiceLoader::Load(ol::synth::VoiceMap voice_map) {

            char char_array[patch_.length() + 1];
            strcpy(char_array, patch_.c_str());
            auto tree = ryml::parse_in_place(char_array);
            auto root = tree.rootref();
            const auto &patch_node = root["patch"];
            const auto &regions = patch_node["regions"];
            for (auto const &region: regions.children()) {
                if (region.has_child("sample")) {
                    auto sample_name = region["sample"];
                    printf("sample has value: %d\n", sample_name.has_val());
                    if (sample_name.has_val()) {
                        printf("  sample name: ");
                        print_substr(sample_name.val());
                    }
                }
            }
        }

    } // ol
} // workout