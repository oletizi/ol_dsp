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
    MOCK_METHOD(void, Update, (), (override));
    MOCK_METHOD(void, Process, (t_sample *frame_out), (override));
    MOCK_METHOD(void, UpdateMidiControl, (uint8_t control, uint8_t value), (override));
    MOCK_METHOD(void, NoteOn, (uint8_t midi_note, uint8_t velocity), (override));
    MOCK_METHOD(void, NoteOff, (uint8_t midi_note, uint8_t velocity), (override));
    MOCK_METHOD(uint8_t, Playing, (), (override));
};

using ::testing::AtLeast;
using ::testing::Return;
TEST(Synth, Polyvoice) {
    MockVoice v1 = MockVoice();
    MockVoice v2 = MockVoice();
    uint64_t voice_count = 2;
    Voice *mock_voices[] = {&v1, &v2};
    Polyvoice p = Polyvoice(mock_voices, voice_count);

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