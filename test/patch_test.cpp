//
// Created by Orion Letizi on 12/14/23.
//
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "iolib/ol_iolib.h"

using namespace ol::synth;
using namespace ol::io;

class MockPatchLoaderCallback : public PatchLoader::PatchLoaderCallback {
public:
    MOCK_METHOD(InitStatus, LoadSample,
                (ol::synth::Voice::Config c, uint8_t channel, uint8_t note, std::string sample_path), (override));
};

TEST(PatchLoader, LoadSample) {
//    auto callback = new MockPatchLoaderCallback();
//    auto patch_directory = "/path/to/patch/directory";
//    auto patch = "patch text here";
//
//    PatchLoader loader(patch_directory, patch);
//    loader.Load(callback);
}