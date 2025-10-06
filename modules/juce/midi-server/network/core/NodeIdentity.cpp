/**
 * NodeIdentity - Implementation
 */

#include "NodeIdentity.h"
#include <iostream>
#include <cstring>
#include <cstdlib>

namespace NetworkMidi {

// Default constructor - generates fresh UUID (no persistence)
NodeIdentity::NodeIdentity()
{
    // Generate fresh UUID for this instance
    nodeId = juce::Uuid();

    // Get system hostname
    hostname = juce::SystemStats::getComputerName();
    if (hostname.isEmpty()) {
        hostname = "unknown-host";
    }

    nodeName = generateNodeName(nodeId);

    std::cout << "Node Identity initialized (ephemeral):" << std::endl;
    std::cout << "  UUID: " << nodeId.toString().toStdString() << std::endl;
    std::cout << "  Name: " << nodeName.toStdString() << std::endl;
    std::cout << "  Hostname: " << hostname.toStdString() << std::endl;
}

// Constructor with persistence support
NodeIdentity::NodeIdentity(const juce::String& configPath)
{
    if (configPath.isEmpty()) {
        // No persistence - same as default constructor
        nodeId = juce::Uuid();
    } else {
        // Use persistent storage
        juce::File configDir(configPath);

        // Create directory if it doesn't exist
        if (!configDir.exists()) {
            configDir.createDirectory();
        }

        idFile = configDir.getChildFile("node-id");
        nodeId = loadOrCreateId();
        std::cout << "Loaded existing node ID from: " << idFile.getFullPathName() << std::endl;
    }

    // Get system hostname
    hostname = juce::SystemStats::getComputerName();
    if (hostname.isEmpty()) {
        hostname = "unknown-host";
    }

    nodeName = generateNodeName(nodeId);

    std::cout << "Node Identity initialized (persistent):" << std::endl;
    std::cout << "  UUID: " << nodeId.toString().toStdString() << std::endl;
    std::cout << "  Name: " << nodeName.toStdString() << std::endl;
    std::cout << "  Hostname: " << hostname.toStdString() << std::endl;
}

juce::Uuid NodeIdentity::getNodeId() const
{
    return nodeId;
}

juce::String NodeIdentity::getNodeName() const
{
    return nodeName;
}

juce::String NodeIdentity::getHostname() const
{
    return hostname;
}

juce::File NodeIdentity::getIdFile() const
{
    return idFile;
}

juce::Uuid NodeIdentity::regenerateId()
{
    std::cout << "Regenerating node ID..." << std::endl;

    juce::Uuid newId;
    saveId(newId);

    nodeId = newId;
    nodeName = generateNodeName(newId);

    std::cout << "  New UUID: " << nodeId.toString().toStdString() << std::endl;
    std::cout << "  New Name: " << nodeName.toStdString() << std::endl;

    return nodeId;
}

juce::Uuid NodeIdentity::loadOrCreateId()
{
    if (idFile.existsAsFile()) {
        // Load existing UUID
        juce::String uuidStr = idFile.loadFileAsString().trim();

        if (uuidStr.isNotEmpty()) {
            juce::Uuid loadedId(uuidStr);

            // Validate UUID (JUCE's Uuid default constructor creates null UUID)
            if (!loadedId.isNull()) {
                std::cout << "Loaded existing node ID from: "
                          << idFile.getFullPathName().toStdString() << std::endl;
                return loadedId;
            }
        }

        std::cout << "Invalid UUID in file, generating new one..." << std::endl;
    } else {
        std::cout << "No existing node ID found, creating new one..." << std::endl;
    }

    // Create new UUID
    juce::Uuid newId;
    saveId(newId);

    return newId;
}

void NodeIdentity::saveId(const juce::Uuid& uuid)
{
    juce::String uuidStr = uuid.toString();

    if (idFile.replaceWithText(uuidStr)) {
        std::cout << "Saved node ID to: "
                  << idFile.getFullPathName().toStdString() << std::endl;
    } else {
        std::cerr << "Warning: Failed to save node ID to: "
                  << idFile.getFullPathName().toStdString() << std::endl;
    }
}

juce::String NodeIdentity::generateNodeName(const juce::Uuid& uuid) const
{
    // Extract first 8 characters of UUID (without dashes)
    juce::String uuidStr = uuid.toString();
    juce::String uuidPrefix = uuidStr.substring(0, 8);

    // Clean hostname (remove spaces, make lowercase, limit length)
    juce::String cleanHost = hostname.toLowerCase()
                                    .replace(" ", "-")
                                    .replace("_", "-")
                                    .substring(0, 20);

    return cleanHost + "-" + uuidPrefix;
}

} // namespace NetworkMidi
