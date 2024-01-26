//
// Created by Orion Letizi on 12/3/23.
//
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "ol_synthlib.h"

using namespace ol::synth;

class MockVoice : public Voice {
public:
    MOCK_METHOD(void, Init, (t_sample sample_rate), (override));
    MOCK_METHOD(void, SetFrequency, (t_sample freq), (override));
    MOCK_METHOD(void, Update, (), (override));
    MOCK_METHOD(void, Process, (t_sample * frame_out), (override));
    MOCK_METHOD(void, UpdateMidiControl, (uint8_t control, uint8_t value), (override));
    MOCK_METHOD(void, UpdateHardwareControl, (uint8_t control, t_sample value), (override));
    MOCK_METHOD(void, UpdateConfig, (Voice::Config & config));
    MOCK_METHOD(void, GateOn, (), (override));
    MOCK_METHOD(void, GateOff, (), (override));
    MOCK_METHOD(bool, Gate, (), (override));
    MOCK_METHOD(void, NoteOn, (uint8_t midi_note, uint8_t velocity), (override));
    MOCK_METHOD(void, NoteOff, (uint8_t midi_note, uint8_t velocity), (override));
    MOCK_METHOD(uint8_t, Playing, (), (override));
};

template<int CHANNEL_COUNT>
class MockSoundSource : public SoundSource<CHANNEL_COUNT> {
public:
    MOCK_METHOD(InitStatus, Init, (t_sample sample_rate), (override));

    MOCK_METHOD(void, Process, (t_sample * frame), (override));

    MOCK_METHOD(void, GateOn, (), (override));

    MOCK_METHOD(void, GateOff, (), (override));

    MOCK_METHOD(void, SetFreq, (t_sample freq), (override));
};

class MockPortamento : public Portamento {
public:
    MOCK_METHOD(void, Init, (t_sample sample_rate, t_sample htime), (override));

    MOCK_METHOD(t_sample, Process, (t_sample in), (override));

    MOCK_METHOD(void, SetHtime, (t_sample htime), (override));

    MOCK_METHOD(t_sample, GetHtime, (), (override));
};

class MockAdsr : public Adsr {
public:
    MOCK_METHOD(void, Init, (t_sample sample_rate, int blockSize), (override));

    MOCK_METHOD(void, Retrigger, (bool hard), (override));

    MOCK_METHOD(t_sample, Process, (bool gate), (override));

    MOCK_METHOD(void, SetTime, (int seg, t_sample time), (override));

    MOCK_METHOD(void, SetAttackTime, (t_sample timeInS, t_sample shape), (override));

    MOCK_METHOD(void, SetDecayTime, (t_sample timeInS), (override));

    MOCK_METHOD(void, SetSustainLevel, (t_sample level), (override));

    MOCK_METHOD(void, SetReleaseTime, (t_sample timeInS), (override));

    MOCK_METHOD(uint8_t, GetCurrentSegment, (), (override));

    MOCK_METHOD(bool, IsRunning, (), (override));

};

class MockFilter : public Filter {
public:
    MOCK_METHOD(void, Init, (t_sample sample_rate), (override));

    MOCK_METHOD(void, SetFreq, (t_sample freq), (override));

    MOCK_METHOD(void, SetRes, (t_sample res), (override));

    MOCK_METHOD(void, SetDrive, (t_sample drive), (override));

    MOCK_METHOD(void, Process, (const t_sample *in), (override));

    MOCK_METHOD(void, Low, (t_sample *out), (override));

    MOCK_METHOD(void, High, (t_sample *out), (override));

    MOCK_METHOD(void, Band, (t_sample *out), (override));

    MOCK_METHOD(void, Notch, (t_sample *out), (override));

    MOCK_METHOD(void, Peak, (t_sample *out), (override));
};

using ::testing::AtLeast;
using ::testing::Exactly;
using ::testing::Return;
TEST(Synth, VoiceDefaultConstructor) {
    Voice *v = new SynthVoice();
    t_sample sample_rate = 48000;
    v->Init(sample_rate);
    EXPECT_EQ(v->Playing(), 0);
    EXPECT_FALSE(v->Gate());

    uint8_t midi_note = 60;
    uint8_t velocity = 100;
    v->NoteOn(midi_note, velocity);

    EXPECT_EQ(v->Playing(), midi_note);
    EXPECT_TRUE(v->Gate());

    v->NoteOff(midi_note, velocity);
    EXPECT_EQ(v->Playing(), 0);
    EXPECT_FALSE(v->Gate());

    t_sample frame_out = 1;
    v->Process(&frame_out);
    EXPECT_EQ(frame_out, 0);

    v->NoteOn(midi_note, velocity);
    v->Process(&frame_out);
    EXPECT_NE(frame_out, 0);
    EXPECT_NE(frame_out, 1);
    v->NoteOff(midi_note, velocity);

    // turn off master volume (amp env amount)
    Voice::Config config = {};
    config.amp_env_amount = 0;
    v->UpdateConfig(config);

    v->NoteOn(midi_note, velocity);
    v->Process(&frame_out);
    EXPECT_EQ(frame_out, 0);
    v->NoteOff(midi_note, velocity);

    // turn master volume back up
    config.amp_env_amount = 1;
    v->UpdateConfig(config);

    v->NoteOn(midi_note, velocity);
    v->Process(&frame_out);
    EXPECT_NE(frame_out, 0);
    v->NoteOff(midi_note, velocity);
}


TEST(Synth, Voice) {
    MockSoundSource<1> source;
    MockFilter filter;
    MockAdsr filter_envelope;
    MockAdsr amp_envelope;
    MockPortamento portamento;
    SynthVoice v(&source, &filter, &filter_envelope, &amp_envelope, &portamento);

    Voice::Config config = {};

    config.filter_cutoff = 0.1f;
    config.filter_resonance = 0.0001f;
    config.filter_drive = 0.00001f;

    config.filter_attack = 0.01f;
    config.filter_attack_shape = 0.02f;
    config.filter_decay = 0.03f;
    config.filter_sustain = 0.04f;
    config.filter_release = 0.05f;

    config.amp_attack = 0.003f;
    config.amp_attack_shape = 0.004f;
    config.amp_decay = 0.005f;
    config.amp_sustain = 0.006f;
    config.amp_release = 0.007f;

    config.portamento = 0.001f;

    //EXPECT_CALL(filter, SetFreq(config.filter_cutoff)).Times(Exactly(1));
    EXPECT_CALL(filter, SetRes(config.filter_resonance)).Times(Exactly(1));
    EXPECT_CALL(filter, SetDrive(config.filter_drive)).Times(Exactly(1));

    EXPECT_CALL(filter_envelope, SetAttackTime(config.filter_attack, config.filter_attack_shape)).Times(
            Exactly(1));
    EXPECT_CALL(filter_envelope, SetDecayTime(config.filter_decay)).Times(Exactly(1));
    EXPECT_CALL(filter_envelope, SetSustainLevel(config.filter_sustain)).Times(Exactly(1));
    EXPECT_CALL(filter_envelope, SetReleaseTime(config.filter_release)).Times(Exactly(1));

    EXPECT_CALL(amp_envelope, SetAttackTime(config.amp_attack, config.amp_attack_shape)).Times(Exactly(1));
    EXPECT_CALL(amp_envelope, SetDecayTime(config.amp_decay)).Times(Exactly(1));
    EXPECT_CALL(amp_envelope, SetSustainLevel(config.amp_sustain)).Times(Exactly(1));
    EXPECT_CALL(amp_envelope, SetReleaseTime(config.amp_release)).Times(Exactly(1));

    EXPECT_CALL(portamento, SetHtime(config.portamento)).Times(Exactly(1));

    // make sure the config values get updated
    v.UpdateConfig(config);

    auto sample_rate = 1;
    auto block_size = 1;
    EXPECT_CALL(filter_envelope, Init(sample_rate, block_size)).Times(Exactly(1));
    EXPECT_CALL(amp_envelope, Init(sample_rate, block_size)).Times(Exactly(1));
    EXPECT_CALL(portamento, Init(sample_rate, config.portamento)).Times(Exactly(1));
    // make sure Init calls Init on its members
    v.Init(sample_rate);

}

TEST(Synth, Polyvoice) {
    MockVoice v1;
    MockVoice v2;
    //    Voice *mock_voices[] = {&v1, &v2};
    std::vector<Voice *> voices = {&v1, &v2};
    auto p = Polyvoice(voices);

    EXPECT_CALL(v1, NoteOn(10, 1)).Times(AtLeast(1));
    EXPECT_CALL(v1, Playing())
            .WillOnce(Return(0))
            .WillOnce(Return(10));

    EXPECT_CALL(v2, NoteOn(11, 1)).Times(AtLeast(1));
    EXPECT_CALL(v2, Playing())
            .WillOnce(Return(0));

    p.NoteOn(10, 1);
    p.NoteOn(11, 1);
}