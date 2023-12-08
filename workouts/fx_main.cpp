#include <iostream>

#define MINIAUDIO_IMPLEMENTATION

#include "miniaudio.h"
#include "RtMidi.h"
#include "verb.h"
#include "verb_structs.h"
#include "daisy/ui/ui.h"
#include "fxlib/Fx.h"
#include "synthlib/ol_synthlib.h"

using namespace ol::fx;
using namespace ol::synth;

auto osc = OscillatorSoundSource(daisysp::Oscillator());
auto vf = daisysp::Svf();
auto vfe = daisysp::Adsr();
auto vae = daisysp::Adsr();
auto vport = daisysp::Port();
auto voice = SynthVoice(osc, vf, vfe, vae, vport);

daisysp::Svf df1;
auto delay_filter1 = FilterFx(df1, 0);
daisysp::Svf df2;
auto delay_filter2 = FilterFx(df2, 1);

daisysp::DelayLine<t_sample, MAX_DELAY> delay_line1;
daisysp::DelayLine<t_sample, MAX_DELAY> delay_line2;
auto delay1 = DelayFx(delay_line1, delay_filter1, 0);
auto delay2 = DelayFx(delay_line2, delay_filter2, 1);
daisysp::ReverbSc verb;
auto reverb = ReverbFx(verb);

daisysp::Svf svf1;
daisysp::Svf svf2;
auto filter1 = FilterFx(svf1, 0);
auto filter2 = FilterFx(svf2, 1);

auto sat1 = SaturatorFx(0);
auto sat2 = SaturatorFx(1);
auto sat3 = SaturatorFx();
auto fxrack = FxRack(delay1, delay2, reverb, filter1, filter2, sat1, sat2, sat3);

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
    //ol::fx::FxRack_UpdateMidiControl(&fxrack, control, value);
    fxrack.UpdateMidiControl(control, value);
}

void
midi_callback([[maybe_unused]] double deltatime, std::vector<unsigned char> *message, [[maybe_unused]] void *userData) {
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

void audio_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    auto *in = (float *) pInput;
    auto *out = (float *) pOutput;

    auto input_channel_count = pDevice->capture.channels;
    auto output_channel_count = pDevice->playback.channels;

    t_sample in_buffer[] = {0, 0};
    t_sample out_buffer[] = {0, 0};
    for (int i = 0; i < frameCount; i++) {

        t_sample voice_out = 0;
        voice.Process(&voice_out);
        t_sample in_value = 0;

        in_buffer[0] = in[i * input_channel_count] + voice_out;
        in_buffer[1] = in[i * input_channel_count + 1] + voice_out;
//        t_sample *fx_out1 = &out[i];
//        t_sample *fx_out2 = &out[i+1];

        //fxrack.Process(&fxrack, fx_in1, fx_in2, &fx_out1, &fx_out2);
        fxrack.Process(in_buffer, out_buffer);

        out[i * output_channel_count + 0] = out_buffer[0];
        out[i * output_channel_count + 1] = out_buffer[1];
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

    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.playback.format = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = 0;               // Set to 0 to use the device's native channel count.
    config.sampleRate = 0;           // Set to 0 to use the device's native sample rate.
    config.dataCallback = audio_callback;   // This function will be called when miniaudio needs more data.
    //config.pUserData         = pMyCustomData;   // Can be accessed from the device object (device.pUserData).

    ma_device device;
    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }

    voice.Init(device.sampleRate);
    fxrack.Init(device.sampleRate);

    ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.

    std::cout << "Send me some MIDI!" << std::endl;
    std::cout << "t: play test sound" << std::endl;
    std::cout << "q: quit" << std::endl;
    while (auto c = getchar()) {
        if (c == 'q' || c == 'Q') {
            break;
        }
    }
    return 0;
}