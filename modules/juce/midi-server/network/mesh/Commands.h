/**
 * Commands.h
 *
 * Command hierarchy for SEDA architecture in NetworkConnection.
 * Provides type-safe commands for the connection worker thread.
 */

#pragma once

#include "NetworkConnection.h"
#include "../core/MidiPacket.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace NetworkMidi {
namespace Commands {

//==============================================================================
/**
 * Base command class for all NetworkConnection commands.
 * Uses polymorphism for type-safe command dispatch.
 */
struct Command {
    enum Type {
        Connect,
        Disconnect,
        CheckHeartbeat,
        NotifyHeartbeat,
        SendMidi,
        SendPacket,         // NEW: Phase 4 - send full packet with context
        GetState,
        GetRemoteNode,
        GetDevices,
        GetHeartbeat,  // New command for heartbeat timing query
        Shutdown
    };

    Type type;
    virtual ~Command() = default;

protected:
    explicit Command(Type t) : type(t) {}
};

//==============================================================================
// Simple commands (no parameters)

struct ConnectCommand : Command {
    ConnectCommand() : Command(Connect) {}
};

struct DisconnectCommand : Command {
    DisconnectCommand() : Command(Disconnect) {}
};

struct CheckHeartbeatCommand : Command {
    CheckHeartbeatCommand() : Command(CheckHeartbeat) {}
};

struct NotifyHeartbeatCommand : Command {
    NotifyHeartbeatCommand() : Command(NotifyHeartbeat) {}
};

struct ShutdownCommand : Command {
    ShutdownCommand() : Command(Shutdown) {}
};

//==============================================================================
// Commands with parameters

struct SendMidiCommand : Command {
    uint16_t deviceId;
    std::vector<uint8_t> data;

    SendMidiCommand(uint16_t id, std::vector<uint8_t> msgData)
        : Command(SendMidi), deviceId(id), data(std::move(msgData)) {}
};

/**
 * Phase 4: Send full MidiPacket with forwarding context.
 * Replaces legacy SendMidiCommand for context-aware routing.
 */
struct SendPacketCommand : Command {
    MidiPacket packet;

    explicit SendPacketCommand(const MidiPacket& pkt)
        : Command(SendPacket), packet(pkt) {}
};

//==============================================================================
// Query commands with synchronous response mechanism
// Uses std::condition_variable for reliable cross-platform synchronization

struct GetStateQuery : Command {
    std::atomic<bool> ready{false};
    NetworkConnection::State result{NetworkConnection::State::Disconnected};

    GetStateQuery() : Command(GetState) {}

    void signal() {
        ready.store(true);
    }

    bool wait(int timeoutMs) {
        // WORKAROUND: Use polling instead of condition_variable to avoid macOS libc++ bug
        auto startTime = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeoutMs);

        while (std::chrono::steady_clock::now() - startTime < timeout) {
            if (ready.load()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
};

struct GetRemoteNodeQuery : Command {
    std::atomic<bool> ready{false};
    NodeInfo result;

    GetRemoteNodeQuery() : Command(GetRemoteNode) {}

    void signal() {
        ready.store(true);
    }

    bool wait(int timeoutMs) {
        // WORKAROUND: Use polling instead of condition_variable to avoid macOS libc++ bug
        auto startTime = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeoutMs);

        while (std::chrono::steady_clock::now() - startTime < timeout) {
            if (ready.load()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
};

struct GetDevicesQuery : Command {
    std::atomic<bool> ready{false};
    std::vector<DeviceInfo> result;

    GetDevicesQuery() : Command(GetDevices) {}

    void signal() {
        ready.store(true);
    }

    bool wait(int timeoutMs) {
        // WORKAROUND: Use polling instead of condition_variable to avoid macOS libc++ bug
        auto startTime = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeoutMs);

        while (std::chrono::steady_clock::now() - startTime < timeout) {
            if (ready.load()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
};

/**
 * Query command for retrieving time since last heartbeat.
 * Provides accurate timing information from worker thread.
 */
struct GetHeartbeatQuery : Command {
    std::atomic<bool> ready{false};
    int64_t timeSinceLastHeartbeat{0};

    GetHeartbeatQuery() : Command(GetHeartbeat) {}

    void signal() {
        ready.store(true);
    }

    bool wait(int timeoutMs) {
        // WORKAROUND: Use polling instead of condition_variable to avoid macOS libc++ bug
        auto startTime = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeoutMs);

        while (std::chrono::steady_clock::now() - startTime < timeout) {
            if (ready.load()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
};

} // namespace Commands
} // namespace NetworkMidi
