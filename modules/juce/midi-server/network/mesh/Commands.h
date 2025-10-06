/**
 * Commands.h
 *
 * Command hierarchy for SEDA architecture in NetworkConnection.
 * Provides type-safe commands for the connection worker thread.
 */

#pragma once

#include "NetworkConnection.h"
#include <juce_events/juce_events.h>
#include <memory>
#include <vector>

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
        GetState,
        GetRemoteNode,
        GetDevices,
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

//==============================================================================
// Query commands with synchronous response mechanism
// Uses juce::WaitableEvent to block caller until worker responds

struct GetStateQuery : Command {
    juce::WaitableEvent responseReady;
    NetworkConnection::State result{NetworkConnection::State::Disconnected};

    GetStateQuery() : Command(GetState) {}
};

struct GetRemoteNodeQuery : Command {
    juce::WaitableEvent responseReady;
    NodeInfo result;

    GetRemoteNodeQuery() : Command(GetRemoteNode) {}
};

struct GetDevicesQuery : Command {
    juce::WaitableEvent responseReady;
    std::vector<DeviceInfo> result;

    GetDevicesQuery() : Command(GetDevices) {}
};

} // namespace Commands
} // namespace NetworkMidi
