/**
 * InstanceManager - Implementation
 */

#include "InstanceManager.h"
#include <iostream>
#include <stdexcept>

#if JUCE_MAC || JUCE_LINUX
#include <signal.h>
#include <unistd.h>
#endif

namespace NetworkMidi {

InstanceManager::InstanceManager(const juce::Uuid& nodeId)
    : nodeId(nodeId)
{
    initializeInstance();
}

InstanceManager::~InstanceManager()
{
    cleanup();
}

juce::File InstanceManager::getInstanceDirectory() const
{
    return instanceDir;
}

juce::File InstanceManager::getStateFile(const juce::String& name) const
{
    return instanceDir.getChildFile(name);
}

void InstanceManager::cleanup()
{
    if (cleaned) {
        return;
    }

    std::cout << "Cleaning up instance directory..." << std::endl;

    // Remove lock file
    if (lockFile.existsAsFile()) {
        if (lockFile.deleteFile()) {
            std::cout << "  Removed lock file: "
                      << lockFile.getFullPathName().toStdString() << std::endl;
        } else {
            std::cerr << "  Warning: Failed to remove lock file: "
                      << lockFile.getFullPathName().toStdString() << std::endl;
        }
    }

    // Remove instance directory (including any state files)
    if (instanceDir.exists()) {
        if (instanceDir.deleteRecursively()) {
            std::cout << "  Removed instance directory: "
                      << instanceDir.getFullPathName().toStdString() << std::endl;
        } else {
            std::cerr << "  Warning: Failed to remove instance directory: "
                      << instanceDir.getFullPathName().toStdString() << std::endl;
        }
    }

    cleaned = true;
}

bool InstanceManager::isLockStale() const
{
    if (!lockFile.existsAsFile()) {
        return false;
    }

    int pid = getLockPid();
    if (pid == 0) {
        // Invalid PID in lock file
        return true;
    }

    // Check if process is still running
    return !isProcessRunning(pid);
}

int InstanceManager::getLockPid() const
{
    if (!lockFile.existsAsFile()) {
        return 0;
    }

    juce::String pidStr = lockFile.loadFileAsString().trim();
    if (pidStr.isEmpty()) {
        return 0;
    }

    return pidStr.getIntValue();
}

void InstanceManager::initializeInstance()
{
    // Determine instance directory path
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::String dirName = "midi-network-" + nodeId.toString();
    instanceDir = tempDir.getChildFile(dirName);

    std::cout << "Initializing instance directory: "
              << instanceDir.getFullPathName().toStdString() << std::endl;

    // Check for existing lock file
    lockFile = instanceDir.getChildFile(".lock");

    if (lockFile.existsAsFile()) {
        if (isLockStale()) {
            std::cout << "  Found stale lock file (orphaned instance), cleaning up..."
                      << std::endl;

            // Clean up orphaned instance
            if (instanceDir.deleteRecursively()) {
                std::cout << "  Cleaned up orphaned instance" << std::endl;
            } else {
                std::cerr << "  Warning: Failed to clean up orphaned instance"
                          << std::endl;
            }
        } else {
            // Another instance is running with same UUID
            int pid = getLockPid();
            throw std::runtime_error(
                "Another instance is already running with UUID " +
                nodeId.toString().toStdString() +
                " (PID: " + std::to_string(pid) + ")"
            );
        }
    }

    // Create instance directory
    if (!instanceDir.exists()) {
        if (!instanceDir.createDirectory()) {
            throw std::runtime_error(
                "Failed to create instance directory: " +
                instanceDir.getFullPathName().toStdString()
            );
        }
        std::cout << "  Created instance directory" << std::endl;
    }

    // Create lock file
    createLockFile();
}

void InstanceManager::createLockFile()
{
#if JUCE_MAC || JUCE_LINUX
    int pid = getpid();
#else
    int pid = 0; // Windows: would need GetCurrentProcessId()
#endif

    juce::String pidStr = juce::String(pid);

    if (lockFile.replaceWithText(pidStr)) {
        std::cout << "  Created lock file with PID: " << pid << std::endl;
    } else {
        throw std::runtime_error(
            "Failed to create lock file: " +
            lockFile.getFullPathName().toStdString()
        );
    }
}

bool InstanceManager::isProcessRunning(int pid)
{
    if (pid <= 0) {
        return false;
    }

#if JUCE_MAC || JUCE_LINUX
    // Use kill(pid, 0) to check if process exists
    // Returns 0 if process exists, -1 if it doesn't
    return (kill(pid, 0) == 0);
#else
    // Windows: would need OpenProcess() and GetExitCodeProcess()
    // For now, assume process is running (conservative approach)
    return true;
#endif
}

} // namespace NetworkMidi
