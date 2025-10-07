/**
 * NodeIdentity - Manages unique node identification for network MIDI mesh
 *
 * Provides persistent UUID-based node identification that survives restarts.
 * Each node has a unique ID stored in ~/.midi-network/node-id
 */

#pragma once

#include <juce_core/juce_core.h>
#include <string>

namespace NetworkMidi {

/**
 * Manages unique node identity for the MIDI mesh network.
 *
 * Each server instance gets a unique UUID, allowing multiple instances
 * on the same machine. The UUID can optionally be persisted to support
 * stable identity across restarts for production deployments.
 *
 * For multi-instance scenarios (testing, development), each instance
 * generates a fresh UUID automatically.
 */
class NodeIdentity
{
public:
    /**
     * Create a new node identity with a fresh UUID.
     * This allows multiple instances on the same machine.
     */
    NodeIdentity();

    /**
     * Create a node identity with persistence.
     * If configDir is provided, loads/saves UUID from configDir/node-id.
     * If configDir is empty, generates a fresh UUID (no persistence).
     */
    explicit NodeIdentity(const juce::String& configDir);

    /**
     * Create a node identity with a specific UUID.
     * Used for testing and integration scenarios where the UUID
     * must be controlled externally.
     */
    static NodeIdentity createWithUuid(const juce::Uuid& customUuid);

    /**
     * Get the unique node UUID.
     * This UUID is persistent across restarts.
     */
    juce::Uuid getNodeId() const;

    /**
     * Get the human-readable node name.
     * Format: "{hostname}-{uuid-prefix}"
     * Example: "studio-mac-a1b2c3d4"
     */
    juce::String getNodeName() const;

    /**
     * Get the system hostname.
     */
    juce::String getHostname() const;

    /**
     * Get the path to the node ID file.
     */
    juce::File getIdFile() const;

    /**
     * Regenerate the node ID (for testing or collision recovery).
     * Returns the new UUID.
     */
    juce::Uuid regenerateId();

    ~NodeIdentity() = default;

private:
    /**
     * Private constructor for creating identity with custom UUID.
     */
    NodeIdentity(const juce::Uuid& customUuid, bool /*tag*/);

    /**
     * Load existing UUID from disk, or create new one if not found.
     */
    juce::Uuid loadOrCreateId();

    /**
     * Save the UUID to disk.
     */
    void saveId(const juce::Uuid& uuid);

    /**
     * Generate a human-readable node name from hostname and UUID.
     */
    juce::String generateNodeName(const juce::Uuid& uuid) const;

    juce::Uuid nodeId;
    juce::String nodeName;
    juce::String hostname;
    juce::File idFile;
};

} // namespace NetworkMidi
