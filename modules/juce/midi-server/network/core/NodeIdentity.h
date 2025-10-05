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
 * On first startup, generates a random UUID and persists it to disk.
 * On subsequent startups, loads the existing UUID from disk.
 *
 * This ensures that a node maintains the same identity across restarts,
 * which is important for stable mesh topology and device routing.
 */
class NodeIdentity
{
public:
    /**
     * Get the singleton instance.
     * Creates and initializes the identity on first access.
     */
    static NodeIdentity& getInstance();

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

private:
    NodeIdentity();
    ~NodeIdentity() = default;

    // Prevent copying
    NodeIdentity(const NodeIdentity&) = delete;
    NodeIdentity& operator=(const NodeIdentity&) = delete;

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
