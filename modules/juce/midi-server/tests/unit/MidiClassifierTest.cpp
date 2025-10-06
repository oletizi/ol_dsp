#include "../../network/transport/MidiClassifier.h"
#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>

using namespace NetworkMidi;

/**
 * Unit tests for MIDI message classification.
 *
 * Tests verify that messages are correctly classified as either:
 * - RealTime (UDP transport, low latency, best-effort)
 * - NonRealTime (TCP transport, reliable delivery)
 */
class MidiClassifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize JUCE if needed
    }

    void TearDown() override {
    }
};

// ============================================================================
// Channel Voice Messages (Real-Time)
// ============================================================================

TEST_F(MidiClassifierTest, ClassifyNoteOn)
{
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
    EXPECT_EQ(classifyMidiMessage(noteOn), MidiMessageClass::RealTime);
}

TEST_F(MidiClassifierTest, ClassifyNoteOff)
{
    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, 60, 0.5f);
    EXPECT_EQ(classifyMidiMessage(noteOff), MidiMessageClass::RealTime);
}

TEST_F(MidiClassifierTest, ClassifyControlChange)
{
    juce::MidiMessage cc = juce::MidiMessage::controllerEvent(1, 7, 127);
    EXPECT_EQ(classifyMidiMessage(cc), MidiMessageClass::RealTime);
}

TEST_F(MidiClassifierTest, ClassifyPitchBend)
{
    juce::MidiMessage pitchBend = juce::MidiMessage::pitchWheel(1, 8192);
    EXPECT_EQ(classifyMidiMessage(pitchBend), MidiMessageClass::RealTime);
}

TEST_F(MidiClassifierTest, ClassifyProgramChange)
{
    juce::MidiMessage programChange = juce::MidiMessage::programChange(1, 42);
    EXPECT_EQ(classifyMidiMessage(programChange), MidiMessageClass::RealTime);
}

TEST_F(MidiClassifierTest, ClassifyAftertouch)
{
    juce::MidiMessage aftertouch = juce::MidiMessage::aftertouchChange(1, 60, 100);
    EXPECT_EQ(classifyMidiMessage(aftertouch), MidiMessageClass::RealTime);
}

TEST_F(MidiClassifierTest, ClassifyChannelPressure)
{
    juce::MidiMessage channelPressure = juce::MidiMessage::channelPressureChange(1, 80);
    EXPECT_EQ(classifyMidiMessage(channelPressure), MidiMessageClass::RealTime);
}

// ============================================================================
// System Real-Time Messages (Real-Time)
// ============================================================================

TEST_F(MidiClassifierTest, ClassifyMidiClock)
{
    juce::MidiMessage clock = juce::MidiMessage::midiClock();
    EXPECT_EQ(classifyMidiMessage(clock), MidiMessageClass::RealTime);
    EXPECT_EQ(clock.getRawData()[0], 0xF8);
}

TEST_F(MidiClassifierTest, ClassifyMidiStart)
{
    juce::MidiMessage start = juce::MidiMessage::midiStart();
    EXPECT_EQ(classifyMidiMessage(start), MidiMessageClass::RealTime);
    EXPECT_EQ(start.getRawData()[0], 0xFA);
}

TEST_F(MidiClassifierTest, ClassifyMidiStop)
{
    juce::MidiMessage stop = juce::MidiMessage::midiStop();
    EXPECT_EQ(classifyMidiMessage(stop), MidiMessageClass::RealTime);
    EXPECT_EQ(stop.getRawData()[0], 0xFC);
}

TEST_F(MidiClassifierTest, ClassifyMidiContinue)
{
    juce::MidiMessage cont = juce::MidiMessage::midiContinue();
    EXPECT_EQ(classifyMidiMessage(cont), MidiMessageClass::RealTime);
    EXPECT_EQ(cont.getRawData()[0], 0xFB);
}

TEST_F(MidiClassifierTest, ClassifyActiveSensing)
{
    // Active Sensing is 0xFE
    uint8_t activeSensingData[] = {0xFE};
    juce::MidiMessage activeSensing(activeSensingData, 1);
    EXPECT_EQ(classifyMidiMessage(activeSensing), MidiMessageClass::RealTime);
}

TEST_F(MidiClassifierTest, ClassifySystemReset)
{
    // System Reset is 0xFF
    uint8_t systemResetData[] = {0xFF};
    juce::MidiMessage systemReset(systemResetData, 1);
    EXPECT_EQ(classifyMidiMessage(systemReset), MidiMessageClass::RealTime);
}

// ============================================================================
// System Exclusive Messages (Non-Real-Time)
// ============================================================================

TEST_F(MidiClassifierTest, ClassifyShortSysEx)
{
    uint8_t sysexData[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
    juce::MidiMessage sysex(sysexData, 5);
    EXPECT_EQ(classifyMidiMessage(sysex), MidiMessageClass::NonRealTime);
    EXPECT_TRUE(sysex.isSysEx());
}

TEST_F(MidiClassifierTest, ClassifyLargeSysEx)
{
    // Create a large SysEx (1KB)
    std::vector<uint8_t> sysexData(1024);
    sysexData[0] = 0xF0;  // SysEx start
    for (size_t i = 1; i < sysexData.size() - 1; ++i) {
        sysexData[i] = static_cast<uint8_t>(i % 128);
    }
    sysexData.back() = 0xF7;  // SysEx end

    juce::MidiMessage sysex(sysexData.data(), static_cast<int>(sysexData.size()));
    EXPECT_EQ(classifyMidiMessage(sysex), MidiMessageClass::NonRealTime);
    EXPECT_TRUE(sysex.isSysEx());
}

TEST_F(MidiClassifierTest, ClassifyUniversalSysEx)
{
    // Universal Real-Time SysEx (0xF0 0x7F ...)
    uint8_t universalSysex[] = {0xF0, 0x7F, 0x7F, 0x04, 0x01, 0xF7};
    juce::MidiMessage msg(universalSysex, 6);
    EXPECT_EQ(classifyMidiMessage(msg), MidiMessageClass::NonRealTime);
}

// ============================================================================
// System Common Messages (Non-Real-Time by default)
// ============================================================================

TEST_F(MidiClassifierTest, ClassifyMTCQuarterFrame)
{
    // MTC Quarter Frame is 0xF1
    uint8_t mtcData[] = {0xF1, 0x20};
    juce::MidiMessage mtc(mtcData, 2);
    EXPECT_EQ(classifyMidiMessage(mtc), MidiMessageClass::NonRealTime);
}

TEST_F(MidiClassifierTest, ClassifySongPosition)
{
    // Song Position Pointer is 0xF2
    uint8_t songPosData[] = {0xF2, 0x00, 0x00};
    juce::MidiMessage songPos(songPosData, 3);
    EXPECT_EQ(classifyMidiMessage(songPos), MidiMessageClass::NonRealTime);
}

TEST_F(MidiClassifierTest, ClassifySongSelect)
{
    // Song Select is 0xF3
    uint8_t songSelectData[] = {0xF3, 0x05};
    juce::MidiMessage songSelect(songSelectData, 2);
    EXPECT_EQ(classifyMidiMessage(songSelect), MidiMessageClass::NonRealTime);
}

TEST_F(MidiClassifierTest, ClassifyTuneRequest)
{
    // Tune Request is 0xF6
    uint8_t tuneRequestData[] = {0xF6};
    juce::MidiMessage tuneRequest(tuneRequestData, 1);
    EXPECT_EQ(classifyMidiMessage(tuneRequest), MidiMessageClass::NonRealTime);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(MidiClassifierTest, ClassifyEmptyMessage)
{
    // Edge case: empty message should default to NonRealTime for safety
    juce::MidiMessage empty;
    EXPECT_EQ(classifyMidiMessage(empty), MidiMessageClass::NonRealTime);
}

TEST_F(MidiClassifierTest, ClassifyAllChannelNumbers)
{
    // Test that classification works for all 16 MIDI channels
    for (int channel = 1; channel <= 16; ++channel) {
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(channel, 60, 0.8f);
        EXPECT_EQ(classifyMidiMessage(noteOn), MidiMessageClass::RealTime)
            << "Failed for channel " << channel;
    }
}

TEST_F(MidiClassifierTest, ClassifyAllNoteNumbers)
{
    // Test note range (0-127)
    for (int note = 0; note <= 127; ++note) {
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, note, 0.8f);
        EXPECT_EQ(classifyMidiMessage(noteOn), MidiMessageClass::RealTime)
            << "Failed for note " << note;
    }
}

TEST_F(MidiClassifierTest, ClassifyAllControllerNumbers)
{
    // Test CC range (0-127)
    for (int cc = 0; cc <= 127; ++cc) {
        juce::MidiMessage controller = juce::MidiMessage::controllerEvent(1, cc, 64);
        EXPECT_EQ(classifyMidiMessage(controller), MidiMessageClass::RealTime)
            << "Failed for CC " << cc;
    }
}

// ============================================================================
// Helper Function Tests
// ============================================================================

TEST_F(MidiClassifierTest, GetMessageClassName)
{
    EXPECT_EQ(getMidiMessageClassName(MidiMessageClass::RealTime), "RealTime");
    EXPECT_EQ(getMidiMessageClassName(MidiMessageClass::NonRealTime), "NonRealTime");
}

TEST_F(MidiClassifierTest, ExplainClassification)
{
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
    juce::String explanation = explainClassification(noteOn);
    EXPECT_TRUE(explanation.contains("Channel Voice"));
    EXPECT_TRUE(explanation.contains("RealTime"));

    uint8_t sysexData[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
    juce::MidiMessage sysex(sysexData, 5);
    explanation = explainClassification(sysex);
    EXPECT_TRUE(explanation.contains("System Exclusive"));
    EXPECT_TRUE(explanation.contains("NonRealTime"));

    juce::MidiMessage clock = juce::MidiMessage::midiClock();
    explanation = explainClassification(clock);
    EXPECT_TRUE(explanation.contains("System Real-Time"));
    EXPECT_TRUE(explanation.contains("RealTime"));
}

// ============================================================================
// Performance Tests (Benchmark)
// ============================================================================

TEST_F(MidiClassifierTest, ClassificationPerformance)
{
    // Measure classification speed
    const int iterations = 100000;
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);

    auto startTime = juce::Time::getHighResolutionTicks();

    for (int i = 0; i < iterations; ++i) {
        volatile auto result = classifyMidiMessage(noteOn);
        (void)result;  // Prevent optimization
    }

    auto endTime = juce::Time::getHighResolutionTicks();
    auto elapsedSeconds = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
    auto avgTimeNs = (elapsedSeconds / iterations) * 1e9;

    // Classification should take < 100ns per call
    EXPECT_LT(avgTimeNs, 100.0) << "Classification too slow: " << avgTimeNs << "ns";

    std::cout << "Average classification time: " << avgTimeNs << "ns" << std::endl;
}
