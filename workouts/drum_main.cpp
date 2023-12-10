//
// Created by Orion Letizi on 12/8/23.
//
#include <fstream>
#include <sstream>
#include "workout_buddy.h"
#include "synthlib/ol_synthlib.h"
#include "VoiceLoader.h"

using namespace ol::workout;

void note_on_callback(workout_buddy *buddy, uint8_t channel, uint8_t note, uint8_t velocity) {
    auto voices = static_cast<ol::synth::Polyvoice *>(buddy->audio_data);
    voices->NoteOn(note, velocity);
}

void note_off_callback(workout_buddy *buddy, uint8_t channel, uint8_t note, uint8_t velocity) {
    auto voices = static_cast<ol::synth::Polyvoice *>(buddy->audio_data);
    voices->NoteOff(note, velocity);
}

void cc_callback(workout_buddy *buddy, uint8_t channel, uint8_t controller, uint8_t value) {
    auto voices = static_cast<ol::synth::Polyvoice *>(buddy->audio_data);
    voices->UpdateMidiControl(controller, value);
}


void audio_callback(workout_buddy *buddy, t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) {
    t_sample out_buffer[] = {0, 0};
    auto *voices = static_cast<ol::synth::Polyvoice *>(buddy->audio_data);
    //sample->Process(frame_out);
    voices->Process(out_buffer);
    *out1 = out_buffer[0];
    *out2 = out_buffer[1];
}

int main() {
    ol::workout::workout_buddy buddy;
    RtMidiIn midi_in;
    ma_device audio_device;

    auto kickfile = "/Users/orion/Dropbox/Music/Sample Library/Splice/LOFI MOODS/OS_LFM_Base_Kick.wav";
    ma_decoder kickdecoder{};
    auto kick_sample_data_source = MaSampleSource(&kickdecoder);
    auto kick_sample = ol::synth::MultiChannelSample(kick_sample_data_source);
    auto kick_sound_source = ol::synth::SampleSoundSource(kick_sample);

    auto voicemap = ol::synth::VoiceMap();

    auto patch_path = "/Users/orion/work/ol_dsp/test/drumkit/drumkit.yaml";
    std::fstream patch_stream(patch_path);
    std::stringstream patch_buffer;
    patch_buffer << patch_stream.rdbuf();
    auto patch = patch_buffer.str();

    auto voiceLoader = VoiceLoader(patch_path, patch);
    voiceLoader.Load(voicemap);

    auto voices = ol::synth::Polyvoice(voicemap);
    daisysp::Svf kick_filter;
    daisysp::Adsr kick_filter_env;
    daisysp::Adsr kick_amp_env;
    daisysp::Port kick_port;
    auto kick_voice = ol::synth::SynthVoice(kick_sound_source, kick_filter, kick_filter_env, kick_amp_env, kick_port);

    voicemap.SetVoice(60, kick_voice);


    printf("Starting audio...");
    Workout_Config(&buddy, &midi_in, &audio_device, note_on_callback, note_off_callback, cc_callback, audio_callback,
                   &voices);
    ol::workout::InitStatus status = Workout_Init(&buddy);
    if (status != InitStatus::Ok) {
        return status;
    }
    t_sample sample_rate = Workout_SampleRate(&buddy);
    printf("Sample rate: %d\n", int(sample_rate));

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, uint32_t(sample_rate));
    ma_result result = ma_decoder_init_file(kickfile, &config, &kickdecoder);
    if (result != MA_SUCCESS) {
        printf("Could not load file: %s\n", kickfile);
        return -2;
    }

    voices.Init(sample_rate);

    Workout_Start(&buddy);

    printf("Play note: p\n");
    printf("command: ");
    while (auto c = getchar()) {
        if (c == 'p') {
            printf("  playing note...\n");
            voices.NoteOff(60, 100);
            voices.NoteOn(60, 100);
        }
        //printf("\ncommand: ");
    }
    return 0;
}