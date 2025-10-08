#include "MidiPacket.h"
#include <juce_core/juce_core.h>
#include <cstring>
#include <stdexcept>
#include <chrono>

namespace NetworkMidi {

// Forward declaration - UuidRegistry will be implemented in Task 4.2
// For now, we use abstract interface to avoid circular dependency
class UuidRegistry {
public:
    virtual ~UuidRegistry() = default;
    virtual std::optional<juce::Uuid> lookupFromHash(uint32_t hash) const = 0;
};

MidiPacket::MidiPacket()
    : magic(MAGIC)
    , version(VERSION)
    , flags(0)
    , sequence(0)
    , timestampMicros(0)
    , deviceId(0)
    , packetType(Data)
{
}

MidiPacket MidiPacket::createDataPacket(
    const juce::Uuid& sourceNode,
    const juce::Uuid& destNode,
    uint16_t deviceId,
    const std::vector<uint8_t>& midiData,
    uint16_t sequence)
{
    MidiPacket packet;
    packet.setSourceNode(sourceNode);
    packet.setDestNode(destNode);
    packet.setDeviceId(deviceId);
    packet.setMidiData(midiData);
    packet.setSequence(sequence);
    packet.updateTimestamp();
    packet.setPacketType(Data);

    // Auto-detect SysEx and set flags
    if (!midiData.empty() && midiData[0] == 0xF0) {
        packet.addFlag(SysEx);
        packet.addFlag(Reliable);  // SysEx requires reliable delivery
    }

    return packet;
}

MidiPacket MidiPacket::createHeartbeatPacket(
    const juce::Uuid& sourceNode,
    const juce::Uuid& destNode,
    uint16_t sequence)
{
    MidiPacket packet;
    packet.setSourceNode(sourceNode);
    packet.setDestNode(destNode);
    packet.setSequence(sequence);
    packet.updateTimestamp();
    packet.setPacketType(Heartbeat);
    return packet;
}

MidiPacket MidiPacket::createAckPacket(
    const juce::Uuid& sourceNode,
    const juce::Uuid& destNode,
    uint16_t ackSequence)
{
    MidiPacket packet;
    packet.setSourceNode(sourceNode);
    packet.setDestNode(destNode);
    packet.setSequence(ackSequence);
    packet.updateTimestamp();
    packet.setPacketType(Ack);
    return packet;
}

MidiPacket MidiPacket::createNackPacket(
    const juce::Uuid& sourceNode,
    const juce::Uuid& destNode,
    uint16_t nackSequence)
{
    MidiPacket packet;
    packet.setSourceNode(sourceNode);
    packet.setDestNode(destNode);
    packet.setSequence(nackSequence);
    packet.updateTimestamp();
    packet.setPacketType(Nack);
    return packet;
}

std::vector<uint8_t> MidiPacket::serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(HEADER_SIZE + midiData.size() + contextExtension.size());

    // Magic (2 bytes, big-endian)
    buffer.push_back((magic >> 8) & 0xFF);
    buffer.push_back(magic & 0xFF);

    // Version (1 byte)
    buffer.push_back(version);

    // Flags (1 byte)
    buffer.push_back(flags);

    // Source Node UUID hash (4 bytes, big-endian)
    uint32_t sourceHash = hashUuid(sourceNode);
    buffer.push_back((sourceHash >> 24) & 0xFF);
    buffer.push_back((sourceHash >> 16) & 0xFF);
    buffer.push_back((sourceHash >> 8) & 0xFF);
    buffer.push_back(sourceHash & 0xFF);

    // Dest Node UUID hash (4 bytes, big-endian)
    uint32_t destHash = hashUuid(destNode);
    buffer.push_back((destHash >> 24) & 0xFF);
    buffer.push_back((destHash >> 16) & 0xFF);
    buffer.push_back((destHash >> 8) & 0xFF);
    buffer.push_back(destHash & 0xFF);

    // Sequence (2 bytes, big-endian)
    buffer.push_back((sequence >> 8) & 0xFF);
    buffer.push_back(sequence & 0xFF);

    // Timestamp (4 bytes, big-endian)
    buffer.push_back((timestampMicros >> 24) & 0xFF);
    buffer.push_back((timestampMicros >> 16) & 0xFF);
    buffer.push_back((timestampMicros >> 8) & 0xFF);
    buffer.push_back(timestampMicros & 0xFF);

    // Device ID (2 bytes, big-endian)
    buffer.push_back((deviceId >> 8) & 0xFF);
    buffer.push_back(deviceId & 0xFF);

    // MIDI data payload
    buffer.insert(buffer.end(), midiData.begin(), midiData.end());

    // Phase 4: Append context extension (if present)
    if (hasFlag(HasContext) && !contextExtension.empty()) {
        buffer.insert(buffer.end(), contextExtension.begin(), contextExtension.end());
    }

    return buffer;
}

bool MidiPacket::serializeInto(uint8_t* buffer, size_t bufferSize, size_t& bytesWritten) const {
    size_t requiredSize = HEADER_SIZE + midiData.size();
    if (hasFlag(HasContext)) {
        requiredSize += contextExtension.size();
    }

    if (bufferSize < requiredSize) {
        bytesWritten = 0;
        return false;
    }

    size_t offset = 0;

    // Magic (2 bytes, big-endian)
    buffer[offset++] = (magic >> 8) & 0xFF;
    buffer[offset++] = magic & 0xFF;

    // Version (1 byte)
    buffer[offset++] = version;

    // Flags (1 byte)
    buffer[offset++] = flags;

    // Source Node UUID hash (4 bytes, big-endian)
    uint32_t sourceHash = hashUuid(sourceNode);
    buffer[offset++] = (sourceHash >> 24) & 0xFF;
    buffer[offset++] = (sourceHash >> 16) & 0xFF;
    buffer[offset++] = (sourceHash >> 8) & 0xFF;
    buffer[offset++] = sourceHash & 0xFF;

    // Dest Node UUID hash (4 bytes, big-endian)
    uint32_t destHash = hashUuid(destNode);
    buffer[offset++] = (destHash >> 24) & 0xFF;
    buffer[offset++] = (destHash >> 16) & 0xFF;
    buffer[offset++] = (destHash >> 8) & 0xFF;
    buffer[offset++] = destHash & 0xFF;

    // Sequence (2 bytes, big-endian)
    buffer[offset++] = (sequence >> 8) & 0xFF;
    buffer[offset++] = sequence & 0xFF;

    // Timestamp (4 bytes, big-endian)
    buffer[offset++] = (timestampMicros >> 24) & 0xFF;
    buffer[offset++] = (timestampMicros >> 16) & 0xFF;
    buffer[offset++] = (timestampMicros >> 8) & 0xFF;
    buffer[offset++] = timestampMicros & 0xFF;

    // Device ID (2 bytes, big-endian)
    buffer[offset++] = (deviceId >> 8) & 0xFF;
    buffer[offset++] = deviceId & 0xFF;

    // MIDI data payload
    if (!midiData.empty()) {
        std::memcpy(buffer + offset, midiData.data(), midiData.size());
        offset += midiData.size();
    }

    // Phase 4: Append context extension (if present)
    if (hasFlag(HasContext) && !contextExtension.empty()) {
        std::memcpy(buffer + offset, contextExtension.data(), contextExtension.size());
        offset += contextExtension.size();
    }

    bytesWritten = offset;
    return true;
}

MidiPacket MidiPacket::deserialize(const uint8_t* data, size_t length) {
    MidiPacket packet;
    if (!tryDeserialize(data, length, packet)) {
        throw std::runtime_error("Failed to deserialize MIDI packet: invalid format");
    }
    return packet;
}

bool MidiPacket::tryDeserialize(const uint8_t* data, size_t length, MidiPacket& outPacket) {
    if (length < HEADER_SIZE) {
        return false;
    }

    size_t offset = 0;

    // Magic (2 bytes, big-endian)
    uint16_t magic = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;
    if (magic != MAGIC) {
        return false;
    }

    // Version (1 byte)
    uint8_t version = data[offset++];
    if (version != VERSION) {
        return false;  // Incompatible version
    }

    // Flags (1 byte)
    uint8_t flags = data[offset++];

    // Source Node UUID hash (4 bytes, big-endian)
    uint32_t sourceHash = (static_cast<uint32_t>(data[offset]) << 24) |
                          (static_cast<uint32_t>(data[offset + 1]) << 16) |
                          (static_cast<uint32_t>(data[offset + 2]) << 8) |
                          static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    // Dest Node UUID hash (4 bytes, big-endian)
    uint32_t destHash = (static_cast<uint32_t>(data[offset]) << 24) |
                        (static_cast<uint32_t>(data[offset + 1]) << 16) |
                        (static_cast<uint32_t>(data[offset + 2]) << 8) |
                        static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    // Sequence (2 bytes, big-endian)
    uint16_t sequence = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    // Timestamp (4 bytes, big-endian)
    uint32_t timestamp = (static_cast<uint32_t>(data[offset]) << 24) |
                         (static_cast<uint32_t>(data[offset + 1]) << 16) |
                         (static_cast<uint32_t>(data[offset + 2]) << 8) |
                         static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    // Device ID (2 bytes, big-endian)
    uint16_t deviceId = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    // Calculate MIDI data size and context extension size
    size_t remainingBytes = length - HEADER_SIZE;
    size_t midiDataSize = remainingBytes;
    size_t contextSize = 0;

    // Phase 4: Check for context extension
    bool hasContextFlag = (flags & HasContext) != 0;

    std::vector<uint8_t> midiData;
    std::vector<uint8_t> contextExtension;

    if (hasContextFlag && remainingBytes > 0) {
        // Scan for context extension from the end of the packet
        // Context format: Type(1) + Length(1) + HopCount(1) + DeviceCount(1) + Devices(N*6)
        // We need to find where MIDI data ends and context begins

        // Look for context extension type marker
        // The extension is at the end of the packet, so we scan backwards
        size_t scanPos = offset;
        while (scanPos < length) {
            if (data[scanPos] == CONTEXT_EXTENSION_TYPE && (scanPos + 1) < length) {
                uint8_t extLength = data[scanPos + 1];
                // Validate that extension fits
                if (scanPos + extLength <= length && extLength >= CONTEXT_HEADER_SIZE) {
                    // Found valid context extension
                    midiDataSize = scanPos - offset;
                    contextSize = length - scanPos;
                    break;
                }
            }
            scanPos++;
        }
    }

    // Extract MIDI data payload
    if (midiDataSize > 0) {
        midiData.resize(midiDataSize);
        std::memcpy(midiData.data(), data + offset, midiDataSize);
    }
    offset += midiDataSize;

    // Extract context extension (if present)
    if (hasContextFlag && contextSize > 0) {
        contextExtension.resize(contextSize);
        std::memcpy(contextExtension.data(), data + offset, contextSize);
    }

    // Populate output packet
    outPacket.magic = magic;
    outPacket.version = version;
    outPacket.flags = flags;
    // Note: We only have hash values, not full UUIDs
    // The caller must maintain UUID mappings
    outPacket.sequence = sequence;
    outPacket.timestampMicros = timestamp;
    outPacket.deviceId = deviceId;
    outPacket.midiData = std::move(midiData);
    outPacket.contextExtension = std::move(contextExtension);

    return true;
}

bool MidiPacket::tryDeserialize(const uint8_t* data, size_t length, MidiPacket& outPacket,
                                 const UuidRegistry* registry)
{
    // First call the basic deserialization
    if (!tryDeserialize(data, length, outPacket)) {
        return false;
    }

    // If registry is provided, look up full UUIDs from hashes
    if (registry) {
        // Re-parse the hash values from the packet (they're at fixed offsets)
        constexpr size_t SOURCE_HASH_OFFSET = 4;  // After magic(2) + version(1) + flags(1)
        constexpr size_t DEST_HASH_OFFSET = 8;    // After source hash(4)

        uint32_t sourceHash = (static_cast<uint32_t>(data[SOURCE_HASH_OFFSET]) << 24) |
                              (static_cast<uint32_t>(data[SOURCE_HASH_OFFSET + 1]) << 16) |
                              (static_cast<uint32_t>(data[SOURCE_HASH_OFFSET + 2]) << 8) |
                              static_cast<uint32_t>(data[SOURCE_HASH_OFFSET + 3]);

        uint32_t destHash = (static_cast<uint32_t>(data[DEST_HASH_OFFSET]) << 24) |
                            (static_cast<uint32_t>(data[DEST_HASH_OFFSET + 1]) << 16) |
                            (static_cast<uint32_t>(data[DEST_HASH_OFFSET + 2]) << 8) |
                            static_cast<uint32_t>(data[DEST_HASH_OFFSET + 3]);

        // Look up full UUIDs
        auto sourceUuid = registry->lookupFromHash(sourceHash);
        auto destUuid = registry->lookupFromHash(destHash);

        if (sourceUuid) {
            outPacket.sourceNode = *sourceUuid;
        } else {
            std::cerr << "WARNING: Source UUID hash 0x" << std::hex << sourceHash
                      << " not found in registry" << std::dec << std::endl;
        }

        if (destUuid) {
            outPacket.destNode = *destUuid;
        } else {
            std::cerr << "WARNING: Dest UUID hash 0x" << std::hex << destHash
                      << " not found in registry" << std::dec << std::endl;
        }
    }

    return true;
}

bool MidiPacket::isValid() const {
    return magic == MAGIC && version == VERSION;
}

bool MidiPacket::verifyChecksum() const {
    // Simple checksum verification placeholder
    // Could be enhanced with CRC32 or similar
    return isValid();
}

void MidiPacket::setSourceNode(const juce::Uuid& uuid) {
    sourceNode = uuid;
}

void MidiPacket::setDestNode(const juce::Uuid& uuid) {
    destNode = uuid;
}

void MidiPacket::setMidiData(const std::vector<uint8_t>& data) {
    midiData = data;

    // Auto-detect SysEx
    if (!data.empty() && data[0] == 0xF0) {
        addFlag(SysEx);
        addFlag(Reliable);
    }
}

void MidiPacket::updateTimestamp() {
    timestampMicros = static_cast<uint32_t>(getCurrentTimeMicros());
}

// Phase 4: Forwarding context support

void MidiPacket::setForwardingContext(const ForwardingContext& ctx) {
    contextExtension = serializeContext(ctx);
    addFlag(HasContext);
}

std::optional<ForwardingContext> MidiPacket::getForwardingContext(const UuidRegistry& registry) const {
    if (!hasFlag(HasContext) || contextExtension.empty()) {
        return std::nullopt;
    }

    try {
        return deserializeContext(contextExtension.data(), contextExtension.size(), registry);
    } catch (const std::exception&) {
        // Deserialization failed - return nullopt
        return std::nullopt;
    }
}

void MidiPacket::clearForwardingContext() {
    contextExtension.clear();
    removeFlag(HasContext);
}

size_t MidiPacket::getTotalSize() const {
    size_t total = HEADER_SIZE + midiData.size();
    if (hasFlag(HasContext)) {
        total += contextExtension.size();
    }
    return total;
}

// Context serialization/deserialization

std::vector<uint8_t> MidiPacket::serializeContext(const ForwardingContext& ctx) {
    std::vector<uint8_t> buffer;

    // Limit device count to MAX_HOPS (8)
    uint8_t deviceCount = static_cast<uint8_t>(
        std::min(ctx.visitedDevices.size(), static_cast<size_t>(ForwardingContext::MAX_HOPS))
    );

    // Calculate extension length: header(4) + devices(N * 6)
    uint8_t extLength = static_cast<uint8_t>(CONTEXT_HEADER_SIZE + (deviceCount * VISITED_DEVICE_SIZE));

    // Reserve space
    buffer.reserve(extLength);

    // Extension Type (1 byte)
    buffer.push_back(CONTEXT_EXTENSION_TYPE);

    // Extension Length (1 byte)
    buffer.push_back(extLength);

    // Hop Count (1 byte)
    buffer.push_back(ctx.hopCount);

    // Device Count (1 byte)
    buffer.push_back(deviceCount);

    // Visited Devices (6 bytes each: nodeIdHash(4) + deviceId(2))
    size_t count = 0;
    for (const auto& devKey : ctx.visitedDevices) {
        if (count >= deviceCount) break;

        // Node ID Hash (4 bytes, big-endian)
        uint32_t hash = hashUuid(devKey.ownerNode);
        buffer.push_back((hash >> 24) & 0xFF);
        buffer.push_back((hash >> 16) & 0xFF);
        buffer.push_back((hash >> 8) & 0xFF);
        buffer.push_back(hash & 0xFF);

        // Device ID (2 bytes, big-endian)
        buffer.push_back((devKey.deviceId >> 8) & 0xFF);
        buffer.push_back(devKey.deviceId & 0xFF);

        count++;
    }

    return buffer;
}

ForwardingContext MidiPacket::deserializeContext(const uint8_t* data, size_t length,
                                                  const UuidRegistry& registry) {
    if (length < CONTEXT_HEADER_SIZE) {
        throw std::runtime_error("Context extension too short");
    }

    size_t offset = 0;

    // Extension Type (1 byte)
    uint8_t extType = data[offset++];
    if (extType != CONTEXT_EXTENSION_TYPE) {
        throw std::runtime_error("Invalid context extension type");
    }

    // Extension Length (1 byte)
    uint8_t extLength = data[offset++];
    if (extLength != length) {
        throw std::runtime_error("Context extension length mismatch");
    }

    // Hop Count (1 byte)
    uint8_t hopCount = data[offset++];

    // Device Count (1 byte)
    uint8_t deviceCount = data[offset++];

    // Validate expected length
    size_t expectedLength = CONTEXT_HEADER_SIZE + (deviceCount * VISITED_DEVICE_SIZE);
    if (extLength != expectedLength) {
        throw std::runtime_error("Context device count mismatch");
    }

    if (length < expectedLength) {
        throw std::runtime_error("Context extension truncated");
    }

    // Deserialize visited devices
    ForwardingContext ctx;
    ctx.hopCount = hopCount;

    for (uint8_t i = 0; i < deviceCount; ++i) {
        // Node ID Hash (4 bytes, big-endian)
        uint32_t hash = (static_cast<uint32_t>(data[offset]) << 24) |
                        (static_cast<uint32_t>(data[offset + 1]) << 16) |
                        (static_cast<uint32_t>(data[offset + 2]) << 8) |
                        static_cast<uint32_t>(data[offset + 3]);
        offset += 4;

        // Device ID (2 bytes, big-endian)
        uint16_t devId = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
        offset += 2;

        // Lookup UUID from hash via registry
        auto nodeIdOpt = registry.lookupFromHash(hash);
        if (!nodeIdOpt.has_value()) {
            throw std::runtime_error("Unknown node hash in context: " +
                                     juce::String::toHexString(static_cast<int>(hash)).toStdString());
        }

        // Add to visited devices
        ctx.visitedDevices.insert(DeviceKey(nodeIdOpt.value(), devId));
    }

    return ctx;
}

uint32_t MidiPacket::hashUuid(const juce::Uuid& uuid) {
    // Simple hash: XOR the 4 32-bit words of the UUID
    const uint64_t* data = reinterpret_cast<const uint64_t*>(uuid.getRawData());
    uint32_t hash = static_cast<uint32_t>(data[0] ^ (data[0] >> 32));
    hash ^= static_cast<uint32_t>(data[1] ^ (data[1] >> 32));
    return hash;
}

juce::Uuid MidiPacket::unhashUuid(uint32_t hash, const juce::Uuid& original) {
    // Hash is lossy, so we can't reverse it
    // This function is a placeholder for when we have UUID lookup tables
    return original;
}

uint64_t MidiPacket::getCurrentTimeMicros() {
    auto now = std::chrono::high_resolution_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    );
    return micros.count();
}

} // namespace NetworkMidi
