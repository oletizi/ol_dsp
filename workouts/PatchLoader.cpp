//
// Created by Orion Letizi on 12/9/23.
//
#include "PatchLoader.h"

namespace ol::workout {

    ol::synth::InitStatus PatchLoader::Load(PatchLoaderCallback *callback) {
        // XXX: This is super fragile and bad
        ol::synth::Voice::Config config = {};
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
                    fill_buf(sample_name.val());
                    std::string sample_path = patch_path_ + std::string(buf);
                    printf("  sample name: %s\n", buf);
                    printf("  patch path: %s\n", patch_path_);
                    printf("  sample path: %s\n", sample_path.c_str());
                    auto note = region["note"];
                    uint64_t note_value = 0;
                    if (note.valid() && note.has_val() && note.val().is_unsigned_integer()) {
                        atou(note.val(), &note_value);
                        printf("  note: %llu\n", note_value);
                    }
                    auto channel = region["channel"];
                    uint64_t channel_value = 0;
                    if (channel.valid() && channel.has_val() && channel.val().is_unsigned_integer()) {
                        atou(channel.val(), &channel_value);
                        printf("   channel: %llu\n", channel_value);
                    }
                    if (note_value && channel_value) {
                        auto status = callback->LoadSample(config, channel_value, note_value, sample_path);
                        if (status != ol::synth::InitStatus::Ok) {
                            return status;
                        }
                    } else {
                        printf("Ignoring sample (bad/missing note or channel): channel: %llu, note: %llu",
                               channel_value, note_value);
                    }
                }
            }
        }
        return ol::synth::InitStatus::Ok;
    }

} // ol
// workout