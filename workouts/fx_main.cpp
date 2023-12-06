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

FilterFx delay_filter1;
FilterFx delay_filter2;

DelayFx delay1;
DelayFx delay2;

ReverbFx reverbSc;
ReverbFx reverbDattorro;

FilterFx filter1;
FilterFx filter2;

FxRack fxrack;

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
    ol::fx::FxRack_UpdateMidiControl(&fxrack, control, value);
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

    for (int i = 0; i < frameCount; i++) {

        t_sample voice_out = voice.Process();
        t_sample in_value = 0;
        for (int j = 0; j < input_channel_count; j++) {
            in_value += in[(i * input_channel_count) + j];
        }
        t_sample fx_in1 = (voice_out + in_value) / 2;
        t_sample fx_in2 = fx_in1;

        t_sample fx_out1 = 0;
        t_sample fx_out2 = 0;

        fxrack.Process(&fxrack, fx_in1, fx_in2, &fx_out1, &fx_out2);

        out[i * output_channel_count + 0] = fx_out1;
        out[i * output_channel_count + 1] = fx_out2;
    }
}


int main() {

    daisysp::Svf df1;
    daisysp::Svf df2;
    ol::fx::Filter_Svf_Config(&delay_filter1, &df1);
    ol::fx::Filter_Svf_Config(&delay_filter2, &df2);

    daisysp::DelayLine<t_sample, MAX_DELAY> delay_line1;
    daisysp::DelayLine<t_sample, MAX_DELAY> delay_line2;

    Delay_Config(&delay1, &delay_line1, &delay_filter1);
    Delay_Config(&delay2, &delay_line2, &delay_filter2);

    daisysp::ReverbSc vsc;
    ReverbSc_Config(&reverbSc, &vsc);

    sDattorroVerb *dverb = DattorroVerb_create();
    Dattorro_Config(&reverbDattorro, dverb);

    daisysp::Svf svf1;
    daisysp::Svf svf2;
    Filter_Svf_Config(&filter1, &svf1);
    Filter_Svf_Config(&filter2, &svf2);

    HyperTan transfer_function;
    SaturatorFx saturator1;
    Saturator_Config(&saturator1, &transfer_function);

    SaturatorFx saturator2;
    Saturator_Config(&saturator2, &transfer_function);

    SaturatorFx interstage_saturator;
    Saturator_Config(&interstage_saturator, &transfer_function);

    FxRack_Config(&fxrack, &delay1, &delay2, &reverbSc, &filter1, &filter2, &saturator1, &saturator2,
                  &interstage_saturator);

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
    fxrack.Init(&fxrack, device.sampleRate);

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