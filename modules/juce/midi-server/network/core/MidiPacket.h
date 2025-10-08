#pragma once

#include "../routing/DeviceRegistry.h"  // For DeviceKey
#include <juce_core/juce_core.h>
#include <cstdint>
#include <vector>
#include <optional>
#include <set>

namespace NetworkMidi {

// Forward declaration for UuidRegistry (to be implemented in Task 4.2)
class UuidRegistry;

// Forwarding context for loop prevention (uses DeviceKey from DeviceRegistry.h)
struct ForwardingContext {
    std::set<DeviceKey> visitedDevices;
    uint8_t hopCount = 0;
    static constexpr uint8_t MAX_HOPS = 8;
};

/**
 * UDP packet format for network MIDI transport.
 *
 * Packet Structure (20-byte header + variable payload):
 * - Magic: 0x4D49 ("MI") - 2 bytes
 * - Version: 0x01 - 1 byte
 * - Flags: [SysEx|Reliable|Fragment|HasContext|Reserved...] - 1 byte
 * - Source Node UUID (hash) - 4 bytes
 * - Dest Node UUID (hash) - 4 bytes
 * - Sequence Number - 2 bytes
 * - Timestamp (microseconds) - 4 bytes
 * - Device ID - 2 bytes
 * - MIDI Data (variable length)
 * - Context Extension (optional, if HasContext flag set)
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
        HasContext = 1 << 3, // Bit 3: Forwarding context extension present (Phase 4)
        Reserved4 = 1 << 4,  // Bits 4-7: Reserved for future use
        Reserved5 = 1 << 5,
        Reserved6 = 1 << 6,
        Reserved7 = 1 << 7
    };

    // Header constants
    static constexpr uint16_t MAGIC = 0x4D49;  // "MI"
    static constexpr uint8_t VERSION = 0x01;
    static constexpr size_t HEADER_SIZE = 20;

    // Context extension constants
    static constexpr uint8_t CONTEXT_EXTENSION_TYPE = 0x01;
    static constexpr size_t CONTEXT_HEADER_SIZE = 4;  // Type(1) + Length(1) + HopCount(1) + DeviceCount(1)
    static constexpr size_t VISITED_DEVICE_SIZE = 6;  // NodeIdHash(4) + DeviceId(2)

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
    static bool tryDeserialize(const uint8_t* data, size_t length, MidiPacket& outPacket,
                                const UuidRegistry* registry);

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

    // Phase 4: Forwarding context support
    /**
     * Set forwarding context to be serialized with this packet.
     * Automatically sets the HasContext flag.
     */
    void setForwardingContext(const ForwardingContext& ctx);

    /**
     * Extract forwarding context from packet (if present).
     * Requires UuidRegistry to reverse lookup node UUIDs from hashes.
     * Returns std::nullopt if no context present or deserialization fails.
     */
    std::optional<ForwardingContext> getForwardingContext(const UuidRegistry& registry) const;

    /**
     * Check if packet has forwarding context extension.
     */
    bool hasForwardingContext() const { return hasFlag(HasContext); }

    /**
     * Clear forwarding context and remove HasContext flag.
     */
    void clearForwardingContext();

    // Utility
    size_t getTotalSize() const;
    void updateTimestamp();  // Sets timestamp to current time in microseconds

    // Helper for UUID hashing (public for UuidRegistry)
    static uint32_t hashUuid(const juce::Uuid& uuid);

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

    // Phase 4: Context extension (optional)
    std::vector<uint8_t> contextExtension;

    // Packet type (not serialized, derived from context)
    PacketType packetType;

    // Helper functions
    static juce::Uuid unhashUuid(uint32_t hash, const juce::Uuid& original);
    static uint64_t getCurrentTimeMicros();

    // Context serialization/deserialization helpers
    static std::vector<uint8_t> serializeContext(const ForwardingContext& ctx);
    static ForwardingContext deserializeContext(const uint8_t* data, size_t length,
                                                 const UuidRegistry& registry);
};

} // namespace NetworkMidi
