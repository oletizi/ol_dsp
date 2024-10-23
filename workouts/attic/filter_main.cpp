//
// Created by Orion Letizi on 11/26/23.
//
#define MINIAUDIO_IMPLEMENTATION

#include "miniaudio.h"
#include "RtMidi.h"
#include "fxlib/Fx.h"
#include "synthlib/ol_synthlib.h"

#define CHANNEL_COUNT 2
using namespace ol::fx;
using namespace ol::synth;
//daisysp::Oscillator dosc;
//auto osc = new ol::synth::OscillatorSoundSource<1>(dosc);
//ol::synth::Filter *v1_f[] = {new ol::synth::SvfFilter()};
//auto v1_fe = ol::synth::DaisyAdsr();
//auto v1_ae = ol::synth::DaisyAdsr();
//auto v1_port = ol::synth::DaisyPortamento();
//auto voice = ol::synth::SynthVoice<1>(osc, v1_f, &v1_fe, &v1_ae, &v1_port);
//auto svf = daisysp::Svf();
//auto filt = ol::fx::FilterFx(svf);
//auto voice = ol::synth::SynthVoice<1>();
SynthVoice<1> voice;
FilterFx<1> filter;
daisysp::Oscillator osc;

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
    voice.NoteOff(note, velocity);
}

void handleCC(int channel, int control, int value) {
    std::cout << "CC: chan: " << channel << "; control: " << control << "; val: " << value << std::endl;
    //synth_control_panel.UpdateMidi(control, value);
    voice.UpdateMidiControl(control, value);
    filter.UpdateMidiControl(control, value);
}

void midi_callback(double deltatime, std::vector<unsigned char> *message, void *userData) {
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

void midi_error_callback(RtMidiError::Type type, const std::string &errorText, void *userData) {
    std::cout << "MIDI ERROR! " << errorText << std::endl;
}

void audio_callback(ma_device *device, void *pOutput, const void *pInput, ma_uint32 frameCount) {

    const uint32_t outputs = device->playback.channels;
    auto *out = (t_sample *) pOutput;
//    t_sample buf_in = 0;
//    t_sample buf_out = 0;
    for (int i = 0; i < frameCount; i++) {

        //t_sample filt_out = voice.Process();
        t_sample buf = 0;
        voice.Process(&buf);
        filter.Process(&buf, &buf);
        //out[i] = frame_buffer[i];
        //t_sample buf = osc.Process();

        out[i * outputs + 0] = buf;
        out[i * outputs + 1] = buf;
        //filter.Process(frame_buffer, &out[i]);
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
        if (strstr(port_name.c_str(), "IAC Driver") != nullptr) {
            std::cout << "Connecting to " << port_name << std::endl;
            midiin->openPort(i);
        }
    }

    midiin->setErrorCallback(midi_error_callback);
    midiin->setCallback(midi_callback);

    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.playback.format = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = CHANNEL_COUNT;               // Set to 0 to use the device's native channel count.
    config.sampleRate = 0;           // Set to 0 to use the device's native sample rate.
    config.dataCallback = audio_callback;   // This function will be called when miniaudio needs more data.
    //config.pUserData         = pMyCustomData;   // Can be accessed from the device object (device.pUserData).

    ma_device device;
    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }

    osc.Init(device.sampleRate);
    voice.Init(device.sampleRate);
    filter.Init(device.sampleRate);

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