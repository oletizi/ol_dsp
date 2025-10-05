/**
 * InstanceManagerTest.cpp
 *
 * Unit tests for InstanceManager
 * Tests: instance isolation, lock files, collision detection, cleanup
 *
 * Coverage Target: 80%+
 */

#include "network/core/InstanceManager.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

using namespace NetworkMidi;
using namespace testing;

class InstanceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        testUuid = juce::Uuid();
        cleanupTestDirectories();
    }

    void TearDown() override {
        cleanupTestDirectories();
    }

    void cleanupTestDirectories() {
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        juce::String dirName = "midi-network-" + testUuid.toString();
        juce::File testDir = tempDir.getChildFile(dirName);

        if (testDir.exists()) {
            testDir.deleteRecursively();
        }
    }

    juce::Uuid testUuid;
};

// Test instance directory creation
TEST_F(InstanceManagerTest, CreatesInstanceDirectory) {
    InstanceManager manager(testUuid);

    juce::File instanceDir = manager.getInstanceDirectory();
    EXPECT_TRUE(instanceDir.exists());
    EXPECT_TRUE(instanceDir.isDirectory());

    // Verify correct path
    juce::File expectedDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("midi-network-" + testUuid.toString());
    EXPECT_EQ(expectedDir.getFullPathName(), instanceDir.getFullPathName());
}

// Test lock file creation
TEST_F(InstanceManagerTest, CreatesLockFile) {
    InstanceManager manager(testUuid);

    juce::File instanceDir = manager.getInstanceDirectory();
    juce::File lockFile = instanceDir.getChildFile(".lock");

    EXPECT_TRUE(lockFile.existsAsFile());
}

// Test lock file contains PID
TEST_F(InstanceManagerTest, LockFileContainsPid) {
    InstanceManager manager(testUuid);

    int lockPid = manager.getLockPid();

#if JUCE_MAC || JUCE_LINUX
    EXPECT_GT(lockPid, 0);
    EXPECT_EQ(getpid(), lockPid);
#else
    // Windows: PID is 0 (not implemented yet)
    EXPECT_EQ(0, lockPid);
#endif
}

// Test duplicate instance detection
TEST_F(InstanceManagerTest, DetectsDuplicateInstance) {
    InstanceManager manager1(testUuid);

    // Attempt to create second instance with same UUID
    EXPECT_THROW({
        InstanceManager manager2(testUuid);
    }, std::runtime_error);
}

// Test cleanup on destruction
TEST_F(InstanceManagerTest, CleansUpOnDestruction) {
    juce::File instanceDir;
    juce::File lockFile;

    {
        InstanceManager manager(testUuid);
        instanceDir = manager.getInstanceDirectory();
        lockFile = instanceDir.getChildFile(".lock");

        EXPECT_TRUE(instanceDir.exists());
        EXPECT_TRUE(lockFile.existsAsFile());
    }

    // After destruction, directory and lock should be removed
    EXPECT_FALSE(lockFile.existsAsFile());
    EXPECT_FALSE(instanceDir.exists());
}

// Test manual cleanup
TEST_F(InstanceManagerTest, ManualCleanupRemovesResources) {
    InstanceManager manager(testUuid);

    juce::File instanceDir = manager.getInstanceDirectory();
    juce::File lockFile = instanceDir.getChildFile(".lock");

    manager.cleanup();

    EXPECT_FALSE(lockFile.existsAsFile());
    EXPECT_FALSE(instanceDir.exists());
}

// Test double cleanup (should be safe)
TEST_F(InstanceManagerTest, DoubleCleanupIsSafe) {
    InstanceManager manager(testUuid);

    manager.cleanup();
    EXPECT_NO_THROW({
        manager.cleanup();
    });
}

// Test state file creation
TEST_F(InstanceManagerTest, CreatesStateFiles) {
    InstanceManager manager(testUuid);

    juce::File stateFile = manager.getStateFile("test-state.json");

    EXPECT_FALSE(stateFile.existsAsFile());  // Not created until written
    EXPECT_EQ("test-state.json", stateFile.getFileName());
    EXPECT_TRUE(stateFile.getParentDirectory() == manager.getInstanceDirectory());
}

// Test multiple state files
TEST_F(InstanceManagerTest, HandlesMultipleStateFiles) {
    InstanceManager manager(testUuid);

    juce::File file1 = manager.getStateFile("config.json");
    juce::File file2 = manager.getStateFile("routes.json");
    juce::File file3 = manager.getStateFile("devices.json");

    EXPECT_NE(file1.getFullPathName(), file2.getFullPathName());
    EXPECT_NE(file2.getFullPathName(), file3.getFullPathName());

    // All in same instance directory
    EXPECT_EQ(file1.getParentDirectory().getFullPathName(),
              file2.getParentDirectory().getFullPathName());
}

// Test stale lock detection (mocked)
TEST_F(InstanceManagerTest, DetectsStaleLock) {
    // Create lock file with invalid PID
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File instanceDir = tempDir.getChildFile("midi-network-" + testUuid.toString());
    instanceDir.createDirectory();

    juce::File lockFile = instanceDir.getChildFile(".lock");
    lockFile.replaceWithText("99999999");  // Invalid/non-existent PID

    // Should detect stale lock and create new instance
    EXPECT_NO_THROW({
        InstanceManager manager(testUuid);

        // Lock should be replaced with current PID
        int newPid = manager.getLockPid();
#if JUCE_MAC || JUCE_LINUX
        EXPECT_EQ(getpid(), newPid);
#endif
    });
}

// Test orphaned instance cleanup
TEST_F(InstanceManagerTest, CleansUpOrphanedInstance) {
    // Create orphaned instance directory with stale lock
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File instanceDir = tempDir.getChildFile("midi-network-" + testUuid.toString());
    instanceDir.createDirectory();

    juce::File lockFile = instanceDir.getChildFile(".lock");
    juce::File stateFile = instanceDir.getChildFile("old-state.json");

    lockFile.replaceWithText("99999999");
    stateFile.replaceWithText("{\"test\": true}");

    EXPECT_TRUE(instanceDir.exists());
    EXPECT_TRUE(lockFile.existsAsFile());
    EXPECT_TRUE(stateFile.existsAsFile());

    // Creating new instance should clean up orphaned files
    InstanceManager manager(testUuid);

    // Old state file should be gone
    EXPECT_FALSE(stateFile.existsAsFile());

    // New lock should exist
    juce::File newLock = manager.getInstanceDirectory().getChildFile(".lock");
    EXPECT_TRUE(newLock.existsAsFile());
}

// Test empty PID handling
TEST_F(InstanceManagerTest, HandlesEmptyPidInLockFile) {
    // Create lock file with empty content
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File instanceDir = tempDir.getChildFile("midi-network-" + testUuid.toString());
    instanceDir.createDirectory();

    juce::File lockFile = instanceDir.getChildFile(".lock");
    lockFile.replaceWithText("");

    InstanceManager manager(testUuid);

    // Should have valid lock with current PID
    int lockPid = manager.getLockPid();
#if JUCE_MAC || JUCE_LINUX
    EXPECT_EQ(getpid(), lockPid);
#endif
}

// Test invalid PID string handling
TEST_F(InstanceManagerTest, HandlesInvalidPidString) {
    // Create lock file with non-numeric PID
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File instanceDir = tempDir.getChildFile("midi-network-" + testUuid.toString());
    instanceDir.createDirectory();

    juce::File lockFile = instanceDir.getChildFile(".lock");
    lockFile.replaceWithText("not-a-number");

    InstanceManager manager(testUuid);

    // Should treat as stale and create new lock
    int lockPid = manager.getLockPid();
#if JUCE_MAC || JUCE_LINUX
    EXPECT_EQ(getpid(), lockPid);
#endif
}

// Test non-existent lock file
TEST_F(InstanceManagerTest, HandlesNonExistentLockFile) {
    InstanceManager manager(testUuid);

    // Before cleanup, lock should exist
    EXPECT_FALSE(manager.isLockStale());
    EXPECT_GT(manager.getLockPid(), 0);

    // After cleanup, lock doesn't exist
    manager.cleanup();
    EXPECT_EQ(0, manager.getLockPid());
}

// Test instance directory name format
TEST_F(InstanceManagerTest, UsesCorrectDirectoryNameFormat) {
    InstanceManager manager(testUuid);

    juce::File instanceDir = manager.getInstanceDirectory();
    juce::String expectedName = "midi-network-" + testUuid.toString();

    EXPECT_EQ(expectedName, instanceDir.getFileName());
}

// Test different UUIDs create different directories
TEST_F(InstanceManagerTest, DifferentUuidsCreateDifferentDirectories) {
    juce::Uuid uuid1;
    juce::Uuid uuid2;

    InstanceManager manager1(uuid1);
    InstanceManager manager2(uuid2);

    juce::File dir1 = manager1.getInstanceDirectory();
    juce::File dir2 = manager2.getInstanceDirectory();

    EXPECT_NE(dir1.getFullPathName(), dir2.getFullPathName());

    // Cleanup
    manager1.cleanup();
    manager2.cleanup();
}

// Test concurrent access safety
TEST_F(InstanceManagerTest, PreventsConcurrentAccess) {
    InstanceManager manager1(testUuid);

    std::atomic<bool> exceptionThrown{false};
    std::thread thread([this, &exceptionThrown]() {
        try {
            InstanceManager manager2(testUuid);
        } catch (const std::runtime_error&) {
            exceptionThrown = true;
        }
    });

    thread.join();
    EXPECT_TRUE(exceptionThrown);
}

// Test instance recovery after crash simulation
TEST_F(InstanceManagerTest, RecoversAfterCrashSimulation) {
    // Create instance and don't cleanup (simulate crash)
    {
        InstanceManager* manager = new InstanceManager(testUuid);
        // Deliberately leak - don't delete (simulates crash)
        (void)manager;
    }

    // Lock file still exists with valid PID - should fail
    EXPECT_THROW({
        InstanceManager manager2(testUuid);
    }, std::runtime_error);

    // Clean up manually
    cleanupTestDirectories();
}
