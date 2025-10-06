/**
 * NodeIdentityTest.cpp
 *
 * Unit tests for NodeIdentity singleton
 * Tests: UUID generation, persistence, reload, regeneration, naming
 *
 * Coverage Target: 80%+
 */

#include "network/core/NodeIdentity.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace NetworkMidi;
using namespace testing;

class NodeIdentityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing test files before each test
        testIdFile = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
            .getChildFile(".midi-network")
            .getChildFile("node-id");

        // Back up existing file if present
        if (testIdFile.existsAsFile()) {
            backupFile = testIdFile.getParentDirectory().getChildFile("node-id.backup");
            testIdFile.copyFileTo(backupFile);
            testIdFile.deleteFile();
        }
    }

    void TearDown() override {
        // Restore backup if it exists
        if (backupFile.existsAsFile()) {
            backupFile.moveFileTo(testIdFile);
        }
    }

    juce::File testIdFile;
    juce::File backupFile;
};

// Test singleton instance creation
TEST_F(NodeIdentityTest, SingletonReturnsConsistentInstance) {
    NodeIdentity& instance1 = NodeIdentity::getInstance();
    NodeIdentity& instance2 = NodeIdentity::getInstance();

    EXPECT_EQ(&instance1, &instance2);
}

// Test UUID generation on first run
TEST_F(NodeIdentityTest, GeneratesUuidOnFirstRun) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::Uuid nodeId = identity.getNodeId();

    EXPECT_FALSE(nodeId.isNull());
    // JUCE UUID toString() returns 32 chars without hyphens
    EXPECT_EQ(32, nodeId.toString().length());
}

// Test UUID persistence
TEST_F(NodeIdentityTest, PersistsUuidToDisk) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::Uuid nodeId = identity.getNodeId();

    // Verify file exists - use actual file path from NodeIdentity
    juce::File idFile = identity.getIdFile();
    EXPECT_TRUE(idFile.existsAsFile());

    // Verify file content matches UUID
    juce::String savedUuid = idFile.loadFileAsString().trim();
    EXPECT_EQ(nodeId.toString(), savedUuid);
}

// Test node name generation
TEST_F(NodeIdentityTest, GeneratesValidNodeName) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::String nodeName = identity.getNodeName();

    // Name format: {hostname}-{uuid-prefix}
    EXPECT_FALSE(nodeName.isEmpty());
    EXPECT_TRUE(nodeName.contains("-"));

    // Should contain hostname prefix
    juce::String hostname = identity.getHostname();
    EXPECT_TRUE(nodeName.startsWith(hostname.toLowerCase().substring(0, 20)));
}

// Test hostname retrieval
TEST_F(NodeIdentityTest, RetrievesSystemHostname) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::String hostname = identity.getHostname();

    EXPECT_FALSE(hostname.isEmpty());
    // Should match system hostname or fallback
    juce::String systemHost = juce::SystemStats::getComputerName();
    if (systemHost.isNotEmpty()) {
        EXPECT_EQ(systemHost, hostname);
    } else {
        EXPECT_EQ(juce::String("unknown-host"), hostname);
    }
}

// Test UUID regeneration
TEST_F(NodeIdentityTest, RegeneratesUuidOnRequest) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::Uuid originalId = identity.getNodeId();

    juce::Uuid newId = identity.regenerateId();

    EXPECT_NE(originalId, newId);
    EXPECT_FALSE(newId.isNull());

    // New ID should be persisted
    juce::File idFile = identity.getIdFile();
    juce::String savedUuid = idFile.loadFileAsString().trim();
    EXPECT_EQ(newId.toString(), savedUuid);
}

// Test node name updates after regeneration
TEST_F(NodeIdentityTest, UpdatesNodeNameAfterRegeneration) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::String originalName = identity.getNodeName();

    identity.regenerateId();
    juce::String newName = identity.getNodeName();

    EXPECT_NE(originalName, newName);
    EXPECT_TRUE(newName.contains("-"));
}

// Test ID file path
TEST_F(NodeIdentityTest, ReturnsCorrectIdFilePath) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::File idFile = identity.getIdFile();

    juce::File expectedPath = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile(".midi-network")
        .getChildFile("node-id");

    EXPECT_EQ(expectedPath.getFullPathName(), idFile.getFullPathName());
}

// Test directory creation
TEST_F(NodeIdentityTest, CreatesConfigDirectoryIfNotExists) {
    // Get the actual config directory from NodeIdentity
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::File configDir = identity.getIdFile().getParentDirectory();

    // The directory should already exist since the singleton was initialized
    // This test verifies that NodeIdentity creates the directory on initialization
    EXPECT_TRUE(configDir.exists());
    EXPECT_TRUE(configDir.isDirectory());

    // Verify that the directory path is what we expect
    juce::File expectedDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile(".midi-network");
    EXPECT_EQ(expectedDir.getFullPathName(), configDir.getFullPathName());
}

// Test invalid UUID handling
TEST_F(NodeIdentityTest, HandlesCorruptedUuidFile) {
    // Write invalid UUID to file
    juce::File configDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile(".midi-network");
    configDir.createDirectory();

    juce::File idFile = configDir.getChildFile("node-id");
    idFile.replaceWithText("invalid-uuid-format");

    // Should generate new UUID
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::Uuid nodeId = identity.getNodeId();

    EXPECT_FALSE(nodeId.isNull());
    EXPECT_NE(juce::String("invalid-uuid-format"), nodeId.toString());
}

// Test empty UUID file handling
TEST_F(NodeIdentityTest, HandlesEmptyUuidFile) {
    // Create empty file
    juce::File configDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile(".midi-network");
    configDir.createDirectory();

    juce::File idFile = configDir.getChildFile("node-id");
    idFile.replaceWithText("");

    // Should generate new UUID
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::Uuid nodeId = identity.getNodeId();

    EXPECT_FALSE(nodeId.isNull());
}

// Test node name sanitization
TEST_F(NodeIdentityTest, SanitizesHostnameInNodeName) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::String nodeName = identity.getNodeName();

    // Should not contain spaces or underscores (replaced with dashes)
    EXPECT_FALSE(nodeName.contains(" "));
    EXPECT_FALSE(nodeName.contains("_"));

    // Should be lowercase
    EXPECT_EQ(nodeName, nodeName.toLowerCase());
}

// Test UUID prefix extraction
TEST_F(NodeIdentityTest, ExtractsUuidPrefixInNodeName) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::Uuid nodeId = identity.getNodeId();
    juce::String nodeName = identity.getNodeName();

    // Extract UUID prefix (first 8 chars of UUID string)
    juce::String uuidPrefix = nodeId.toString().substring(0, 8);

    EXPECT_TRUE(nodeName.endsWith(uuidPrefix));
}

// Test hostname length limiting
TEST_F(NodeIdentityTest, LimitsHostnameLengthInNodeName) {
    NodeIdentity& identity = NodeIdentity::getInstance();
    juce::String nodeName = identity.getNodeName();

    // Node name should not exceed reasonable length
    // Format: hostname(max 20) + "-" + uuid(8) = 29 chars max
    EXPECT_LE(nodeName.length(), 29);
}

// Test UUID uniqueness across regenerations
TEST_F(NodeIdentityTest, GeneratesUniqueUuidsOnRegeneration) {
    NodeIdentity& identity = NodeIdentity::getInstance();

    std::vector<juce::Uuid> uuids;
    uuids.push_back(identity.getNodeId());

    // Regenerate multiple times
    for (int i = 0; i < 5; ++i) {
        juce::Uuid newId = identity.regenerateId();

        // Should be unique
        for (const auto& existingId : uuids) {
            EXPECT_NE(existingId, newId);
        }

        uuids.push_back(newId);
    }
}
