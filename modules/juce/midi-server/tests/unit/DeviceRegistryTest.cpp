/**
 * DeviceRegistryTest.cpp
 *
 * Unit tests for DeviceRegistry
 * Tests: device add/remove, local/remote, lookups, thread safety
 *
 * Coverage Target: 80%+
 */

#include "network/routing/DeviceRegistry.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

using namespace NetworkMidi;
using namespace testing;

class DeviceRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry = std::make_unique<DeviceRegistry>();
        localNode = juce::Uuid::null();
        remoteNode1 = juce::Uuid();
        remoteNode2 = juce::Uuid();
    }

    std::unique_ptr<DeviceRegistry> registry;
    juce::Uuid localNode;
    juce::Uuid remoteNode1;
    juce::Uuid remoteNode2;
};

// Test add local device
TEST_F(DeviceRegistryTest, AddsLocalDevice) {
    registry->addLocalDevice(1, "Test Input", "input", "Manufacturer");

    auto device = registry->getDevice(1);

    ASSERT_TRUE(device.has_value());
    EXPECT_EQ(1, device->id);
    EXPECT_EQ("Test Input", device->name);
    EXPECT_EQ("input", device->type);
    EXPECT_TRUE(device->isLocal);
    EXPECT_EQ("Manufacturer", device->manufacturer);
}

// Test add remote device
TEST_F(DeviceRegistryTest, AddsRemoteDevice) {
    registry->addRemoteDevice(remoteNode1, 2, "Remote Output", "output", "Vendor");

    auto device = registry->getDevice(2);

    ASSERT_TRUE(device.has_value());
    EXPECT_EQ(2, device->id);
    EXPECT_EQ("Remote Output", device->name);
    EXPECT_EQ("output", device->type);
    EXPECT_FALSE(device->isLocal);
    EXPECT_EQ(remoteNode1, device->ownerNode);
    EXPECT_EQ("Vendor", device->manufacturer);
}

// Test remove local device
TEST_F(DeviceRegistryTest, RemovesLocalDevice) {
    registry->addLocalDevice(1, "Test Device", "input");

    registry->removeLocalDevice(1);

    auto device = registry->getDevice(1);
    EXPECT_FALSE(device.has_value());
}

// Test remove remote device
TEST_F(DeviceRegistryTest, RemovesRemoteDevice) {
    registry->addRemoteDevice(remoteNode1, 2, "Remote Device", "output");

    registry->removeRemoteDevice(2);

    auto device = registry->getDevice(2);
    EXPECT_FALSE(device.has_value());
}

// Test clear local devices
TEST_F(DeviceRegistryTest, ClearsLocalDevices) {
    registry->addLocalDevice(1, "Local 1", "input");
    registry->addLocalDevice(2, "Local 2", "output");
    registry->addRemoteDevice(remoteNode1, 3, "Remote 1", "input");

    registry->clearLocalDevices();

    EXPECT_FALSE(registry->getDevice(1).has_value());
    EXPECT_FALSE(registry->getDevice(2).has_value());
    EXPECT_TRUE(registry->getDevice(3).has_value());  // Remote should remain
}

// Test remove node devices
TEST_F(DeviceRegistryTest, RemovesNodeDevices) {
    registry->addRemoteDevice(remoteNode1, 1, "Node1 Device1", "input");
    registry->addRemoteDevice(remoteNode1, 2, "Node1 Device2", "output");
    registry->addRemoteDevice(remoteNode2, 3, "Node2 Device1", "input");

    registry->removeNodeDevices(remoteNode1);

    EXPECT_FALSE(registry->getDevice(1).has_value());
    EXPECT_FALSE(registry->getDevice(2).has_value());
    EXPECT_TRUE(registry->getDevice(3).has_value());  // Different node
}

// Test get all devices
TEST_F(DeviceRegistryTest, GetsAllDevices) {
    registry->addLocalDevice(1, "Local", "input");
    registry->addRemoteDevice(remoteNode1, 2, "Remote", "output");

    auto devices = registry->getAllDevices();

    EXPECT_EQ(2u, devices.size());
}

// Test get local devices
TEST_F(DeviceRegistryTest, GetsLocalDevices) {
    registry->addLocalDevice(1, "Local 1", "input");
    registry->addLocalDevice(2, "Local 2", "output");
    registry->addRemoteDevice(remoteNode1, 3, "Remote", "input");

    auto devices = registry->getLocalDevices();

    EXPECT_EQ(2u, devices.size());
    for (const auto& device : devices) {
        EXPECT_TRUE(device.isLocal);
    }
}

// Test get remote devices
TEST_F(DeviceRegistryTest, GetsRemoteDevices) {
    registry->addLocalDevice(1, "Local", "input");
    registry->addRemoteDevice(remoteNode1, 2, "Remote 1", "output");
    registry->addRemoteDevice(remoteNode2, 3, "Remote 2", "input");

    auto devices = registry->getRemoteDevices();

    EXPECT_EQ(2u, devices.size());
    for (const auto& device : devices) {
        EXPECT_FALSE(device.isLocal);
    }
}

// Test get node devices
TEST_F(DeviceRegistryTest, GetsNodeDevices) {
    registry->addRemoteDevice(remoteNode1, 1, "Node1 Device1", "input");
    registry->addRemoteDevice(remoteNode1, 2, "Node1 Device2", "output");
    registry->addRemoteDevice(remoteNode2, 3, "Node2 Device", "input");

    auto devices = registry->getNodeDevices(remoteNode1);

    EXPECT_EQ(2u, devices.size());
    for (const auto& device : devices) {
        EXPECT_EQ(remoteNode1, device.ownerNode);
    }
}

// Test get non-existent device
TEST_F(DeviceRegistryTest, GetNonExistentDevice) {
    auto device = registry->getDevice(999);

    EXPECT_FALSE(device.has_value());
}

// Test total device count
TEST_F(DeviceRegistryTest, GetsTotalDeviceCount) {
    registry->addLocalDevice(1, "Local", "input");
    registry->addRemoteDevice(remoteNode1, 2, "Remote", "output");

    EXPECT_EQ(2, registry->getTotalDeviceCount());
}

// Test local device count
TEST_F(DeviceRegistryTest, GetsLocalDeviceCount) {
    registry->addLocalDevice(1, "Local 1", "input");
    registry->addLocalDevice(2, "Local 2", "output");
    registry->addRemoteDevice(remoteNode1, 3, "Remote", "input");

    EXPECT_EQ(2, registry->getLocalDeviceCount());
}

// Test remote device count
TEST_F(DeviceRegistryTest, GetsRemoteDeviceCount) {
    registry->addLocalDevice(1, "Local", "input");
    registry->addRemoteDevice(remoteNode1, 2, "Remote 1", "output");
    registry->addRemoteDevice(remoteNode2, 3, "Remote 2", "input");

    EXPECT_EQ(2, registry->getRemoteDeviceCount());
}

// Test node device count
TEST_F(DeviceRegistryTest, GetsNodeDeviceCount) {
    registry->addRemoteDevice(remoteNode1, 1, "Node1 Device1", "input");
    registry->addRemoteDevice(remoteNode1, 2, "Node1 Device2", "output");
    registry->addRemoteDevice(remoteNode2, 3, "Node2 Device", "input");

    EXPECT_EQ(2, registry->getNodeDeviceCount(remoteNode1));
    EXPECT_EQ(1, registry->getNodeDeviceCount(remoteNode2));
}

// Test next available ID
TEST_F(DeviceRegistryTest, GetsNextAvailableId) {
    uint16_t id1 = registry->getNextAvailableId();
    registry->addLocalDevice(id1, "Device 1", "input");

    uint16_t id2 = registry->getNextAvailableId();
    EXPECT_NE(id1, id2);
}

// Test is device ID available
TEST_F(DeviceRegistryTest, ChecksDeviceIdAvailability) {
    EXPECT_TRUE(registry->isDeviceIdAvailable(100));

    registry->addLocalDevice(100, "Device", "input");

    EXPECT_FALSE(registry->isDeviceIdAvailable(100));
}

// Test update existing device
TEST_F(DeviceRegistryTest, UpdatesExistingDevice) {
    registry->addLocalDevice(1, "Original Name", "input", "Vendor A");

    registry->addLocalDevice(1, "Updated Name", "output", "Vendor B");

    auto device = registry->getDevice(1);

    ASSERT_TRUE(device.has_value());
    EXPECT_EQ("Updated Name", device->name);
    EXPECT_EQ("output", device->type);
    EXPECT_EQ("Vendor B", device->manufacturer);
}

// Test empty registry
TEST_F(DeviceRegistryTest, EmptyRegistry) {
    EXPECT_EQ(0, registry->getTotalDeviceCount());
    EXPECT_EQ(0, registry->getLocalDeviceCount());
    EXPECT_EQ(0, registry->getRemoteDeviceCount());

    auto devices = registry->getAllDevices();
    EXPECT_TRUE(devices.empty());
}

// Test concurrent device addition
TEST_F(DeviceRegistryTest, HandlesConcurrentAddition) {
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            registry->addLocalDevice(i, "Device " + juce::String(i), "input");
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(10, registry->getTotalDeviceCount());
}

// Test concurrent device removal
TEST_F(DeviceRegistryTest, HandlesConcurrentRemoval) {
    // Add devices
    for (int i = 0; i < 10; ++i) {
        registry->addLocalDevice(i, "Device " + juce::String(i), "input");
    }

    // Remove concurrently
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            registry->removeLocalDevice(i);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(0, registry->getTotalDeviceCount());
}

// Test concurrent read/write
TEST_F(DeviceRegistryTest, HandlesConcurrentReadWrite) {
    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    // Writer thread
    threads.emplace_back([this, &running]() {
        int id = 0;
        while (running) {
            registry->addLocalDevice(id++, "Device", "input");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Reader threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([this, &running]() {
            while (running) {
                auto devices = registry->getAllDevices();
                auto count = registry->getTotalDeviceCount();
                (void)devices;
                (void)count;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    for (auto& thread : threads) {
        thread.join();
    }
}

// Test device equality
TEST_F(DeviceRegistryTest, DeviceEquality) {
    MidiDevice device1(1, "Device", "input", true, localNode);
    MidiDevice device2(1, "Different Name", "output", false, remoteNode1);
    MidiDevice device3(2, "Device", "input", true, localNode);

    EXPECT_TRUE(device1 == device2);  // Same ID
    EXPECT_FALSE(device1 == device3);  // Different ID
}

// Test device type validation
TEST_F(DeviceRegistryTest, StoresDeviceTypes) {
    registry->addLocalDevice(1, "Input Device", "input");
    registry->addLocalDevice(2, "Output Device", "output");

    auto inputDevice = registry->getDevice(1);
    auto outputDevice = registry->getDevice(2);

    ASSERT_TRUE(inputDevice.has_value());
    ASSERT_TRUE(outputDevice.has_value());
    EXPECT_EQ("input", inputDevice->type);
    EXPECT_EQ("output", outputDevice->type);
}

// Test manufacturer field
TEST_F(DeviceRegistryTest, StoresManufacturer) {
    registry->addLocalDevice(1, "Device", "input", "ACME Corp");

    auto device = registry->getDevice(1);

    ASSERT_TRUE(device.has_value());
    EXPECT_EQ("ACME Corp", device->manufacturer);
}

// Test empty manufacturer
TEST_F(DeviceRegistryTest, HandlesEmptyManufacturer) {
    registry->addLocalDevice(1, "Device", "input", "");

    auto device = registry->getDevice(1);

    ASSERT_TRUE(device.has_value());
    EXPECT_TRUE(device->manufacturer.isEmpty());
}

// Test ID allocation increment
TEST_F(DeviceRegistryTest, IncrementsIdAllocation) {
    uint16_t id1 = registry->getNextAvailableId();
    registry->addLocalDevice(id1, "Device 1", "input");

    uint16_t id2 = registry->getNextAvailableId();
    registry->addLocalDevice(id2, "Device 2", "input");

    EXPECT_EQ(id1 + 1, id2);
}

// Test skip used IDs
TEST_F(DeviceRegistryTest, SkipsUsedIds) {
    registry->addLocalDevice(10, "Device", "input");

    uint16_t nextId = registry->getNextAvailableId();

    EXPECT_NE(10, nextId);
}
