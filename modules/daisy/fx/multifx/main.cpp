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


//std::queue<Voice*> DSY_SDRAM_BSS voice_pool;
//std::vector<Voice*> DSY_SDRAM_BSS voices;
const uint8_t voice_count = 5;
Polyvoice DSY_SDRAM_BSS multi;
daisysp::Svf DSY_SDRAM_BSS v1_f;
daisysp::Adsr v1_fe;
daisysp::Adsr v1_ae;
daisysp::Port v1_port;
Voice v1;

daisysp::Svf DSY_SDRAM_BSS v2_f;
daisysp::Adsr v2_fe;
daisysp::Adsr v2_ae;
daisysp::Port v2_port;
Voice v2;

daisysp::Svf DSY_SDRAM_BSS v3_f;
daisysp::Adsr v3_fe;
daisysp::Adsr v3_ae;
daisysp::Port v3_port;
Voice v3;

daisysp::Svf DSY_SDRAM_BSS v4_f;
daisysp::Adsr v4_fe;
daisysp::Adsr v4_ae;
daisysp::Port v4_port;
Voice v4;

daisysp::Svf DSY_SDRAM_BSS v5_f;
daisysp::Adsr v5_fe;
daisysp::Adsr v5_ae;
daisysp::Port v5_port;
Voice DSY_SDRAM_BSS v5;

daisysp::Svf delay_svf1;
daisysp::Svf delay_svf2;


daisysp::DelayLine<t_sample, MAX_DELAY> DSY_SDRAM_BSS delay_line1;
ol::fx::DelayFx delay1;
FilterFx delay_filter1;

daisysp::DelayLine<t_sample, MAX_DELAY> DSY_SDRAM_BSS delay_line2;
ol::fx::DelayFx delay2;
FilterFx delay_filter2;

daisysp::ReverbSc verb;
ol::fx::ReverbFx reverb;


daisysp::Svf svf1;
daisysp::Svf svf2;
ol::fx::FilterFx filter1;
ol::fx::FilterFx filter2;


HyperTan xfr_function;
SaturatorFx saturator1;
SaturatorFx saturator2;

SaturatorFx interstage_saturator;


ol::fx::FxRack fxrack;


static void callback(AudioHandle::InterleavingInputBuffer in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t size) {
    for (size_t i = 0; i < size; i += 2) {

        t_sample synth = multi.Process(&multi);
        t_sample out1 = synth; // this value gets overwritten by the fxrack, but it makes it easy to disable the fx rack with a single comment.
        t_sample out2 = synth;
        fxrack.Process(&fxrack, in[i] + synth, in[i + 1] + synth, &out1, &out2);

        out[i] = out1;
        out[i + 1] = out2;
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
            multi.NoteOn(&multi, n.note, n.velocity);
        }
        signalLed(LedSignal::NoteOn);
    }
    if (m.type == daisy::NoteOff) {
        NoteOffEvent n = m.AsNoteOff();
        if (m.channel == SYNTH_CHANNEL) {
            multi.NoteOff(&multi, n.note, n.velocity);
        }
        signalLed(LedSignal::NoteOff);
    }
    if (m.type == daisy::ControlChange) {
        ControlChangeEvent p = m.AsControlChange();
        if (m.channel == FX_CHANNEL) {
            FxRack_UpdateMidiControl(&fxrack, p.control_number, p.value);
        }
        if (m.channel == SYNTH_CHANNEL) {
            multi.UpdateMidiControl(&multi, p.control_number, p.value);
        }
        signalLed(LedSignal::Control);
    }

}


static Page reverb_page = {
        "Reverb",
        &reverb_page,
        &reverb_page,
        [](t_sample value) {
            Reverb_UpdateHardwareControl(&reverb, CC_REVERB_TIME, value);
            DaisySeed::PrintLine("Updated reverb time: %d", uint16_t(reverb.decay_time * 1000));
        },
        [](t_sample value) {
            Reverb_UpdateHardwareControl(&reverb, CC_REVERB_CUTOFF, value);
            DaisySeed::PrintLine("Updated reverb cutoff: %d", uint16_t(reverb.cutoff * 1000));
        }
};

static Page delay_page = {
        "Delay",
        &delay_page,
        &reverb_page,
        [](t_sample value) {
            Delay_UpdateHardwareControl(&delay1, CC_DELAY_TIME, value);
            Delay_UpdateHardwareControl(&delay2, CC_DELAY_TIME, value);
            DaisySeed::PrintLine("Updated delay time: %d", uint16_t(delay1.time * 1000));
        },
        [](t_sample value) {
            Delay_UpdateHardwareControl(&delay1, CC_DELAY_FEEDBACK, value);
            Delay_UpdateHardwareControl(&delay2, CC_DELAY_FEEDBACK, value);
            DaisySeed::PrintLine("Update delay feedback: %d", uint16_t(delay1.filter->cutoff * 1000));
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
    Voice_Config(&v1, &v1_f, &v1_fe, &v1_ae, &v1_port);
    Voice_Config(&v2, &v2_f, &v2_fe, &v2_ae, &v2_port);
    Voice_Config(&v3, &v3_f, &v3_fe, &v3_ae, &v3_port);
    Voice_Config(&v4, &v4_f, &v4_fe, &v4_ae, &v4_port);
    Voice_Config(&v5, &v5_f, &v5_fe, &v5_ae, &v5_port);

    Voice *voices[] = {&v1, &v2, &v3, &v4, &v5};
    Polyvoice_Config(&multi, voices, voice_count);

    multi.Init(&multi, sample_rate);


    // Config delay filters
    Filter_Svf_Config(&delay_filter1, &delay_svf1);
    Filter_Svf_Config(&delay_filter2, &delay_svf2);

    // Config delays
    Delay_Config(&delay1, &delay_line1, &delay_filter1);
    Delay_Config(&delay2, &delay_line2, &delay_filter2);

    // Config reverb
    ReverbSc_Config(&reverb, &verb);
    reverb.PrintLine = [](const char *msg) { DaisySeed::PrintLine(msg); };

    // Config output filters
    Filter_Svf_Config(&filter1, &svf1);
    Filter_Svf_Config(&filter2, &svf2);

    // Config output saturators (probably only need one of these, but one of the implementations
    // has history, so...)
    Saturator_Config(&saturator1, &xfr_function);
    Saturator_Config(&saturator2, &xfr_function);

    // Config interstage saturator
    Saturator_Config(&interstage_saturator, &xfr_function);

    // Config and init fx rack
    FxRack_Config(&fxrack, &delay1, &delay2, &reverb, &filter1, &filter2, &saturator1, &saturator2,
                  &interstage_saturator);

    fxrack.Init(&fxrack, sample_rate);

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