#pragma once

#include <juce_core/juce_core.h>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <optional>
#include <cstdint>

namespace NetworkMidi {

/**
 * Thread-safe registry that maps 32-bit UUID hashes to full juce::Uuid objects.
 *
 * This registry enables context deserialization during multi-hop routing by
 * providing reverse lookup from compact hash values (used in ForwardingContext)
 * to full UUID objects (required for DeviceKey construction).
 *
 * The registry is populated by MeshManager on connection establishment and
 * queried by MidiRouter during context deserialization.
 *
 * Hash Collision Risk:
 * - 32-bit hash: ~1 in 4 billion collision probability per pair
 * - Acceptable for mesh networks <1000 nodes
 * - Collisions are detected and logged
 *
 * Performance Characteristics:
 * - Register: O(1) with mutex lock
 * - Lookup: O(1) with mutex lock
 * - Memory: ~16 bytes per registered node (hash + UUID)
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses std::mutex for synchronization
 * - Safe for concurrent register/lookup operations
 */
class UuidRegistry {
public:
    /**
     * Constructor
     */
    UuidRegistry();

    /**
     * Destructor
     */
    ~UuidRegistry();

    /**
     * Register a node UUID for hash lookup.
     * Called when a connection is established.
     *
     * If the hash already exists:
     * - If UUID matches: no-op (duplicate registration)
     * - If UUID differs: logs collision error, keeps first registration
     *
     * @param nodeId The full UUID to register
     */
    void registerNode(const juce::Uuid& nodeId);

    /**
     * Unregister a node UUID.
     * Called when a connection is closed.
     *
     * If the UUID is not registered, this is a no-op.
     *
     * @param nodeId The UUID to unregister
     */
    void unregisterNode(const juce::Uuid& nodeId);

    /**
     * Look up a full UUID from its 32-bit hash.
     * Called during context deserialization.
     *
     * @param hash The 32-bit hash value
     * @return The full UUID if found, std::nullopt otherwise
     */
    std::optional<juce::Uuid> lookupFromHash(uint32_t hash) const;

    /**
     * Get the number of currently registered nodes.
     *
     * @return Number of registered nodes
     */
    size_t getRegisteredNodeCount() const;

    /**
     * Get all registered node UUIDs.
     * Useful for debugging and statistics.
     *
     * @return Vector of all registered UUIDs (copy)
     */
    std::vector<juce::Uuid> getAllNodes() const;

    /**
     * Check if a hash has a collision (multiple UUIDs map to same hash).
     *
     * Note: Current implementation only tracks the first registered UUID
     * per hash, so this will return false unless specifically tracked.
     *
     * @param hash The hash to check
     * @return True if collision detected, false otherwise
     */
    bool hasCollision(uint32_t hash) const;

    /**
     * Clear all registered nodes.
     * Useful for testing and shutdown cleanup.
     */
    void clear();

    /**
     * Compute 32-bit hash of a UUID.
     * Uses the same algorithm as MidiPacket::hashUuid().
     *
     * This is made public for testing and consistency verification.
     *
     * Algorithm: XOR-based hash of UUID's 128 bits
     * - UUID is 16 bytes (2 uint64_t values)
     * - Each uint64_t is XORed with itself shifted right 32 bits
     * - Results are XORed together to produce final 32-bit hash
     *
     * @param uuid The UUID to hash
     * @return 32-bit hash value
     */
    static uint32_t computeHash(const juce::Uuid& uuid);

private:
    // Hash-to-UUID mapping
    std::unordered_map<uint32_t, juce::Uuid> hashToUuid;

    // UUID-to-hash mapping (for efficient unregister)
    std::unordered_map<juce::Uuid, uint32_t> uuidToHash;

    // Detected collisions (hash values that had conflicts)
    std::unordered_set<uint32_t> collisions;

    // Thread synchronization
    mutable std::mutex mutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UuidRegistry)
};

} // namespace NetworkMidi
