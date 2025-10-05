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
 * Represents a MIDI device (local or remote) in the network mesh
 */
struct MidiDevice {
    uint16_t id;                    // Global unique device ID
    juce::String name;              // Device name (e.g., "IAC Driver Bus 1")
    juce::String type;              // "input" or "output"
    bool isLocal;                   // True if physically connected to this node
    juce::Uuid ownerNode;           // Node UUID owning this device
    juce::String manufacturer;      // Device manufacturer (optional)

    MidiDevice() : id(0), isLocal(false) {}

    MidiDevice(uint16_t deviceId,
               const juce::String& deviceName,
               const juce::String& deviceType,
               bool local,
               const juce::Uuid& owner)
        : id(deviceId)
        , name(deviceName)
        , type(deviceType)
        , isLocal(local)
        , ownerNode(owner)
    {}

    bool operator==(const MidiDevice& other) const {
        return id == other.id;
    }
};

/**
 * Thread-safe registry for managing MIDI device lifecycle in the mesh
 *
 * Responsibilities:
 * - Track local devices (physically connected to this node)
 * - Track remote devices (from other mesh nodes)
 * - Assign global unique IDs to all devices
 * - Provide thread-safe device enumeration
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

    void removeRemoteDevice(uint16_t deviceId);
    void removeNodeDevices(const juce::Uuid& nodeId);

    // Device queries
    std::vector<MidiDevice> getAllDevices() const;
    std::vector<MidiDevice> getLocalDevices() const;
    std::vector<MidiDevice> getRemoteDevices() const;
    std::vector<MidiDevice> getNodeDevices(const juce::Uuid& nodeId) const;

    std::optional<MidiDevice> getDevice(uint16_t deviceId) const;

    // Statistics
    int getTotalDeviceCount() const;
    int getLocalDeviceCount() const;
    int getRemoteDeviceCount() const;
    int getNodeDeviceCount(const juce::Uuid& nodeId) const;

    // Device ID management
    uint16_t getNextAvailableId() const;
    bool isDeviceIdAvailable(uint16_t deviceId) const;

private:
    // Thread-safe device storage
    mutable std::mutex deviceMutex;
    std::map<uint16_t, MidiDevice> devices;

    // ID allocation tracking
    uint16_t nextDeviceId;

    // Helper methods
    void addDeviceInternal(const MidiDevice& device);
    void removeDeviceInternal(uint16_t deviceId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceRegistry)
};

} // namespace NetworkMidi
