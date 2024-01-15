#include <cstdint>
#include "daisy_pod.h"
#include "daisysp.h"
#include "fxlib/Fx.h"

using namespace daisy;
using namespace daisysp;

daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> DSY_SDRAM_BSS delay_line1;
daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> DSY_SDRAM_BSS delay_line2;

ol::fx::DelayControlPanel delay_control_panel;
ol::perflib::Profile profile(1024, []()->uint64_t{
	return daisy::System::GetNow();
});
ol::fx::Delay delay1(&delay_control_panel, &delay_line1, &profile);
ol::fx::Delay delay2(&delay_control_panel, &delay_line2, &profile);
DaisyPod hw;
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	float balance = delay_control_panel.balance.Value();
	for (size_t i = 0; i < size; i++)
	{

		out[0][i] = (delay1.Process(in[0][i]) * balance) + (in[0][i] * (1-balance));
		out[1][i] = (delay2.Process(in[1][i]) * balance) + (in[1][i] * (1-balance));
	}
}

void handleMidiMessage(MidiEvent m) {
    if (m.type == daisy::ControlChange) {
        ControlChangeEvent p = m.AsControlChange();
        DaisySeed::PrintLine("Channel: %d, cc: %d; value: %d", p.channel, p.control_number, p.value);
        delay_control_panel.UpdateMidi(p.control_number, p.value);
    }
}

int main(void)
{

	int identifier = 1;
	hw.Init();

	DaisySeed::StartLog();
	DaisySeed::PrintLine("Hello, world!");
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	
    delay1.Init(hw.AudioSampleRate());
	delay2.Init(hw.AudioSampleRate());

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	hw.midi.StartReceive();
	uint16_t counter = 0;
	uint64_t t0 = daisy::System::GetNow();
	while (1)
	{

		hw.midi.Listen(); // ???: Can this be done outside control loop?
        // Handle MIDI Events
		
	    while (hw.midi.HasEvents()) {
            const MidiEvent m = hw.midi.PopEvent();
            handleMidiMessage(m);
        }

		hw.ProcessAllControls();
		daisy::System::Delay(1);
		float average_output = profile.AvgOut1Value();
		float average_delay_input = profile.AvgVal1Value();

		hw.led1.SetGreen(average_output);
		hw.led2.SetRed(average_delay_input);
		hw.UpdateLeds();
		//delay_control_panel.time.UpdateValueHardware(hw.knob1.Value());
		//delay_control_panel.feedback.UpdateValueHardware(hw.knob2.Value());

		uint64_t now = daisy::System::GetNow();
		if (now - t0 > 1000) 
		{
			t0 = now;
			counter = 0;
			// int factor = 100;
			// float max_val_1 = profile.MaxVal1Value();
			// DaisySeed::PrintLine( "  Max execution time    :  %d " , f2s32(profile.MaxExecutionTime() * factor) );
			// DaisySeed::PrintLine( "  Average execution time:  %d " , f2s32(profile.AverageExecutionTime() * factor));
			// DaisySeed::PrintLine( "  Max input value       :  %d " , f2s16(profile.MaxIn1Value()));
			// DaisySeed::PrintLine( "  Min input value       : %d " , f2s16(profile.MinIn1Value()));
			// DaisySeed::PrintLine( "  Max output value      :  %d " , f2s32( profile.MaxOut1Value() * factor ));
			// DaisySeed::PrintLine( "  Min output value      : %d " , f2s32(profile.MinOut1Value() * factor ));
			// DaisySeed::PrintLine( "  Max delay input value :  %d " , f2s32(max_val_1 * factor ));
			// DaisySeed::PrintLine( "    > 1?                :  %s ", max_val_1 > 1 ? "yes" : "no");
			// DaisySeed::PrintLine( "    < 1 ?               :  %s ", max_val_1 < 1 ? "yes" : "no");
			// DaisySeed::PrintLine( "    > 0.1               :  %s ", max_val_1 > 0.1f ? "yes" : "no");
			// DaisySeed::PrintLine( "    < 0.1               :  %s ", max_val_1 < 0.1 ? "yes" : "no");
			// DaisySeed::PrintLine( "    > 0.01              :  %s ", max_val_1 > 0.01f ? "yes" : "no");
			// DaisySeed::PrintLine( "    > 0.001              :  %s ", max_val_1 > 0.001 ? "yes" : "no");
			// DaisySeed::PrintLine( "  Min delay output value: %d " , f2s32(profile.MinVal1Value() * factor ));
			// DaisySeed::PrintLine( "  Avg abs dely in value :  %d " , f2s32(profile.AvgVal1Value() * factor ));
		}

		counter++;
	}
}
