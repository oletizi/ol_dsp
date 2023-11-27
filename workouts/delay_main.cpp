//
// Created by Orion Letizi on 11/26/23.
//
#define MINIAUDIO_IMPLEMENTATION

#include "miniaudio.h"
#include "RtMidi.h"
#include "fxlib/Fx.h"
#include "synthlib/ol_synthlib.h"

#define CHANNEL_COUNT 2

ol::synthlib::ControlPanel synth_control_panel;
ol::synthlib::Voice voice(&synth_control_panel);
daisysp::DelayLine<t_sample, MAX_DELAY> delay_line;
ol::fx::DelayFx delay;

//ol::fx::delay::Delay_Config(&delay, &delay_line);


int notes_on = 0;

void handleNoteOn(int channel, int note, int velocity) {
    std::cout << "NOTE ON: chan: " << channel << "; note: " << note << "; vel: " << velocity << std::endl;
    notes_on++;
    voice.NoteOn(note, velocity);
}

void handleNoteOff(int channel, int note, int velocity) {
    std::cout << "NOTE OFF: chan: " << channel << "; note: " << note << "; vel: " << velocity << std::endl;
    if (notes_on > 0) {
        notes_on--;
    }
    voice.NoteOff(note);
}

void handleCC(int channel, int control, int value) {
    std::cout << "CC: chan: " << channel << "; control: " << control << "; val: " << value << std::endl;
    synth_control_panel.UpdateMidi(control, value);
    ol::fx::Delay_UpdateMidiControl(&delay, control, value);
}

void midi_callback([[maybe_unused]] double deltatime, std::vector<unsigned char> *message, [[maybe_unused]] void *userData) {
    unsigned int nBytes = message->size();
    if (nBytes > 0) {
        unsigned char status = message->at(0);
        int type = (int) (status >> 4);
        int channel = (int) (status & 0x0F);
        std::cout << "TYPE: " << type << "; CHAN: " << channel << std::endl;
        switch (type) {
            case 9:
                //std::cout << "NOTE ON!" << std::endl;
                handleNoteOn(channel, message->at(1), message->at(2));
                break;
            case 8:
                //std::cout << "NOTE OFF!" << std::endl;
                handleNoteOff(channel, message->at(1), message->at(2));
                break;
            case 11:
                handleCC(channel, message->at(1), message->at(2));
                break;
            default:
                break;
        }

    }
}

void midi_error_callback([[maybe_unused]] RtMidiError::Type type, [[maybe_unused]] const std::string &errorText,
                         [[maybe_unused]] void *userData) {
    std::cout << "MIDI ERROR! " << errorText << std::endl;
}

void audio_callback([[maybe_unused]] ma_device *pDevice, void *pOutput, [[maybe_unused]] const void *pInput, ma_uint32 frameCount) {
    auto *out = (float *) pOutput;
    for (int i = 0; i < frameCount; i++) {
        t_sample voice_out = voice.Process();
        t_sample delay_out = 0;
        delay.Process(&delay, voice_out, &delay_out);

        t_sample out1 = voice_out + delay_out;
        t_sample out2 = voice_out + delay_out;

        out[i * CHANNEL_COUNT + 0] = out1;
        out[i * CHANNEL_COUNT + 1] = out2;
    }
}

int main() {

    RtMidiIn *midiin = nullptr;
    try {
        midiin = new RtMidiIn();
    } catch (RtMidiError &e) {
        e.printMessage();
        exit(1);
    }

    uint16_t ports = midiin->getPortCount();
    for (int i = 0; i < ports; i++) {

        std::cout << "Input port " << i << ": " << midiin->getPortName(i) << std::endl;
        const std::string &port_name = midiin->getPortName(i);
        if (strstr(port_name.c_str(), "Maschine") != nullptr) {
            std::cout << "Connecting to " << port_name << std::endl;
            midiin->openPort(i);
        }
    }

    midiin->setErrorCallback(midi_error_callback);
    midiin->setCallback(midi_callback);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = CHANNEL_COUNT;               // Set to 0 to use the device's native channel count.
    config.sampleRate = 0;           // Set to 0 to use the device's native sample rate.
    config.dataCallback = audio_callback;   // This function will be called when miniaudio needs more data.
    //config.pUserData         = pMyCustomData;   // Can be accessed from the device object (device.pUserData).

    ma_device device;
    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }

    voice.Init(device.sampleRate);
    ol::fx::Delay_Config(&delay, &delay_line);
    delay.Init(&delay, device.sampleRate);

    ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.

    // Do something here. Probably your program's main loop.
    std::cout << "Hi!" << std::endl;
    std::cout << "q: quit" << std::endl;
    while (auto c = getchar()) {
        if (c == 'q' || c == 'Q') {
            break;
        }
    }

    ma_device_uninit(&device);
    return 0;
}