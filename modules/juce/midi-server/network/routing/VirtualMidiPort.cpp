/**
 * VirtualMidiPort.cpp
 *
 * Implementation of virtual and local MIDI port wrappers
 */

#include "VirtualMidiPort.h"
#include <algorithm>

namespace NetworkMidi {

//==============================================================================
// VirtualMidiPort implementation

VirtualMidiPort::VirtualMidiPort(const juce::String& name,
                                 const juce::Uuid& ownerNode,
                                 uint16_t remoteDeviceId,
                                 NetworkTransport* transport,
                                 bool isInputPort)
    : portName(name)
    , ownerNodeId(ownerNode)
    , deviceId(remoteDeviceId)
    , inputPort(isInputPort)
    , networkTransport(transport)
    , messagesSent(0)
    , messagesReceived(0)
{
    if (!networkTransport) {
        throw std::invalid_argument("NetworkTransport cannot be null");
    }
}

VirtualMidiPort::~VirtualMidiPort()
{
    std::lock_guard<std::mutex> lock(messageMutex);
    receivedMessages.clear();
}

//==============================================================================
// MidiPortInterface implementation

void VirtualMidiPort::sendMessage(const std::vector<uint8_t>& data)
{
    if (data.empty()) {
        throw std::invalid_argument("Cannot send empty MIDI message");
    }

    if (!networkTransport) {
        throw std::runtime_error("Network transport not configured");
    }

    // Send via network
    networkTransport->sendMidiMessage(ownerNodeId, deviceId, data);

    // Update statistics
    std::lock_guard<std::mutex> lock(statsMutex);
    messagesSent++;
}

std::vector<std::vector<uint8_t>> VirtualMidiPort::getMessages()
{
    std::lock_guard<std::mutex> lock(messageMutex);

    std::vector<std::vector<uint8_t>> result;
    result.swap(receivedMessages);

    return result;
}

juce::String VirtualMidiPort::getName() const
{
    return portName;
}

bool VirtualMidiPort::isInput() const
{
    return inputPort;
}

bool VirtualMidiPort::isOutput() const
{
    return !inputPort;
}

//==============================================================================
// Virtual port specific methods

juce::Uuid VirtualMidiPort::getOwnerNode() const
{
    return ownerNodeId;
}

uint16_t VirtualMidiPort::getRemoteDeviceId() const
{
    return deviceId;
}

void VirtualMidiPort::onMessageReceived(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(messageMutex);

    // Limit buffer size
    if (receivedMessages.size() >= maxBufferedMessages) {
        // Drop oldest message
        receivedMessages.erase(receivedMessages.begin());
    }

    receivedMessages.push_back(data);

    // Update statistics
    {
        std::lock_guard<std::mutex> statsLock(statsMutex);
        messagesReceived++;
    }
}

//==============================================================================
// Statistics

uint64_t VirtualMidiPort::getSentMessageCount() const
{
    std::lock_guard<std::mutex> lock(statsMutex);
    return messagesSent;
}

uint64_t VirtualMidiPort::getReceivedMessageCount() const
{
    std::lock_guard<std::mutex> lock(statsMutex);
    return messagesReceived;
}

void VirtualMidiPort::resetStatistics()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    messagesSent = 0;
    messagesReceived = 0;
}

//==============================================================================
// VirtualMidiPortFactory implementation

std::unique_ptr<VirtualMidiPort>
VirtualMidiPortFactory::createForRemoteDevice(const MidiDevice& remoteDevice,
                                              NetworkTransport* transport)
{
    if (remoteDevice.isLocal) {
        throw std::invalid_argument("Cannot create virtual port for local device");
    }

    bool isInput = (remoteDevice.type == "input");

    return std::make_unique<VirtualMidiPort>(
        remoteDevice.name,
        remoteDevice.ownerNode,
        remoteDevice.id,
        transport,
        isInput);
}

std::unique_ptr<VirtualMidiPort>
VirtualMidiPortFactory::createInputPort(const juce::String& name,
                                        const juce::Uuid& ownerNode,
                                        uint16_t remoteDeviceId,
                                        NetworkTransport* transport)
{
    return std::make_unique<VirtualMidiPort>(
        name, ownerNode, remoteDeviceId, transport, true);
}

std::unique_ptr<VirtualMidiPort>
VirtualMidiPortFactory::createOutputPort(const juce::String& name,
                                         const juce::Uuid& ownerNode,
                                         uint16_t remoteDeviceId,
                                         NetworkTransport* transport)
{
    return std::make_unique<VirtualMidiPort>(
        name, ownerNode, remoteDeviceId, transport, false);
}

//==============================================================================
// LocalMidiPort implementation

LocalMidiPort::LocalMidiPort(const juce::String& name,
                             int portIndex,
                             bool isInputPort)
    : portName(name)
    , portIdx(portIndex)
    , inputPort(isInputPort)
{
}

LocalMidiPort::~LocalMidiPort()
{
    close();
}

//==============================================================================
// MidiPortInterface implementation

void LocalMidiPort::sendMessage(const std::vector<uint8_t>& data)
{
    if (data.empty()) {
        throw std::invalid_argument("Cannot send empty MIDI message");
    }

    if (inputPort) {
        throw std::runtime_error("Cannot send to input port");
    }

    if (!midiOutput || !isOpen()) {
        throw std::runtime_error("MIDI output port not open");
    }

    // Convert to JUCE MidiMessage
    juce::MidiMessage message(data.data(), static_cast<int>(data.size()));

    // Send via JUCE
    midiOutput->sendMessageNow(message);
}

std::vector<std::vector<uint8_t>> LocalMidiPort::getMessages()
{
    std::lock_guard<std::mutex> lock(messageMutex);

    std::vector<std::vector<uint8_t>> result;
    result.swap(receivedMessages);

    return result;
}

juce::String LocalMidiPort::getName() const
{
    return portName;
}

bool LocalMidiPort::isInput() const
{
    return inputPort;
}

bool LocalMidiPort::isOutput() const
{
    return !inputPort;
}

//==============================================================================
// JUCE MidiInputCallback

void LocalMidiPort::handleIncomingMidiMessage(juce::MidiInput* source,
                                              const juce::MidiMessage& message)
{
    std::lock_guard<std::mutex> lock(messageMutex);

    // Convert to byte vector
    std::vector<uint8_t> data(message.getRawData(),
                              message.getRawData() + message.getRawDataSize());

    // Limit buffer size
    if (receivedMessages.size() >= maxBufferedMessages) {
        // Drop oldest message
        receivedMessages.erase(receivedMessages.begin());
    }

    receivedMessages.push_back(std::move(data));
}

//==============================================================================
// Local port management

bool LocalMidiPort::open()
{
    if (inputPort) {
        // Open MIDI input
        auto inputDevices = juce::MidiInput::getAvailableDevices();

        if (portIdx < 0 || portIdx >= inputDevices.size()) {
            return false;
        }

        midiInput = juce::MidiInput::openDevice(
            inputDevices[portIdx].identifier, this);

        if (midiInput) {
            midiInput->start();
            return true;
        }
    } else {
        // Open MIDI output
        auto outputDevices = juce::MidiOutput::getAvailableDevices();

        if (portIdx < 0 || portIdx >= outputDevices.size()) {
            return false;
        }

        midiOutput = juce::MidiOutput::openDevice(
            outputDevices[portIdx].identifier);

        return midiOutput != nullptr;
    }

    return false;
}

void LocalMidiPort::close()
{
    if (midiInput) {
        midiInput->stop();
        midiInput.reset();
    }

    if (midiOutput) {
        midiOutput.reset();
    }
}

bool LocalMidiPort::isOpen() const
{
    if (inputPort) {
        return midiInput != nullptr;
    } else {
        return midiOutput != nullptr;
    }
}

} // namespace NetworkMidi
