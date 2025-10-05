/**
 * VirtualMidiPort.h
 *
 * Virtual MIDI port that wraps a remote device to appear as a local port.
 * Provides transparent network MIDI routing by implementing the
 * MidiPortInterface for remote devices.
 */

#pragma once

#include "MidiRouter.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <string>
#include <mutex>

namespace NetworkMidi {

/**
 * Virtual MIDI port representing a remote device
 *
 * Design:
 * - Implements MidiPortInterface for uniform local/remote handling
 * - Delegates sendMessage() to NetworkTransport
 * - Buffers received messages from network
 * - Thread-safe operations
 *
 * Usage:
 *   auto virtualPort = std::make_unique<VirtualMidiPort>(
 *       "Remote Piano", remoteNodeId, remoteDeviceId, transport);
 *
 *   virtualPort->sendMessage({0x90, 0x3C, 0x64}); // Note On
 *   auto messages = virtualPort->getMessages();   // Receive
 */
class VirtualMidiPort : public MidiPortInterface {
public:
    /**
     * Create virtual port for a remote device
     *
     * @param name          Device name (e.g., "studio-mac:IAC Bus 1")
     * @param ownerNode     UUID of the node owning the physical device
     * @param remoteDeviceId Device ID on the remote node
     * @param transport     Network transport for message transmission
     * @param isInputPort   True if this is a MIDI input port
     */
    VirtualMidiPort(const juce::String& name,
                    const juce::Uuid& ownerNode,
                    uint16_t remoteDeviceId,
                    NetworkTransport* transport,
                    bool isInputPort = false);

    virtual ~VirtualMidiPort();

    // MidiPortInterface implementation
    void sendMessage(const std::vector<uint8_t>& data) override;
    std::vector<std::vector<uint8_t>> getMessages() override;

    juce::String getName() const override;
    bool isInput() const override;
    bool isOutput() const override;

    // Virtual port specific methods
    juce::Uuid getOwnerNode() const;
    uint16_t getRemoteDeviceId() const;

    // Message reception (called by MidiRouter when network packets arrive)
    void onMessageReceived(const std::vector<uint8_t>& data);

    // Statistics
    uint64_t getSentMessageCount() const;
    uint64_t getReceivedMessageCount() const;
    void resetStatistics();

private:
    // Port identity
    juce::String portName;
    juce::Uuid ownerNodeId;
    uint16_t deviceId;
    bool inputPort;

    // Network transport
    NetworkTransport* networkTransport;

    // Received message buffer
    mutable std::mutex messageMutex;
    std::vector<std::vector<uint8_t>> receivedMessages;

    // Statistics
    mutable std::mutex statsMutex;
    uint64_t messagesSent;
    uint64_t messagesReceived;

    // Configuration
    static constexpr size_t maxBufferedMessages = 1000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualMidiPort)
};

/**
 * Factory for creating virtual MIDI ports
 *
 * Simplifies creation of virtual ports for remote devices
 */
class VirtualMidiPortFactory {
public:
    static std::unique_ptr<VirtualMidiPort> createForRemoteDevice(
        const MidiDevice& remoteDevice,
        NetworkTransport* transport);

    static std::unique_ptr<VirtualMidiPort> createInputPort(
        const juce::String& name,
        const juce::Uuid& ownerNode,
        uint16_t remoteDeviceId,
        NetworkTransport* transport);

    static std::unique_ptr<VirtualMidiPort> createOutputPort(
        const juce::String& name,
        const juce::Uuid& ownerNode,
        uint16_t remoteDeviceId,
        NetworkTransport* transport);
};

/**
 * Local MIDI port wrapper (for uniform interface)
 *
 * Wraps JUCE MidiInput/MidiOutput to implement MidiPortInterface
 */
class LocalMidiPort : public MidiPortInterface,
                      public juce::MidiInputCallback {
public:
    LocalMidiPort(const juce::String& name,
                  int portIndex,
                  bool isInputPort);

    virtual ~LocalMidiPort();

    // MidiPortInterface implementation
    void sendMessage(const std::vector<uint8_t>& data) override;
    std::vector<std::vector<uint8_t>> getMessages() override;

    juce::String getName() const override;
    bool isInput() const override;
    bool isOutput() const override;

    // JUCE MidiInputCallback
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;

    // Local port management
    bool open();
    void close();
    bool isOpen() const;

private:
    juce::String portName;
    int portIdx;
    bool inputPort;

    // JUCE MIDI port handles
    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;

    // Received message buffer (for input ports)
    mutable std::mutex messageMutex;
    std::vector<std::vector<uint8_t>> receivedMessages;

    // Configuration
    static constexpr size_t maxBufferedMessages = 1000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LocalMidiPort)
};

} // namespace NetworkMidi
