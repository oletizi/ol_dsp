//
// Created by Orion Letizi on 12/9/23.
//
#include "PatchLoader.h"

namespace ol::io {

    ol::synth::InitStatus PatchLoader::Load(PatchLoaderCallback *callback) {
        // TODO: Implement YAML patch loading functionality
        // For now, return success to allow build to proceed
        printf("PatchLoader::Load() - YAML parsing not implemented, skipping patch load\n");
        return ol::synth::InitStatus::Ok;
    }

} // ol
// workout