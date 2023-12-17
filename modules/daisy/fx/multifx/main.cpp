#ifdef DAISY_NATIVE

#include "daisy/daisy_dummy.h"

#else

#include "daisy.h"
#include "daisy_pod.h"

#endif

#include "fxlib/Fx.h"
#include "synthlib/ol_synthlib.h"
#include "daisy/ui/ui.h"

#define AUDIO_BLOCK_SIZE 4
#define SYNTH_CHANNEL 0
#define FX_CHANNEL 1
#define CHANNEL_COUNT 2
#define VOICE_COUNT 5

using namespace daisy;
using namespace ol::fx;
using namespace ol::synth;
static DaisyPod hw;

uint64_t led2_red_timestamp;
t_sample led2_red = 0;

uint64_t led2_green_timestamp;
t_sample led2_green = 0;

uint64_t led2_blue_timestamp;
t_sample led2_blue = 0;

//daisysp::Oscillator dosc1;
//auto osc1 = new OscillatorSoundSource<1>(&dosc1);
//ol::synth::Filter DSY_SDRAM_BSS *v1_f[] = {new ol::synth::SvfFilter()};
//ol::synth::DaisyAdsr v1_fe;
//ol::synth::DaisyAdsr v1_ae;
//ol::synth::DaisyPortamento v1_port;
//auto v1 = new SynthVoice<1>(osc1, v1_f, &v1_fe, &v1_ae, &v1_port);
//
//daisysp::Oscillator dosc2;
//auto osc2 = new OscillatorSoundSource<1>(&dosc2);
//ol::synth::Filter DSY_SDRAM_BSS *v2_f[] = {new ol::synth::SvfFilter()};
//ol::synth::DaisyAdsr v2_fe;
//ol::synth::DaisyAdsr v2_ae;
//ol::synth::DaisyPortamento v2_port;
//auto v2 = new SynthVoice<1>(osc2, v2_f, &v2_fe, &v2_ae, &v2_port);
//
//daisysp::Oscillator dosc3;
//auto osc3 = new OscillatorSoundSource<1>(&dosc3);
//ol::synth::Filter DSY_SDRAM_BSS *v3_f[] = {new ol::synth::SvfFilter()};
//ol::synth::DaisyAdsr v3_fe;
//ol::synth::DaisyAdsr v3_ae;
//ol::synth::DaisyPortamento v3_port;
//auto v3 = new SynthVoice<1>(osc3, v3_f, &v3_fe, &v3_ae, &v3_port);
//
//daisysp::Oscillator dosc4;
//auto osc4 = new OscillatorSoundSource<1>(&dosc4);
//ol::synth::Filter DSY_SDRAM_BSS *v4_f[] = {new ol::synth::SvfFilter()};
//ol::synth::DaisyAdsr v4_fe;
//ol::synth::DaisyAdsr v4_ae;
//ol::synth::DaisyPortamento v4_port;
//auto v4 = new SynthVoice<1>(osc4, v4_f, &v4_fe, &v4_ae, &v4_port);
//
//daisysp::Oscillator dosc5;
//auto osc5 = new OscillatorSoundSource<1>(&dosc5);
//ol::synth::Filter DSY_SDRAM_BSS *v5_f[] = {new ol::synth::SvfFilter()};
//ol::synth::DaisyAdsr v5_fe;
//ol::synth::DaisyAdsr v5_ae;
//ol::synth::DaisyPortamento v5_port;
//auto v5 = new SynthVoice<1>(osc5, v5_f, &v5_fe, &v5_ae, &v5_port);
//
//Voice *voices[VOICE_COUNT] = {v1, v2, v3, v4, v5};
//auto poly = Polyvoice<CHANNEL_COUNT, VOICE_COUNT>(voices);
auto poly = Polyvoice<CHANNEL_COUNT, VOICE_COUNT>();

//auto delay_filter1 = FilterFx<CHANNEL_COUNT>();
//daisysp::DelayLine<t_sample, MAX_DELAY> DSY_SDRAM_BSS delay_line1;
//daisysp::DelayLine<t_sample, MAX_DELAY> DSY_SDRAM_BSS delay_line2;
//daisysp::DelayLine<t_sample, MAX_DELAY> *delay_lines[CHANNEL_COUNT];                                    m
//auto delay1 = ol::fx::DelayFx<CHANNEL_COUNT>(delay_line1, delay_filter1);
//auto delay2 = DelayFx(delay_line2, delay_filter2);
//ol::fx::DelayFx<CHANNEL_COUNT> delay1(delay_lines);


//daisysp::ReverbSc verb;
//auto reverb = ReverbFx(verb);
//
//
//daisysp::Svf svf1;
//daisysp::Svf svf2;
//auto filter1 = FilterFx(svf1);
//auto filter2 = FilterFx(svf2);
//
//SaturatorFx saturator1;
//SaturatorFx saturator2;
//SaturatorFx interstage_saturator;

//auto fxrack = FxRack(delay1, delay2, reverb, filter1, filter2, saturator1, saturator2, interstage_saturator, 2);
FxRack<CHANNEL_COUNT> DSY_SDRAM_BSS fxrack = FxRack<CHANNEL_COUNT>();

t_sample process_buffer[CHANNEL_COUNT]{};

static void callback(AudioHandle::InterleavingInputBuffer in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t size) {
    for (size_t i = 0; i < size; i += 2) {
        //auto frame_out = {out[i], out[i+1]};
        poly.Process(process_buffer);
        fxrack.Process(process_buffer, &out[i]);

        //fxrack.Process(process_buffer, out[i]);
        //fxrack.Process(in[i] + process_buffer[0], in[i + 1] + process_buffer[1], out);
        //fxrack.Process(process_buffer, &out[i]);

    }
}

enum LedSignal {
    NoteOn,
    NoteOff,
    Control
};

static void signalLed(LedSignal type) {
    t_sample val = 0.9;
    uint32_t timestamp = daisy::System::GetNow();
    switch (type) {
        case LedSignal::NoteOn:
            led2_green = val;
            led2_green_timestamp = timestamp;
            break;
        case LedSignal::NoteOff:
            led2_red = val;
            led2_red_timestamp = timestamp;
            break;
        case LedSignal::Control:
            led2_blue = val;
            led2_blue_timestamp = timestamp;
        default:
            break;
    }
}

static void handleSignalLed() {
    if (led2_red + led2_green + led2_blue > 0) {
        uint32_t gate_time = 50;
        uint32_t now = daisy::System::GetNow();
        if (now - led2_red_timestamp > gate_time) {
            led2_red = 0;
        }
        if (now - led2_green_timestamp > gate_time) {
            led2_green = 0;
        }
        if (now - led2_blue_timestamp > gate_time) {
            led2_blue = 0;
        }

        hw.led2.SetRed(led2_red);
        hw.led2.SetGreen(led2_green);
        hw.led2.SetBlue(led2_blue);
        hw.UpdateLeds();
    }
}

static void handleMidiMessage(MidiEvent m) {
    DaisySeed::PrintLine("Midi event; channel: %d", uint32_t(m.channel));

    if (m.type == daisy::NoteOn) {
        auto n = m.AsNoteOn();
        if (m.channel == SYNTH_CHANNEL) {
            poly.NoteOn(n.note, n.velocity);
        }
        signalLed(LedSignal::NoteOn);
    }
    if (m.type == daisy::NoteOff) {
        NoteOffEvent n = m.AsNoteOff();
        if (m.channel == SYNTH_CHANNEL) {
            poly.NoteOff(n.note, n.velocity);
        }
        signalLed(LedSignal::NoteOff);
    }
    if (m.type == daisy::ControlChange) {
        ControlChangeEvent p = m.AsControlChange();
        if (m.channel == FX_CHANNEL) {
            //FxRack_UpdateMidiControl(&fxrack, p.control_number, p.value);
            fxrack.UpdateMidiControl(p.control_number, p.value);
        }
        if (m.channel == SYNTH_CHANNEL) {
            poly.UpdateMidiControl(m.channel, p.control_number, p.value);
        }
        signalLed(LedSignal::Control);
    }

}


static Page reverb_page = {
        "Reverb",
        &reverb_page,
        &reverb_page,
        [](t_sample value) {
            //Reverb_UpdateHardwareControl(&reverb, CC_REVERB_TIME, value);
            fxrack.UpdateMidiControl(CC_REVERB_TIME, value);

            //DaisySeed::PrintLine("Updated reverb time: %d", uint16_t(reverb.decay_time * 1000));
        },
        [](t_sample value) {
            //Reverb_UpdateHardwareControl(&reverb, CC_REVERB_CUTOFF, value);
            fxrack.UpdateMidiControl(CC_REVERB_CUTOFF, value);
            //DaisySeed::PrintLine("Updated reverb cutoff: %d", uint16_t(reverb.cutoff * 1000));
        }
};

static Page delay_page = {
        "Delay",
        &delay_page,
        &reverb_page,
        [](t_sample value) {
            //Delay_UpdateHardwareControl(&delay1, CC_DELAY_TIME, value);
            //Delay_UpdateHardwareControl(&delay2, CC_DELAY_TIME, value);
            fxrack.UpdateMidiControl(CC_DELAY_TIME, value);
            //DaisySeed::PrintLine("Updated delay time: %d", uint16_t(delay1.time * 1000));
        },
        [](t_sample value) {
            //Delay_UpdateHardwareControl(&delay1, CC_DELAY_FEEDBACK, value);
            //Delay_UpdateHardwareControl(&delay2, CC_DELAY_FEEDBACK, value);
            fxrack.UpdateMidiControl(CC_DELAY_FEEDBACK, value);
            //DaisySeed::PrintLine("Update delay feedback: %d", uint16_t(delay1.filter->cutoff * 1000));
        }
};


static Page *current_page = &reverb_page;


void initPages() {
    delay_page.next = &reverb_page;
    delay_page.prev = &delay_page;

    reverb_page.next = &delay_page;
    reverb_page.prev = &delay_page;
}


static t_sample knob1_value = 0;
static t_sample knob2_value = 0;

static bool analog_value_changed(t_sample previous, t_sample updated) {
    return (updated < (previous - 0.01)) || (updated > (previous + 0.01));
}

static void handleControls() {
    hw.ProcessAllControls();
    if (hw.button1.RisingEdge()) {
        DaisySeed::PrintLine("Button 1 rising edge!");
        current_page = current_page->prev;
        DaisySeed::PrintLine("New page: %s", current_page->name);
    } else if (hw.button2.RisingEdge()) {
        DaisySeed::PrintLine("Button 2 rising edge!");
        current_page = current_page->next;
        DaisySeed::PrintLine("New page: %s", current_page->name);
    }

    hw.knob1.Process();
    t_sample k1_val = hw.knob1.Value();
    if (analog_value_changed(knob1_value, k1_val)) {
        DaisySeed::PrintLine("Updating based on knob 1 val: %d; prev: %d", uint16_t(k1_val * 1000),
                             uint16_t(knob1_value * 1000));
        knob1_value = k1_val;
        current_page->UpdateKnob1(knob1_value);
    }

    hw.knob2.Process();
    t_sample k2_val = hw.knob2.Value();

    if (analog_value_changed(knob2_value, k2_val)) {
        DaisySeed::PrintLine("Updating based on knob 2 val: %d; prev: %d", uint16_t(k2_val * 1000),
                             uint16_t(knob2_value * 1000));
        knob2_value = k2_val;
        current_page->UpdateKnob2(knob2_value);
    }
}

template<typename... V>
void println(const char *msg, V... args) {
    DaisySeed::PrintLine(msg, args...);
}

void println(const char *msg) {
    DaisySeed::PrintLine(msg);
}

int main() {
    float sample_rate;

    hw.Init();
    DaisySeed::StartLog(false);
    hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
    sample_rate = hw.AudioSampleRate();
    hw.StartAdc();


    // Config and init synth voices
    poly.Init(sample_rate);
    fxrack.Init(sample_rate);

    initPages();

    hw.StartAudio(callback);
    hw.midi.StartReceive();

    long count = 0;
    bool led_state = 0;
    while (true) {
        // Control loop
        System::Delay(1);
        hw.midi.Listen(); // ???: Can this be done outside control loop?
        // Handle MIDI Events
        while (hw.midi.HasEvents()) {
            const MidiEvent m = hw.midi.PopEvent();
            handleMidiMessage(m);
        }

        handleSignalLed();
        handleControls();

        if (count % 500 == 0) {
            hw.led1.SetBlue(led_state);
            hw.UpdateLeds();
            led_state = !led_state;
        }
        if (count % 1000 == 0) {
            count = 0;
        }
        count++;
    }

}