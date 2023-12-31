//
// Created by Orion Letizi on 12/4/23.
//
#include "workout_buddy.h"

using namespace ol::io;

void note_on_callback(workout_buddy *, uint8_t channel, uint8_t note, uint8_t velocity) {}

void note_off_callback(workout_buddy *, uint8_t channel, uint8_t note, uint8_t velocity) {}

void cc_callback(workout_buddy *, uint8_t channel, uint8_t controller, uint8_t value) {}

void audio_callback(workout_buddy *buddy, t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) {
    auto *sample = static_cast<ol::synth::Sample *>(buddy->audio_data);
    uint64_t channel_count = sample->GetChannelCount();
    t_sample frame_out[channel_count];
    sample->Process(frame_out);
    if (channel_count > 0) {
        *out1 = frame_out[0];
    }
    if (channel_count > 1) {
        *out2 = frame_out[1];
    }
}

int main() {
    ol::io::workout_buddy buddy;
    RtMidiIn midi_in;
    ma_device audio_device;

    auto filename = "/Users/orion/Dropbox/Music/Sample Library/Splice/LOFI MOODS/OS_LFM_90_Drum_Loop_Salty.wav";
    printf("Using audio file: %s", filename);
    
    ma_decoder decoder{};
    auto sample_source = MaSampleSource(filename, &decoder);
    auto sample = ol::synth::Sample(sample_source);


    printf("Starting audio...");
    Workout_Config(&buddy, &midi_in, &audio_device, note_on_callback, note_off_callback, cc_callback, audio_callback,
                   &sample);
    ol::io::InitStatus status = Workout_Init(&buddy);
    if (status != InitStatus::Ok) {
        return status;
    }
    t_sample sample_rate = Workout_SampleRate(&buddy);
    printf("Sample rate: %d\n", int(sample_rate));




//    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, uint32_t(sample_rate));
//    ma_result result = ma_decoder_init_file(filename, &config, &decoder);
//    if (result != MA_SUCCESS) {
//        printf("Could not load file: %s\n", filename);
//        return -2;
//    }

    ol::synth::InitStatus initStatus = sample.Init(sample_rate);
    if (initStatus != ol::synth::InitStatus::Ok) {
        printf("Could not load file: %s\n", filename);
        return -2;
    }

    Workout_Start(&buddy);

    printf("Playing %s\n", filename);
    printf("Replay [r]\n");
    printf("Loop [l]\n");
    printf("Play/pause [p]\n");
    printf("Quit [q|Q] \n");
    printf("command: ");
    while (auto c = getchar()) {
        if (c == 'q' || c == 'Q') {
            break;
        } else if (c == 'l') {
            sample.SetPlayMode(ol::synth::SamplePlayMode::Loop);
            sample.Play();
        } else if (c == 'p') {
            sample.TogglePlay();
        } else if (c == 'r') {
            sample.Seek(0);
            sample.Play();
        }
    }
    return 0;

}