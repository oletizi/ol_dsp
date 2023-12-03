//
// Created by Orion Letizi on 12/3/23.
//
#include "gtest/gtest.h"
#include "ol_synthlib.h"

int v1_note_on_calls = 0;
int v1_note_off_calls = 0;

int v2_note_on_calls = 0;
int v2_note_off_calls = 0;

using namespace ol::synth;
TEST(Synth, Multi) {
    Multivoice m;

    Voice v1 = Voice();
    v1.NoteOn = [](Voice *v, uint8_t note, uint8_t velocity) {
        v1_note_on_calls++;
        v->playing = note;
    };
    v1.NoteOff = [](Voice *v, uint8_t note, uint8_t velocity) {
        v1_note_off_calls++;
        v->playing = 0;
    };

    Voice v2 = Voice();
    v2.NoteOn = [](Voice *v, uint8_t note, uint8_t velocity) {
        v2_note_on_calls++;
        v->playing = note;
    };
    v2.NoteOff = [](Voice *v, uint8_t note, uint8_t velocity) {
        v2_note_off_calls++;
        v->playing = 0;
    };

    Voice *voices[] = {&v1, &v2};
    Multivoice_Config(&m, voices, 2);

    m.NoteOn(&m, 10, 1);
    m.NoteOn(&m, 11, 1);

    EXPECT_EQ(v1_note_on_calls, 1);
    EXPECT_EQ(v2_note_on_calls, 1);
    EXPECT_TRUE(v1.playing);
    EXPECT_TRUE(v2.playing);

    m.NoteOff(&m, 1, 1);
    m.NoteOff(&m, 1, 1);
    EXPECT_EQ(v1_note_off_calls, 0);
    EXPECT_EQ(v2_note_off_calls, 0);
    EXPECT_TRUE(v1.playing);
    EXPECT_TRUE(v1.playing);

    m.NoteOff(&m, 10, 1);
    m.NoteOff(&m, 11, 1);
    EXPECT_EQ(v1_note_off_calls, 1);
    EXPECT_EQ(v2_note_off_calls, 1);
    EXPECT_FALSE(v1.playing);
    EXPECT_FALSE(v1.playing);
}