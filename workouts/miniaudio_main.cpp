#define MINIAUDIO_IMPLEMENTATION

#include <cstdio>
#include "miniaudio.h"

#define WAVETABLE_SIZE 256

// Data for the sine wave generation
struct Wave {
    float data[WAVETABLE_SIZE]{};
    int i = 0;
};


bool play_wave = false;
uint64_t callback_count = 0;
ma_uint32 max_frame_count = 0;


void fill_ramp(Wave &wave) {
    for (int i = 0; i < WAVETABLE_SIZE; i++) {
        wave.data[i] = (float) i / WAVETABLE_SIZE;
    }
}

void audio_callback(ma_device *device, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    callback_count++;
    if (frameCount > max_frame_count) {
        max_frame_count = frameCount;
    }

    auto *wave = (Wave *) device->pUserData;
    auto *output = (float *) pOutput;

    for (int i = 0; i < frameCount; i++) {
        if (play_wave) {
            auto sample = wave->data[wave->i];

            wave->i++;
            if (wave->i >= WAVETABLE_SIZE) {
                wave->i = 0;
            }

            for (int j=0; j<device->playback.channels; j++) {
                output[device->playback.channels * i + j] = sample;
            }

        }
    }

    (void) pInput;
}


int main() {
    printf("Exercising the miniaudio system...\n");
    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    ma_device audio_device;

    Wave wave{};

    fill_ramp(wave);

    config.playback.format = ma_format_f32;     // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = 0;               // Set to 0 to use the device's native channel count.
    config.sampleRate = 0;                      // Set to 0 to use the device's native sample rate.
    config.dataCallback = audio_callback;       // This function will be called when miniaudio needs more data.
    config.pUserData = &wave;

    printf("Initializing miniaudio device...\n");
    if (ma_device_init(nullptr, &config, &audio_device) != MA_SUCCESS) {
        printf("Miniaudio device init failed.\n");
        return -1;
    }
    printf("Done.\n");
    printf("Audio device info:\n");
    printf("  Sample rate: %d\n", audio_device.sampleRate);
    printf("  Channels   : %d\n", audio_device.playback.channels);

    printf("Starting miniaudio device...\n");
    if (ma_device_start(&audio_device) != MA_SUCCESS) {
        printf("Miniaudio device start failed.\n");
        return -1;
    }
    printf("Dene.\n");

    printf("Command list:\n");
    printf("  p: Play a wave.\n");
    printf("  i: Print info.\n");
    printf("command: ");
    while (auto c = getchar()) {
        if (c == 'q') {
            break;
        } else if (c == 'p') {
            play_wave = !play_wave;
            printf("Toggle wave playback: %d\n", play_wave);
        } else if (c == 'i') {
            printf("Info: \n");
            printf(" Wave:\n");
            for (int i=0; i<WAVETABLE_SIZE; i++) {
                printf("    %d: %f\n", i, wave.data[i]);
            }
            printf(" Callback count : %llu\n", callback_count);
            printf(" Max frame count: %d\n", max_frame_count);
            printf(" Sample rate    : %d\n", audio_device.sampleRate);
        }
    }

    return 0;
}