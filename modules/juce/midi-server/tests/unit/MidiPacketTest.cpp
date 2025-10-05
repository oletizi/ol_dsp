/**
 * MidiPacketTest.cpp
 *
 * Unit tests for MidiPacket
 * Tests: packet serialization/deserialization, flags, UUID hashing, validation
 *
 * Coverage Target: 80%+
 */

#include "network/core/MidiPacket.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace NetworkMidi;
using namespace testing;

class MidiPacketTest : public ::testing::Test {
protected:
    void SetUp() override {
        sourceNode = juce::Uuid();
        destNode = juce::Uuid();
        deviceId = 42;
        sequence = 100;
    }

    juce::Uuid sourceNode;
    juce::Uuid destNode;
    uint16_t deviceId;
    uint16_t sequence;
};

// Test default constructor
TEST_F(MidiPacketTest, DefaultConstructor) {
    MidiPacket packet;

    EXPECT_EQ(MidiPacket::MAGIC, packet.getMagic());
    EXPECT_EQ(MidiPacket::VERSION, packet.getVersion());
    EXPECT_EQ(0, packet.getFlags());
    EXPECT_EQ(0, packet.getSequence());
    EXPECT_EQ(0, packet.getDeviceId());
    EXPECT_TRUE(packet.getMidiData().empty());
    EXPECT_EQ(MidiPacket::PacketType::Data, packet.getPacketType());
}

// Test data packet creation
TEST_F(MidiPacketTest, CreateDataPacket) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};  // Note On

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    EXPECT_EQ(sourceNode, packet.getSourceNode());
    EXPECT_EQ(destNode, packet.getDestNode());
    EXPECT_EQ(deviceId, packet.getDeviceId());
    EXPECT_EQ(midiData, packet.getMidiData());
    EXPECT_EQ(sequence, packet.getSequence());
    EXPECT_EQ(MidiPacket::PacketType::Data, packet.getPacketType());
    EXPECT_GT(packet.getTimestampMicros(), 0u);
}

// Test SysEx auto-detection
TEST_F(MidiPacketTest, AutoDetectsSysEx) {
    std::vector<uint8_t> sysexData = {0xF0, 0x43, 0x12, 0x00, 0xF7};  // SysEx

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, sysexData, sequence
    );

    EXPECT_TRUE(packet.isSysEx());
    EXPECT_TRUE(packet.isReliable());  // SysEx requires reliable delivery
    EXPECT_TRUE(packet.hasFlag(MidiPacket::SysEx));
    EXPECT_TRUE(packet.hasFlag(MidiPacket::Reliable));
}

// Test non-SysEx data
TEST_F(MidiPacketTest, NonSysExDataDoesNotSetFlags) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    EXPECT_FALSE(packet.isSysEx());
    EXPECT_FALSE(packet.isReliable());
}

// Test heartbeat packet creation
TEST_F(MidiPacketTest, CreateHeartbeatPacket) {
    MidiPacket packet = MidiPacket::createHeartbeatPacket(
        sourceNode, destNode, sequence
    );

    EXPECT_EQ(sourceNode, packet.getSourceNode());
    EXPECT_EQ(destNode, packet.getDestNode());
    EXPECT_EQ(sequence, packet.getSequence());
    EXPECT_TRUE(packet.getMidiData().empty());
    EXPECT_EQ(MidiPacket::PacketType::Heartbeat, packet.getPacketType());
}

// Test ACK packet creation
TEST_F(MidiPacketTest, CreateAckPacket) {
    uint16_t ackSeq = 200;

    MidiPacket packet = MidiPacket::createAckPacket(
        sourceNode, destNode, ackSeq
    );

    EXPECT_EQ(sourceNode, packet.getSourceNode());
    EXPECT_EQ(destNode, packet.getDestNode());
    EXPECT_EQ(ackSeq, packet.getSequence());
    EXPECT_TRUE(packet.getMidiData().empty());
    EXPECT_EQ(MidiPacket::PacketType::Ack, packet.getPacketType());
}

// Test NACK packet creation
TEST_F(MidiPacketTest, CreateNackPacket) {
    uint16_t nackSeq = 300;

    MidiPacket packet = MidiPacket::createNackPacket(
        sourceNode, destNode, nackSeq
    );

    EXPECT_EQ(sourceNode, packet.getSourceNode());
    EXPECT_EQ(destNode, packet.getDestNode());
    EXPECT_EQ(nackSeq, packet.getSequence());
    EXPECT_TRUE(packet.getMidiData().empty());
    EXPECT_EQ(MidiPacket::PacketType::Nack, packet.getPacketType());
}

// Test packet serialization
TEST_F(MidiPacketTest, SerializesPacket) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    std::vector<uint8_t> serialized = packet.serialize();

    EXPECT_EQ(MidiPacket::HEADER_SIZE + midiData.size(), serialized.size());

    // Check magic bytes
    uint16_t magic = (static_cast<uint16_t>(serialized[0]) << 8) | serialized[1];
    EXPECT_EQ(MidiPacket::MAGIC, magic);

    // Check version
    EXPECT_EQ(MidiPacket::VERSION, serialized[2]);
}

// Test packet deserialization
TEST_F(MidiPacketTest, DeserializesPacket) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    MidiPacket original = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    std::vector<uint8_t> serialized = original.serialize();

    MidiPacket deserialized = MidiPacket::deserialize(serialized.data(), serialized.size());

    EXPECT_EQ(original.getSequence(), deserialized.getSequence());
    EXPECT_EQ(original.getDeviceId(), deserialized.getDeviceId());
    EXPECT_EQ(original.getMidiData(), deserialized.getMidiData());
    EXPECT_EQ(original.getFlags(), deserialized.getFlags());
}

// Test tryDeserialize success
TEST_F(MidiPacketTest, TryDeserializeSuccess) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    MidiPacket original = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    std::vector<uint8_t> serialized = original.serialize();

    MidiPacket deserialized;
    bool success = MidiPacket::tryDeserialize(serialized.data(), serialized.size(), deserialized);

    EXPECT_TRUE(success);
    EXPECT_EQ(original.getSequence(), deserialized.getSequence());
}

// Test tryDeserialize failure - invalid magic
TEST_F(MidiPacketTest, TryDeserializeFailsOnInvalidMagic) {
    std::vector<uint8_t> invalidData(MidiPacket::HEADER_SIZE);
    invalidData[0] = 0xFF;  // Wrong magic
    invalidData[1] = 0xFF;

    MidiPacket packet;
    bool success = MidiPacket::tryDeserialize(invalidData.data(), invalidData.size(), packet);

    EXPECT_FALSE(success);
}

// Test tryDeserialize failure - invalid version
TEST_F(MidiPacketTest, TryDeserializeFailsOnInvalidVersion) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    MidiPacket original = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    std::vector<uint8_t> serialized = original.serialize();

    // Corrupt version byte
    serialized[2] = 0xFF;

    MidiPacket deserialized;
    bool success = MidiPacket::tryDeserialize(serialized.data(), serialized.size(), deserialized);

    EXPECT_FALSE(success);
}

// Test tryDeserialize failure - insufficient data
TEST_F(MidiPacketTest, TryDeserializeFailsOnInsufficientData) {
    std::vector<uint8_t> shortData(5);  // Less than HEADER_SIZE

    MidiPacket packet;
    bool success = MidiPacket::tryDeserialize(shortData.data(), shortData.size(), packet);

    EXPECT_FALSE(success);
}

// Test serializeInto with sufficient buffer
TEST_F(MidiPacketTest, SerializeIntoSufficientBuffer) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    uint8_t buffer[256];
    size_t bytesWritten = 0;

    bool success = packet.serializeInto(buffer, sizeof(buffer), bytesWritten);

    EXPECT_TRUE(success);
    EXPECT_EQ(MidiPacket::HEADER_SIZE + midiData.size(), bytesWritten);
}

// Test serializeInto with insufficient buffer
TEST_F(MidiPacketTest, SerializeIntoInsufficientBuffer) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    uint8_t buffer[10];  // Too small
    size_t bytesWritten = 0;

    bool success = packet.serializeInto(buffer, sizeof(buffer), bytesWritten);

    EXPECT_FALSE(success);
    EXPECT_EQ(0u, bytesWritten);
}

// Test flag manipulation
TEST_F(MidiPacketTest, FlagManipulation) {
    MidiPacket packet;

    EXPECT_EQ(0, packet.getFlags());

    packet.addFlag(MidiPacket::Reliable);
    EXPECT_TRUE(packet.hasFlag(MidiPacket::Reliable));
    EXPECT_TRUE(packet.isReliable());

    packet.addFlag(MidiPacket::Fragment);
    EXPECT_TRUE(packet.hasFlag(MidiPacket::Fragment));
    EXPECT_TRUE(packet.isFragment());

    packet.removeFlag(MidiPacket::Reliable);
    EXPECT_FALSE(packet.hasFlag(MidiPacket::Reliable));
    EXPECT_TRUE(packet.hasFlag(MidiPacket::Fragment));
}

// Test timestamp update
TEST_F(MidiPacketTest, UpdateTimestamp) {
    MidiPacket packet;

    uint32_t timestamp1 = packet.getTimestampMicros();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    packet.updateTimestamp();
    uint32_t timestamp2 = packet.getTimestampMicros();

    EXPECT_GT(timestamp2, timestamp1);
}

// Test packet validation
TEST_F(MidiPacketTest, PacketValidation) {
    MidiPacket validPacket = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    EXPECT_TRUE(validPacket.isValid());
    EXPECT_TRUE(validPacket.verifyChecksum());
}

// Test getTotalSize
TEST_F(MidiPacketTest, GetTotalSize) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    EXPECT_EQ(MidiPacket::HEADER_SIZE + midiData.size(), packet.getTotalSize());
}

// Test empty MIDI data
TEST_F(MidiPacketTest, EmptyMidiData) {
    std::vector<uint8_t> emptyData;

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, emptyData, sequence
    );

    EXPECT_TRUE(packet.getMidiData().empty());
    EXPECT_EQ(MidiPacket::HEADER_SIZE, packet.getTotalSize());
}

// Test large MIDI data
TEST_F(MidiPacketTest, LargeMidiData) {
    std::vector<uint8_t> largeData(1000, 0x42);

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, largeData, sequence
    );

    EXPECT_EQ(1000u, packet.getMidiData().size());
    EXPECT_EQ(MidiPacket::HEADER_SIZE + 1000, packet.getTotalSize());
}

// Test sequence number wraparound
TEST_F(MidiPacketTest, SequenceNumberWraparound) {
    MidiPacket packet1 = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90}, 65535
    );

    MidiPacket packet2 = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90}, 0
    );

    EXPECT_EQ(65535, packet1.getSequence());
    EXPECT_EQ(0, packet2.getSequence());
}

// Test setMidiData auto-detection
TEST_F(MidiPacketTest, SetMidiDataAutoDetectsSysEx) {
    MidiPacket packet;

    std::vector<uint8_t> sysexData = {0xF0, 0x43, 0x12, 0x00, 0xF7};
    packet.setMidiData(sysexData);

    EXPECT_TRUE(packet.isSysEx());
    EXPECT_TRUE(packet.isReliable());
}

// Test setters
TEST_F(MidiPacketTest, Setters) {
    MidiPacket packet;

    packet.setSourceNode(sourceNode);
    packet.setDestNode(destNode);
    packet.setSequence(sequence);
    packet.setDeviceId(deviceId);
    packet.setFlags(MidiPacket::Reliable);
    packet.setPacketType(MidiPacket::PacketType::Heartbeat);

    EXPECT_EQ(sourceNode, packet.getSourceNode());
    EXPECT_EQ(destNode, packet.getDestNode());
    EXPECT_EQ(sequence, packet.getSequence());
    EXPECT_EQ(deviceId, packet.getDeviceId());
    EXPECT_EQ(MidiPacket::Reliable, packet.getFlags());
    EXPECT_EQ(MidiPacket::PacketType::Heartbeat, packet.getPacketType());
}

// Test round-trip serialization with SysEx
TEST_F(MidiPacketTest, RoundTripSerializationWithSysEx) {
    std::vector<uint8_t> sysexData = {0xF0, 0x43, 0x12, 0x00, 0xF7};

    MidiPacket original = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, sysexData, sequence
    );

    std::vector<uint8_t> serialized = original.serialize();
    MidiPacket deserialized = MidiPacket::deserialize(serialized.data(), serialized.size());

    EXPECT_EQ(original.isSysEx(), deserialized.isSysEx());
    EXPECT_EQ(original.isReliable(), deserialized.isReliable());
}

// Test header size constant
TEST_F(MidiPacketTest, HeaderSizeConstant) {
    EXPECT_EQ(20u, MidiPacket::HEADER_SIZE);
}

// Test magic constant
TEST_F(MidiPacketTest, MagicConstant) {
    EXPECT_EQ(0x4D49, MidiPacket::MAGIC);  // "MI"
}

// Test version constant
TEST_F(MidiPacketTest, VersionConstant) {
    EXPECT_EQ(0x01, MidiPacket::VERSION);
}
