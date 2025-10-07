/**
 * MidiPacketTest.cpp
 *
 * Unit tests for MidiPacket
 * Tests: packet serialization/deserialization, flags, UUID hashing, validation
 * Phase 4: Forwarding context serialization/deserialization
 *
 * Coverage Target: 80%+
 */

#include "network/core/MidiPacket.h"
#include "network/routing/UuidRegistry.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace NetworkMidi;
using namespace testing;

// Mock UuidRegistry for Phase 4 testing
class MockUuidRegistry : public UuidRegistry {
public:
    std::optional<juce::Uuid> lookupFromHash(uint32_t hash) const override {
        auto it = hashToUuid.find(hash);
        if (it != hashToUuid.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void registerNode(const juce::Uuid& uuid) override {
        uint32_t hash = MidiPacket::hashUuid(uuid);
        hashToUuid[hash] = uuid;
    }

private:
    mutable std::map<uint32_t, juce::Uuid> hashToUuid;
};

class MidiPacketTest : public ::testing::Test {
protected:
    void SetUp() override {
        sourceNode = juce::Uuid();
        destNode = juce::Uuid();
        deviceId = 42;
        sequence = 100;

        // Setup mock registry for Phase 4 tests
        mockRegistry = std::make_unique<MockUuidRegistry>();
        mockRegistry->registerNode(sourceNode);
        mockRegistry->registerNode(destNode);
    }

    juce::Uuid sourceNode;
    juce::Uuid destNode;
    uint16_t deviceId;
    uint16_t sequence;
    std::unique_ptr<MockUuidRegistry> mockRegistry;
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

//=============================================================================
// Phase 4: Forwarding Context Tests
//=============================================================================

// Test: Empty context (hopCount=0, no devices)
TEST_F(MidiPacketTest, Phase4_SerializeEmptyContext) {
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    ForwardingContext ctx;
    ctx.hopCount = 0;
    // visitedDevices is empty

    packet.setForwardingContext(ctx);

    EXPECT_TRUE(packet.hasForwardingContext());
    EXPECT_TRUE(packet.hasFlag(MidiPacket::HasContext));

    // Context size: Type(1) + Length(1) + HopCount(1) + DeviceCount(1) = 4 bytes
    size_t expectedSize = MidiPacket::HEADER_SIZE + 3 + 4; // header + MIDI + context
    EXPECT_EQ(expectedSize, packet.getTotalSize());
}

// Test: Context with 1 device
TEST_F(MidiPacketTest, Phase4_SerializeContextWithOneDevice) {
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    ForwardingContext ctx;
    ctx.hopCount = 1;
    ctx.visitedDevices.insert(DeviceKey(sourceNode, 1));

    packet.setForwardingContext(ctx);

    EXPECT_TRUE(packet.hasForwardingContext());

    // Context size: Type(1) + Length(1) + HopCount(1) + DeviceCount(1) + Device(6) = 10 bytes
    size_t expectedSize = MidiPacket::HEADER_SIZE + 3 + 10; // header + MIDI + context
    EXPECT_EQ(expectedSize, packet.getTotalSize());
}

// Test: Context with 4 devices
TEST_F(MidiPacketTest, Phase4_SerializeContextWithFourDevices) {
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    ForwardingContext ctx;
    ctx.hopCount = 4;

    juce::Uuid node1, node2, node3, node4;
    mockRegistry->registerNode(node1);
    mockRegistry->registerNode(node2);
    mockRegistry->registerNode(node3);
    mockRegistry->registerNode(node4);

    ctx.visitedDevices.insert(DeviceKey(node1, 1));
    ctx.visitedDevices.insert(DeviceKey(node2, 2));
    ctx.visitedDevices.insert(DeviceKey(node3, 3));
    ctx.visitedDevices.insert(DeviceKey(node4, 4));

    packet.setForwardingContext(ctx);

    EXPECT_TRUE(packet.hasForwardingContext());

    // Context size: 4 + (4 devices * 6) = 28 bytes
    size_t expectedSize = MidiPacket::HEADER_SIZE + 3 + 28;
    EXPECT_EQ(expectedSize, packet.getTotalSize());
}

// Test: Context with 8 devices (MAX_HOPS)
TEST_F(MidiPacketTest, Phase4_SerializeContextWithMaxDevices) {
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    ForwardingContext ctx;
    ctx.hopCount = 8;

    // Add 8 devices
    for (int i = 0; i < 8; ++i) {
        juce::Uuid nodeId;
        mockRegistry->registerNode(nodeId);
        ctx.visitedDevices.insert(DeviceKey(nodeId, static_cast<uint16_t>(i + 1)));
    }

    packet.setForwardingContext(ctx);

    EXPECT_TRUE(packet.hasForwardingContext());

    // Context size: 4 + (8 devices * 6) = 52 bytes
    size_t expectedSize = MidiPacket::HEADER_SIZE + 3 + 52;
    EXPECT_EQ(expectedSize, packet.getTotalSize());
}

// Test: Round-trip context preservation
TEST_F(MidiPacketTest, Phase4_RoundTripContextPreservation) {
    MidiPacket original = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    ForwardingContext originalCtx;
    originalCtx.hopCount = 3;

    juce::Uuid node1, node2;
    mockRegistry->registerNode(node1);
    mockRegistry->registerNode(node2);

    originalCtx.visitedDevices.insert(DeviceKey(node1, 10));
    originalCtx.visitedDevices.insert(DeviceKey(node2, 20));

    original.setForwardingContext(originalCtx);

    // Serialize and deserialize
    std::vector<uint8_t> serialized = original.serialize();
    MidiPacket deserialized = MidiPacket::deserialize(serialized.data(), serialized.size());

    EXPECT_TRUE(deserialized.hasForwardingContext());

    // Extract context using registry
    auto extractedCtxOpt = deserialized.getForwardingContext(*mockRegistry);
    ASSERT_TRUE(extractedCtxOpt.has_value());

    ForwardingContext extractedCtx = extractedCtxOpt.value();
    EXPECT_EQ(originalCtx.hopCount, extractedCtx.hopCount);
    EXPECT_EQ(originalCtx.visitedDevices.size(), extractedCtx.visitedDevices.size());

    // Verify visited devices
    for (const auto& devKey : originalCtx.visitedDevices) {
        EXPECT_TRUE(extractedCtx.visitedDevices.count(devKey) > 0);
    }
}

// Test: Backward compatibility - Phase 3 packet (no context)
TEST_F(MidiPacketTest, Phase4_BackwardCompatibilityPhase3Packet) {
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    // Don't set context - this is a Phase 3 packet
    EXPECT_FALSE(packet.hasForwardingContext());
    EXPECT_FALSE(packet.hasFlag(MidiPacket::HasContext));

    // Serialize and deserialize
    std::vector<uint8_t> serialized = packet.serialize();
    MidiPacket deserialized = MidiPacket::deserialize(serialized.data(), serialized.size());

    EXPECT_FALSE(deserialized.hasForwardingContext());

    auto ctxOpt = deserialized.getForwardingContext(*mockRegistry);
    EXPECT_FALSE(ctxOpt.has_value());
}

// Test: Clear forwarding context
TEST_F(MidiPacketTest, Phase4_ClearForwardingContext) {
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    ForwardingContext ctx;
    ctx.hopCount = 2;
    ctx.visitedDevices.insert(DeviceKey(sourceNode, 1));

    packet.setForwardingContext(ctx);
    EXPECT_TRUE(packet.hasForwardingContext());

    packet.clearForwardingContext();
    EXPECT_FALSE(packet.hasForwardingContext());
    EXPECT_FALSE(packet.hasFlag(MidiPacket::HasContext));
}

// Test: Invalid context data handling
TEST_F(MidiPacketTest, Phase4_InvalidContextDataReturnsNullopt) {
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    // Manually set corrupted context extension
    packet.addFlag(MidiPacket::HasContext);
    // contextExtension is empty or corrupted

    auto ctxOpt = packet.getForwardingContext(*mockRegistry);
    EXPECT_FALSE(ctxOpt.has_value());
}

// Test: Unknown node hash in context
TEST_F(MidiPacketTest, Phase4_UnknownNodeHashThrows) {
    MidiPacket original = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    ForwardingContext ctx;
    ctx.hopCount = 1;

    // Add device with node NOT in registry
    juce::Uuid unknownNode;
    // Don't register this node!
    ctx.visitedDevices.insert(DeviceKey(unknownNode, 99));

    original.setForwardingContext(ctx);

    // Serialize
    std::vector<uint8_t> serialized = original.serialize();
    MidiPacket deserialized = MidiPacket::deserialize(serialized.data(), serialized.size());

    // Attempt to extract context - should fail because unknownNode is not registered
    auto ctxOpt = deserialized.getForwardingContext(*mockRegistry);
    EXPECT_FALSE(ctxOpt.has_value());  // Should return nullopt on error
}

// Test: Packet size calculations with context
TEST_F(MidiPacketTest, Phase4_PacketSizeWithContext) {
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    // No context
    size_t sizeWithoutContext = packet.getTotalSize();
    EXPECT_EQ(MidiPacket::HEADER_SIZE + 3, sizeWithoutContext);

    // Add context with 2 devices
    ForwardingContext ctx;
    ctx.hopCount = 2;

    juce::Uuid node1, node2;
    ctx.visitedDevices.insert(DeviceKey(node1, 1));
    ctx.visitedDevices.insert(DeviceKey(node2, 2));

    packet.setForwardingContext(ctx);

    // Context size: 4 + (2 * 6) = 16 bytes
    size_t sizeWithContext = packet.getTotalSize();
    EXPECT_EQ(MidiPacket::HEADER_SIZE + 3 + 16, sizeWithContext);
}

// Test: serializeInto with context
TEST_F(MidiPacketTest, Phase4_SerializeIntoWithContext) {
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, sequence
    );

    ForwardingContext ctx;
    ctx.hopCount = 1;
    ctx.visitedDevices.insert(DeviceKey(sourceNode, 1));
    packet.setForwardingContext(ctx);

    uint8_t buffer[256];
    size_t bytesWritten = 0;

    bool success = packet.serializeInto(buffer, sizeof(buffer), bytesWritten);

    EXPECT_TRUE(success);
    EXPECT_EQ(packet.getTotalSize(), bytesWritten);
}

// Test: Context extension type constant
TEST_F(MidiPacketTest, Phase4_ContextExtensionTypeConstant) {
    EXPECT_EQ(0x01, MidiPacket::CONTEXT_EXTENSION_TYPE);
}

// Test: HasContext flag bit
TEST_F(MidiPacketTest, Phase4_HasContextFlagBit) {
    EXPECT_EQ(1 << 3, MidiPacket::HasContext);
}

// Test: Maximum packet size with max context
TEST_F(MidiPacketTest, Phase4_MaximumPacketSize) {
    // Max MIDI data (say 3 bytes) + max context (8 devices)
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};
    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, midiData, sequence
    );

    ForwardingContext ctx;
    ctx.hopCount = 8;

    for (int i = 0; i < 8; ++i) {
        juce::Uuid nodeId;
        ctx.visitedDevices.insert(DeviceKey(nodeId, static_cast<uint16_t>(i)));
    }

    packet.setForwardingContext(ctx);

    // Total: 20 (header) + 3 (MIDI) + 52 (context with 8 devices) = 75 bytes
    EXPECT_EQ(75u, packet.getTotalSize());
}
