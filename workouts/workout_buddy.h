//
// Created by Orion Letizi on 11/29/23.
//

#ifndef OL_DSP_WORKOUT_BUDDY_H
#define OL_DSP_WORKOUT_BUDDY_H

#define MINIAUDIO_IMPLEMENTATION

#include "miniaudio.h"
#include "RtMidi.h"
#include "corelib/ol_corelib.h"
#include "synthlib/ol_synthlib.h"

#define MAX_PATH_LENGTH 256
namespace ol::workout {

    class MaSampleSource : public ol::synth::SampleDataSource {
    public:
        explicit MaSampleSource(const char *sample_path, ma_decoder *decoder) :
                decoder_(decoder) {
            set_path(sample_path);
        }

        uint64_t GetChannelCount() override;

        ol::synth::InitStatus Init(t_sample sample_rate, const char * sample_path) override;

        void Seek(uint64_t) override;

        uint64_t Read(t_sample *frames_out) override;


    private:
        inline void set_path(const char *path) {
            std::snprintf(path_buffer, MAX_PATH_LENGTH, "%s", path);
        }

        char path_buffer[MAX_PATH_LENGTH] = {};
        ma_decoder *decoder_;
        t_sample sample_rate_ = 0;
    };

    enum InitStatus {
        Ok,
        MidiInitError,
        AudioInitError
    };

    struct workout_buddy {
        typedef
        void (*MidiNoteOnCallback)(workout_buddy *, uint8_t channel, uint8_t note, uint8_t velocity);

        typedef
        void (*MidiNoteOffCallback)(workout_buddy *, uint8_t channel, uint8_t note, uint8_t value);

        typedef
        void (*MidiControlChangeCallback)(workout_buddy *, uint8_t channel, uint8_t controller, uint8_t value);

        typedef
        void (*AudioCallback)(workout_buddy *, t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2);

        MidiNoteOnCallback HandleNoteOn = nullptr;
        MidiNoteOffCallback HandleNoteOff = nullptr;
        MidiControlChangeCallback HandleMidiControlChange = nullptr;
        AudioCallback Process = nullptr;

        ma_device *audio_device = nullptr;

        RtMidiIn *midi_in = nullptr;

        void *audio_data = nullptr;
    };


    t_sample Workout_SampleRate(workout_buddy *);

    void Workout_Start(workout_buddy *);

    InitStatus Workout_Init(workout_buddy *);

    void
    Workout_Config(workout_buddy *, RtMidiIn *, ma_device *, workout_buddy::MidiNoteOnCallback,
                   workout_buddy::MidiNoteOffCallback,
                   workout_buddy::MidiControlChangeCallback, workout_buddy::AudioCallback, void *audio_data = nullptr);
}


#endif //OL_DSP_WORKOUT_BUDDY_H
