/**
 * DeviceRegistry.h
 *
 * Thread-safe registry for tracking both local and remote MIDI devices
 * in the network mesh. Manages device lifecycle and provides unified
 * device enumeration across the mesh.
 */

#pragma once

#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <mutex>
#include <optional>

namespace NetworkMidi {

/**
 * Composite key for uniquely identifying devices across the mesh
 * Uses (ownerNode, deviceId) pair to avoid ID conflicts between nodes
 */
struct DeviceKey {
    juce::Uuid ownerNode;  // Null UUID for local devices
    uint16_t deviceId;

    DeviceKey() : deviceId(0) {}

    DeviceKey(const juce::Uuid& owner, uint16_t id)
        : ownerNode(owner), deviceId(id) {}

    bool isLocal() const {
        return ownerNode.isNull();
    }

    bool operator<(const DeviceKey& other) const {
        if (ownerNode != other.ownerNode)
            return ownerNode < other.ownerNode;
        return deviceId < other.deviceId;
    }

    bool operator==(const DeviceKey& other) const {
        return ownerNode == other.ownerNode && deviceId == other.deviceId;
    }

    bool operator!=(const DeviceKey& other) const {
        return !(*this == other);
    }
};

/**
 * Represents a MIDI device (local or remote) in the network mesh
 */
struct MidiDevice {
    DeviceKey key;                  // Composite key (ownerNode, deviceId)
    juce::String name;              // Device name (e.g., "IAC Driver Bus 1")
    juce::String type;              // "input" or "output"
    juce::String manufacturer;      // Device manufacturer (optional)

    MidiDevice() {}

    MidiDevice(const juce::Uuid& owner,
               uint16_t deviceId,
               const juce::String& deviceName,
               const juce::String& deviceType)
        : key(owner, deviceId)
        , name(deviceName)
        , type(deviceType)
    {}

    // Convenience accessors
    bool isLocal() const { return key.isLocal(); }
    uint16_t id() const { return key.deviceId; }
    const juce::Uuid& ownerNode() const { return key.ownerNode; }

    bool operator==(const MidiDevice& other) const {
        return key == other.key;
    }
};

/**
 * Thread-safe registry for managing MIDI device lifecycle in the mesh
 *
 * Design (Phase 1: Device ID Namespacing):
 * - Uses composite keys (ownerNode, deviceId) to prevent ID conflicts
 * - Local devices have ownerNode == Uuid::null()
 * - Remote devices have ownerNode set to owning node's UUID
 * - Fast O(log n) lookup by composite key
 * - Backward-compatible APIs for local-only lookups
 * - Handle node disconnection (cleanup remote devices)
 */
class DeviceRegistry {
public:
    DeviceRegistry();
    ~DeviceRegistry();

    // Local device management
    void addLocalDevice(uint16_t deviceId,
                        const juce::String& name,
                        const juce::String& type,
                        const juce::String& manufacturer = "");

    void removeLocalDevice(uint16_t deviceId);
    void clearLocalDevices();

    // Remote device management
    void addRemoteDevice(const juce::Uuid& nodeId,
                         uint16_t deviceId,
                         const juce::String& name,
                         const juce::String& type,
                         const juce::String& manufacturer = "");

    void removeRemoteDevice(const juce::Uuid& nodeId, uint16_t deviceId);
    void removeNodeDevices(const juce::Uuid& nodeId);

    // Device queries (namespaced with composite keys)
    std::optional<MidiDevice> getDevice(const juce::Uuid& ownerNode, uint16_t deviceId) const;
    std::optional<MidiDevice> getLocalDevice(uint16_t deviceId) const;
    std::vector<MidiDevice> getAllDevices() const;
    std::vector<MidiDevice> getLocalDevices() const;
    std::vector<MidiDevice> getRemoteDevices() const;
    std::vector<MidiDevice> getNodeDevices(const juce::Uuid& nodeId) const;

    // Device checks (namespaced)
    bool hasDevice(const juce::Uuid& ownerNode, uint16_t deviceId) const;
    bool hasLocalDevice(uint16_t deviceId) const;

    // Statistics
    int getTotalDeviceCount() const;
    int getLocalDeviceCount() const;
    int getRemoteDeviceCount() const;
    int getNodeDeviceCount(const juce::Uuid& nodeId) const;

    // Device ID management
    uint16_t getNextAvailableId() const;
    bool isDeviceIdAvailable(uint16_t deviceId) const;

private:
    // Thread-safe device storage (now using composite keys)
    mutable std::mutex deviceMutex;
    std::map<DeviceKey, MidiDevice> devices;

    // ID allocation tracking
    uint16_t nextDeviceId;

    // Helper methods
    void addDeviceInternal(const MidiDevice& device);
    void removeDeviceInternal(const DeviceKey& key);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceRegistry)
};

} // namespace NetworkMidi
