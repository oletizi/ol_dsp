//
// Created by Orion Letizi on 12/4/23.
//
#include "workout_buddy.h"

using namespace ol::workout;


void note_on_callback(workout_buddy *, uint8_t channel, uint8_t note, uint8_t velocity) {}

void note_off_callback(workout_buddy *, uint8_t channel, uint8_t note, uint8_t velocity) {}

void cc_callback(workout_buddy *, uint8_t channel, uint8_t controller, uint8_t value) {}

void audio_callback(workout_buddy *buddy, t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) {
    auto sample = static_cast<ol::synth::Sample *>(buddy->audio_data);
    auto result = sample->Process();
    *out1 = result;
    *out2 = result;
}

int main() {
    ol::workout::workout_buddy buddy;
    RtMidiIn midi_in;
    ma_device audio_device;

    ma_decoder decoder{};
    auto sample_source = MaSampleSource(&decoder);
    auto sample = ol::synth::Sample(sample_source);


    printf("Starting audio...");
    Workout_Config(&buddy, &midi_in, &audio_device, note_on_callback, note_off_callback, cc_callback, audio_callback,
                   &sample);
    ol::workout::InitStatus status = Workout_Init(&buddy);
    if (status != InitStatus::Ok) {
        return status;
    }
    t_sample sample_rate = Workout_SampleRate(&buddy);
    printf("Sample rate: %d\n", int(sample_rate));

    auto filename = "/Users/orion/Dropbox/Music/Sample Library/Splice/LOFI MOODS/OS_LFM_90_Drum_Loop_Salty.wav";
    printf("Loading audio file: %s", filename);


    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, uint32_t(sample_rate));
    ma_result result = ma_decoder_init_file(filename, &config, &decoder);
    if (result != MA_SUCCESS) {
        printf("Could not load file: %s\n", filename);
        return -2;
    }

    sample.Init(sample_rate);

    Workout_Start(&buddy);

    printf("Playing %s\n", filename);
    printf("Quit [q|Q]: ");
    while (auto c = getchar()) {
        if (c == 'q' || c == 'Q') {
            break;
        }
    }
    return 0;

}