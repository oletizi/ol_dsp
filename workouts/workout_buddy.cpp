//
// Created by Orion Letizi on 11/29/23.
//

#include "workout_buddy.h"

namespace ol::workout {

    void
    MaSampleSource_read (void *source, t_sample *frames_out, uint64_t frame_count,
                        uint64_t *frames_read) {
        auto s = static_cast<MaSampleSource *>(source);
        ma_decoder_read_pcm_frames(s->decoder, frames_out, frame_count, frames_read);
    }

    void MaSampleSource_seek(void *source, uint64_t frame_index) {
        auto s = static_cast<MaSampleSource *>(source);
        ma_decoder_seek_to_pcm_frame(s->decoder, frame_index);
    }

    void MaSampleSource_Config(MaSampleSource *s, ma_decoder *d) {
        s->decoder = d;
        s->Read = MaSampleSource_read;
        s->Seek = MaSampleSource_seek;
    }

    void
    Workout_rtMidiCallback([[maybe_unused]] double deltatime, std::vector<unsigned char> *message, void *userData) {
        auto *buddy = static_cast<workout_buddy *>(userData);
        unsigned int nBytes = message->size();
        if (nBytes > 0) {
            unsigned char status = message->at(0);
            int type = (int) (status >> 4);
            int channel = (int) (status & 0x0F);
            switch (type) {
                case 9:
                    std::cout << "NOTE ON!" << std::endl;
                    if (buddy->HandleNoteOn) {
                        buddy->HandleNoteOn(buddy, channel, message->at(1), message->at(2));
                    } else {
                        std::cout << "No Note On callback configured." << std::endl;
                    }
                    break;
                case 8:
                    std::cout << "NOTE OFF!" << std::endl;
                    if (buddy->HandleNoteOff) {
                        buddy->HandleNoteOff(buddy, channel, message->at(1), message->at(2));
                    } else {
                        std::cout << "No Note Off callback configured." << std::endl;
                    }
                    break;
                case 11:
                    std::cout << "MIDI CC!" << std::endl;
                    if (buddy->HandleMidiControlChange) {
                        buddy->HandleMidiControlChange(buddy, channel, message->at(1), message->at(2));
                    } else {
                        std::cout << "No CC callback configured." << std::endl;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    uint64_t counter = 0;

    void Workout_audioCallback(ma_device *device, void *pOutput, const void *pInput, ma_uint32 frameCount) {
        counter++;
        if (counter % 10000 == 0) {
            std::cout << "AUDIO CALLBACK!" << std::endl;
        }
        auto *buddy = static_cast<workout_buddy *>(device->pUserData);
        if (buddy->Process) {
            auto *in = (t_sample *) pInput;
            auto *out = (t_sample *) pOutput;
            auto output_channel_count = device->playback.channels;

            for (int i = 0; i < frameCount; i++) {
                t_sample out1 = 0;
                t_sample out2 = 0;
                buddy->Process(buddy, in[i], in[i], &out1, &out2);
                out[i * output_channel_count + 0] = out1;
                out[i * output_channel_count + 1] = out2;

            }
        }
        if (counter % 20000 == 0) {
            std::cout << "Workout audio callback!" << std::endl;
            counter = 0;
        }
    }


    InitStatus Workout_Init(workout_buddy *buddy) {
        // Initialize Midi
        auto midi_in = buddy->midi_in;
        uint16_t ports = midi_in->getPortCount();
        for (int i = 0; i < ports; i++) {

            std::cout << "Input port " << i << ": " << midi_in->getPortName(i) << std::endl;
            const std::string &port_name = midi_in->getPortName(i);
            // XXX: Fix the hard-coding
            if (strstr(port_name.c_str(), "Maschine") != nullptr) {
                std::cout << "Connecting to " << port_name << std::endl;
                midi_in->openPort(i);
            }
        }
        midi_in->setCallback(Workout_rtMidiCallback, buddy);

        ma_device_config config = ma_device_config_init(ma_device_type_duplex);
        config.playback.format = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
        config.playback.channels = 0;               // Set to 0 to use the device's native channel count.
        config.sampleRate = 0;           // Set to 0 to use the device's native sample rate.
        config.dataCallback = Workout_audioCallback;   // This function will be called when miniaudio needs more data.
        std::cout << "Set audio callback to: " << config.dataCallback << std::endl;
        config.pUserData = buddy;   // Can be accessed from the device object (device.pUserData).

        if (ma_device_init(nullptr, &config, buddy->audio_device) != MA_SUCCESS) {
            return InitStatus::AudioInitError;  // Failed to initialize the device.
        }
        std::cout << "Audio device initialized. Sample rate: " << buddy->audio_device->sampleRate << std::endl;
        return InitStatus::Ok;
    }

    void Workout_Config(workout_buddy *buddy, RtMidiIn *mi, ma_device *audio_device,
                        workout_buddy::MidiNoteOnCallback note_on_callback,
                        workout_buddy::MidiNoteOffCallback note_off_callback,
                        workout_buddy::MidiControlChangeCallback cc_callback,
                        workout_buddy::AudioCallback audio_callback,
                        void * audio_data) {
        buddy->midi_in = mi;
        buddy->audio_device = audio_device;
        buddy->HandleNoteOn = note_on_callback;
        buddy->HandleNoteOff = note_off_callback;
        buddy->HandleMidiControlChange = cc_callback;
        buddy->Process = audio_callback;
        buddy->audio_data = audio_data;
    }

    void Workout_Start(workout_buddy *buddy) {
        ma_device_start(buddy->audio_device);
    }

    t_sample Workout_SampleRate(workout_buddy *buddy) {
        return t_sample(buddy->audio_device->sampleRate);
    }

}