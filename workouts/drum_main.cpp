//
// Created by Orion Letizi on 12/8/23.
//
#include <fstream>
#include <sstream>
#include "workout_buddy.h"
#include "PatchLoader.h"
#include "SamplePool.h"

#define VOICE_COUNT 4
#define CHANNEL_COUNT 2

using namespace ol::workout;

void note_on_callback(workout_buddy *buddy, uint8_t channel, uint8_t note, uint8_t velocity) {
    auto voices = static_cast<ol::synth::Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *>(buddy->audio_data);
    voices->NoteOn(note, velocity);
}

void note_off_callback(workout_buddy *buddy, uint8_t channel, uint8_t note, uint8_t velocity) {
    auto voices = static_cast<ol::synth::Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *>(buddy->audio_data);
    voices->NoteOff(note, velocity);
}

void cc_callback(workout_buddy *buddy, uint8_t channel, uint8_t controller, uint8_t value) {
    auto voices = static_cast<ol::synth::Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *>(buddy->audio_data);
    voices->UpdateMidiControl(controller, value);
}


void audio_callback(workout_buddy *buddy, t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) {
    t_sample out_buffer[] = {0, 0};
    auto *voices = static_cast<ol::synth::Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *>(buddy->audio_data);
    //sample->Process(frame_out);
    voices->Process(out_buffer);
    *out1 = out_buffer[0];
    *out2 = out_buffer[1];
}

int main() {
    ol::workout::workout_buddy buddy;
    RtMidiIn midi_in;
    ma_device audio_device;


    const int pool_size = 4;

    ol::synth::SampleDataSource *data_sources[pool_size] = {};
    for (auto &data_source: data_sources) {
        data_source = new MaSampleSource("", new ma_decoder);
    }
    auto patch_path = "/Users/orion/work/ol_dsp/test/drumkit/drumkit.yaml";
    std::fstream patch_stream(patch_path);
    std::stringstream patch_buffer;
    patch_buffer << patch_stream.rdbuf();
    patch_buffer.str();
    auto patch = patch_buffer.str();
    printf("Patch: %s", patch.c_str());

    auto patch_loader = PatchLoader("/Users/orion/work/ol_dsp/test/drumkit/", patch);

    auto voicemap = ol::synth::VoiceMap<CHANNEL_COUNT>();
    auto pool = SamplePool<pool_size, CHANNEL_COUNT>(voicemap, data_sources, patch_loader);

    ol::synth::Polyvoice<CHANNEL_COUNT, VOICE_COUNT> voices(voicemap);

    printf("Starting audio...");
    Workout_Config(&buddy, &midi_in, &audio_device, note_on_callback, note_off_callback, cc_callback, audio_callback,
                   &voices);
    ol::workout::InitStatus status = Workout_Init(&buddy);
    if (status != InitStatus::Ok) {
        return status;
    }
    t_sample sample_rate = Workout_SampleRate(&buddy);
    printf("Sample rate: %d\n", int(sample_rate));

    pool.Init(sample_rate);

    Workout_Start(&buddy);

    printf("Play note: p\n");
    printf("Swap samples: s\n");
    printf("command: ");
    while (auto c = getchar()) {
        if (c == 'p') {
            printf("  playing note...\n");
            voices.NoteOff(60, 100);
            voices.NoteOn(60, 100);
        }
    }
    return 0;
}