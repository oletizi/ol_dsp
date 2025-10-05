/**
 * InstanceManager - Manages instance isolation for multi-instance support
 *
 * Each instance gets a unique temporary directory for state files and a lock file
 * to detect UUID collisions and orphaned instances.
 */

#pragma once

#include <juce_core/juce_core.h>

namespace NetworkMidi {

/**
 * Manages instance-specific resources and isolation.
 *
 * Each running instance gets:
 * - Unique temporary directory: /tmp/midi-network-{uuid}/
 * - Lock file to prevent UUID collisions
 * - State file storage for runtime data
 *
 * Automatically cleans up resources on shutdown.
 */
class InstanceManager
{
public:
    /**
     * Create instance manager for a given node UUID.
     *
     * @param nodeId The UUID of this node
     * @throws std::runtime_error if lock file indicates another instance with same UUID
     */
    explicit InstanceManager(const juce::Uuid& nodeId);

    /**
     * Destructor - automatically cleans up instance directory and lock file.
     */
    ~InstanceManager();

    // Prevent copying
    InstanceManager(const InstanceManager&) = delete;
    InstanceManager& operator=(const InstanceManager&) = delete;

    /**
     * Get the instance-specific temporary directory.
     * Format: /tmp/midi-network-{uuid}/
     */
    juce::File getInstanceDirectory() const;

    /**
     * Get a state file within the instance directory.
     *
     * @param name The name of the state file (e.g., "port-mappings.json")
     * @return File object for the state file
     */
    juce::File getStateFile(const juce::String& name) const;

    /**
     * Clean up instance directory and lock file.
     * Called automatically by destructor, but can be called explicitly.
     */
    void cleanup();

    /**
     * Check if the lock file is stale (from a crashed process).
     * Returns true if the lock file exists but the process is no longer running.
     */
    bool isLockStale() const;

    /**
     * Get the process ID stored in the lock file.
     * Returns 0 if lock file doesn't exist or is invalid.
     */
    int getLockPid() const;

private:
    /**
     * Initialize instance directory and lock file.
     * Throws if another instance is already running with this UUID.
     */
    void initializeInstance();

    /**
     * Create the lock file with current process ID.
     */
    void createLockFile();

    /**
     * Check if a process with given PID is running.
     */
    static bool isProcessRunning(int pid);

    juce::Uuid nodeId;
    juce::File instanceDir;
    juce::File lockFile;
    bool cleaned = false;
};

} // namespace NetworkMidi
