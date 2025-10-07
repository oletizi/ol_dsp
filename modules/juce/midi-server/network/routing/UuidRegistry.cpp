#include "UuidRegistry.h"
#include <juce_core/juce_core.h>

namespace NetworkMidi {

UuidRegistry::UuidRegistry()
{
}

UuidRegistry::~UuidRegistry()
{
    std::lock_guard<std::mutex> lock(mutex);
    hashToUuid.clear();
    uuidToHash.clear();
    collisions.clear();
}

void UuidRegistry::registerNode(const juce::Uuid& nodeId)
{
    if (nodeId.isNull()) {
        DBG("UuidRegistry: Attempted to register null UUID");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    uint32_t hash = computeHash(nodeId);

    // Check for existing registration
    auto hashIt = hashToUuid.find(hash);
    if (hashIt != hashToUuid.end()) {
        // Hash already registered
        if (hashIt->second == nodeId) {
            // Same UUID - duplicate registration, no-op
            DBG("UuidRegistry: Duplicate registration for UUID " + nodeId.toString());
            return;
        } else {
            // Hash collision detected!
            DBG("UuidRegistry: COLLISION DETECTED! Hash " + juce::String::toHexString(static_cast<int>(hash)) +
                " already maps to " + hashIt->second.toString() +
                ", attempted to register " + nodeId.toString());
            collisions.insert(hash);
            // Keep first registration, don't overwrite
            return;
        }
    }

    // Register new mapping
    hashToUuid[hash] = nodeId;
    uuidToHash[nodeId] = hash;

    DBG("UuidRegistry: Registered node " + nodeId.toString() +
        " with hash " + juce::String::toHexString(static_cast<int>(hash)) +
        " (total nodes: " + juce::String(hashToUuid.size()) + ")");
}

void UuidRegistry::unregisterNode(const juce::Uuid& nodeId)
{
    if (nodeId.isNull()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    // Find hash for this UUID
    auto uuidIt = uuidToHash.find(nodeId);
    if (uuidIt == uuidToHash.end()) {
        // Not registered, no-op
        DBG("UuidRegistry: Attempted to unregister unknown UUID " + nodeId.toString());
        return;
    }

    uint32_t hash = uuidIt->second;

    // Remove both mappings
    hashToUuid.erase(hash);
    uuidToHash.erase(uuidIt);

    // Remove from collisions set if present
    collisions.erase(hash);

    DBG("UuidRegistry: Unregistered node " + nodeId.toString() +
        " (remaining nodes: " + juce::String(hashToUuid.size()) + ")");
}

std::optional<juce::Uuid> UuidRegistry::lookupFromHash(uint32_t hash) const
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = hashToUuid.find(hash);
    if (it != hashToUuid.end()) {
        return it->second;
    }

    return std::nullopt;
}

size_t UuidRegistry::getRegisteredNodeCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return hashToUuid.size();
}

std::vector<juce::Uuid> UuidRegistry::getAllNodes() const
{
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<juce::Uuid> nodes;
    nodes.reserve(hashToUuid.size());

    for (const auto& pair : hashToUuid) {
        nodes.push_back(pair.second);
    }

    return nodes;
}

bool UuidRegistry::hasCollision(uint32_t hash) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return collisions.find(hash) != collisions.end();
}

void UuidRegistry::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    hashToUuid.clear();
    uuidToHash.clear();
    collisions.clear();
    DBG("UuidRegistry: Cleared all registrations");
}

uint32_t UuidRegistry::computeHash(const juce::Uuid& uuid)
{
    // Use the same hash algorithm as MidiPacket::hashUuid()
    // This ensures consistency across the system
    //
    // Algorithm: XOR-based hash of UUID's 128 bits
    // - UUID is 16 bytes represented as 2 uint64_t values
    // - Each uint64_t is XORed with itself shifted right 32 bits to produce uint32_t
    // - The two uint32_t results are XORed together for final hash
    //
    // This provides good distribution for UUID-4 random values

    const uint64_t* data = reinterpret_cast<const uint64_t*>(uuid.getRawData());

    // Extract high and low 64-bit words
    uint64_t high64 = data[0];
    uint64_t low64 = data[1];

    // Fold each 64-bit value into 32 bits via XOR
    uint32_t highHash = static_cast<uint32_t>(high64 ^ (high64 >> 32));
    uint32_t lowHash = static_cast<uint32_t>(low64 ^ (low64 >> 32));

    // Combine the two 32-bit hashes
    uint32_t finalHash = highHash ^ lowHash;

    return finalHash;
}

} // namespace NetworkMidi
