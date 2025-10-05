/**
 * DeviceRegistry.cpp
 *
 * Implementation of thread-safe MIDI device registry
 */

#include "DeviceRegistry.h"
#include <algorithm>

namespace NetworkMidi {

DeviceRegistry::DeviceRegistry()
    : nextDeviceId(0)
{
}

DeviceRegistry::~DeviceRegistry()
{
    std::lock_guard<std::mutex> lock(deviceMutex);
    devices.clear();
}

//==============================================================================
// Local device management

void DeviceRegistry::addLocalDevice(uint16_t deviceId,
                                    const juce::String& name,
                                    const juce::String& type,
                                    const juce::String& manufacturer)
{
    MidiDevice device(deviceId, name, type, true, juce::Uuid::null());
    device.manufacturer = manufacturer;
    addDeviceInternal(device);
}

void DeviceRegistry::removeLocalDevice(uint16_t deviceId)
{
    removeDeviceInternal(deviceId);
}

void DeviceRegistry::clearLocalDevices()
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    // Remove all local devices
    auto it = devices.begin();
    while (it != devices.end()) {
        if (it->second.isLocal) {
            it = devices.erase(it);
        } else {
            ++it;
        }
    }
}

//==============================================================================
// Remote device management

void DeviceRegistry::addRemoteDevice(const juce::Uuid& nodeId,
                                     uint16_t deviceId,
                                     const juce::String& name,
                                     const juce::String& type,
                                     const juce::String& manufacturer)
{
    MidiDevice device(deviceId, name, type, false, nodeId);
    device.manufacturer = manufacturer;
    addDeviceInternal(device);
}

void DeviceRegistry::removeRemoteDevice(uint16_t deviceId)
{
    removeDeviceInternal(deviceId);
}

void DeviceRegistry::removeNodeDevices(const juce::Uuid& nodeId)
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    // Remove all devices owned by specified node
    auto it = devices.begin();
    while (it != devices.end()) {
        if (!it->second.isLocal && it->second.ownerNode == nodeId) {
            it = devices.erase(it);
        } else {
            ++it;
        }
    }
}

//==============================================================================
// Device queries

std::vector<MidiDevice> DeviceRegistry::getAllDevices() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    std::vector<MidiDevice> result;
    result.reserve(devices.size());

    for (const auto& pair : devices) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<MidiDevice> DeviceRegistry::getLocalDevices() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    std::vector<MidiDevice> result;

    for (const auto& pair : devices) {
        if (pair.second.isLocal) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<MidiDevice> DeviceRegistry::getRemoteDevices() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    std::vector<MidiDevice> result;

    for (const auto& pair : devices) {
        if (!pair.second.isLocal) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<MidiDevice> DeviceRegistry::getNodeDevices(const juce::Uuid& nodeId) const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    std::vector<MidiDevice> result;

    for (const auto& pair : devices) {
        if (!pair.second.isLocal && pair.second.ownerNode == nodeId) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::optional<MidiDevice> DeviceRegistry::getDevice(uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    auto it = devices.find(deviceId);
    if (it != devices.end()) {
        return it->second;
    }

    return std::nullopt;
}

//==============================================================================
// Statistics

int DeviceRegistry::getTotalDeviceCount() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);
    return static_cast<int>(devices.size());
}

int DeviceRegistry::getLocalDeviceCount() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    int count = 0;
    for (const auto& pair : devices) {
        if (pair.second.isLocal) {
            ++count;
        }
    }

    return count;
}

int DeviceRegistry::getRemoteDeviceCount() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    int count = 0;
    for (const auto& pair : devices) {
        if (!pair.second.isLocal) {
            ++count;
        }
    }

    return count;
}

int DeviceRegistry::getNodeDeviceCount(const juce::Uuid& nodeId) const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    int count = 0;
    for (const auto& pair : devices) {
        if (!pair.second.isLocal && pair.second.ownerNode == nodeId) {
            ++count;
        }
    }

    return count;
}

//==============================================================================
// Device ID management

uint16_t DeviceRegistry::getNextAvailableId() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    // Find first available ID
    uint16_t candidateId = nextDeviceId;

    while (devices.find(candidateId) != devices.end()) {
        candidateId++;
        if (candidateId == 0) {
            throw std::runtime_error("Device ID space exhausted");
        }
    }

    return candidateId;
}

bool DeviceRegistry::isDeviceIdAvailable(uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(deviceMutex);
    return devices.find(deviceId) == devices.end();
}

//==============================================================================
// Private helper methods

void DeviceRegistry::addDeviceInternal(const MidiDevice& device)
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    // Check for duplicate ID
    auto it = devices.find(device.id);
    if (it != devices.end()) {
        // Update existing device
        it->second = device;
    } else {
        // Add new device
        devices[device.id] = device;

        // Update next ID if this ID is >= current next
        if (device.id >= nextDeviceId) {
            nextDeviceId = device.id + 1;
        }
    }
}

void DeviceRegistry::removeDeviceInternal(uint16_t deviceId)
{
    std::lock_guard<std::mutex> lock(deviceMutex);
    devices.erase(deviceId);
}

} // namespace NetworkMidi
