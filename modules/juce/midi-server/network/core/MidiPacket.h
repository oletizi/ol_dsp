#pragma once

#include <juce_core/juce_core.h>
#include <cstdint>
#include <vector>

namespace NetworkMidi {

/**
 * UDP packet format for network MIDI transport.
 *
 * Packet Structure (20-byte header + variable payload):
 * - Magic: 0x4D49 ("MI") - 2 bytes
 * - Version: 0x01 - 1 byte
 * - Flags: [SysEx|Reliable|Fragment|Reserved] - 1 byte
 * - Source Node UUID (hash) - 4 bytes
 * - Dest Node UUID (hash) - 4 bytes
 * - Sequence Number - 2 bytes
 * - Timestamp (microseconds) - 4 bytes
 * - Device ID - 2 bytes
 * - MIDI Data (variable length)
 */
class MidiPacket {
public:
    // Packet type constants
    enum PacketType : uint8_t {
        Data = 0x00,
        Heartbeat = 0x01,
        Ack = 0x02,
        Nack = 0x03
    };

    // Flag bits
    enum Flags : uint8_t {
        None = 0,
        SysEx = 1 << 0,      // Bit 0: SysEx message
        Reliable = 1 << 1,   // Bit 1: Reliable delivery required (ACK expected)
        Fragment = 1 << 2,   // Bit 2: Fragmented message (part of larger SysEx)
        Reserved3 = 1 << 3,  // Bits 3-7: Reserved for future use
        Reserved4 = 1 << 4,
        Reserved5 = 1 << 5,
        Reserved6 = 1 << 6,
        Reserved7 = 1 << 7
    };

    // Header constants
    static constexpr uint16_t MAGIC = 0x4D49;  // "MI"
    static constexpr uint8_t VERSION = 0x01;
    static constexpr size_t HEADER_SIZE = 20;

    // Constructor
    MidiPacket();

    // Factory methods for different packet types
    static MidiPacket createDataPacket(
        const juce::Uuid& sourceNode,
        const juce::Uuid& destNode,
        uint16_t deviceId,
        const std::vector<uint8_t>& midiData,
        uint16_t sequence
    );

    static MidiPacket createHeartbeatPacket(
        const juce::Uuid& sourceNode,
        const juce::Uuid& destNode,
        uint16_t sequence
    );

    static MidiPacket createAckPacket(
        const juce::Uuid& sourceNode,
        const juce::Uuid& destNode,
        uint16_t ackSequence
    );

    static MidiPacket createNackPacket(
        const juce::Uuid& sourceNode,
        const juce::Uuid& destNode,
        uint16_t nackSequence
    );

    // Serialization
    std::vector<uint8_t> serialize() const;
    bool serializeInto(uint8_t* buffer, size_t bufferSize, size_t& bytesWritten) const;

    // Deserialization
    static MidiPacket deserialize(const uint8_t* data, size_t length);
    static bool tryDeserialize(const uint8_t* data, size_t length, MidiPacket& outPacket);

    // Validation
    bool isValid() const;
    bool verifyChecksum() const;

    // Getters
    uint16_t getMagic() const { return magic; }
    uint8_t getVersion() const { return version; }
    uint8_t getFlags() const { return flags; }
    juce::Uuid getSourceNode() const { return sourceNode; }
    juce::Uuid getDestNode() const { return destNode; }
    uint16_t getSequence() const { return sequence; }
    uint32_t getTimestampMicros() const { return timestampMicros; }
    uint16_t getDeviceId() const { return deviceId; }
    const std::vector<uint8_t>& getMidiData() const { return midiData; }
    PacketType getPacketType() const { return packetType; }

    // Setters
    void setSourceNode(const juce::Uuid& uuid);
    void setDestNode(const juce::Uuid& uuid);
    void setSequence(uint16_t seq) { sequence = seq; }
    void setDeviceId(uint16_t id) { deviceId = id; }
    void setMidiData(const std::vector<uint8_t>& data);
    void setFlags(uint8_t f) { flags = f; }
    void addFlag(Flags flag) { flags |= flag; }
    void removeFlag(Flags flag) { flags &= ~flag; }
    void setPacketType(PacketType type) { packetType = type; }

    // Flag queries
    bool hasFlag(Flags flag) const { return (flags & flag) != 0; }
    bool isSysEx() const { return hasFlag(SysEx); }
    bool isReliable() const { return hasFlag(Reliable); }
    bool isFragment() const { return hasFlag(Fragment); }

    // Utility
    size_t getTotalSize() const { return HEADER_SIZE + midiData.size(); }
    void updateTimestamp();  // Sets timestamp to current time in microseconds

private:
    // Header fields
    uint16_t magic;
    uint8_t version;
    uint8_t flags;
    juce::Uuid sourceNode;
    juce::Uuid destNode;
    uint16_t sequence;
    uint32_t timestampMicros;
    uint16_t deviceId;

    // Payload
    std::vector<uint8_t> midiData;

    // Packet type (not serialized, derived from context)
    PacketType packetType;

    // Helper functions
    static uint32_t hashUuid(const juce::Uuid& uuid);
    static juce::Uuid unhashUuid(uint32_t hash, const juce::Uuid& original);
    static uint64_t getCurrentTimeMicros();
};

} // namespace NetworkMidi
