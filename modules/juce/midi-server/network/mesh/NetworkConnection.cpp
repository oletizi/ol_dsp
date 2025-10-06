/**
 * NetworkConnection.cpp
 *
 * Implementation of NetworkConnection class for peer-to-peer MIDI node connections.
 */

#include "NetworkConnection.h"
#include "../../httplib.h"

#include <sstream>

namespace NetworkMidi {

//==============================================================================
NetworkConnection::NetworkConnection(const NodeInfo& remoteNode)
    : remoteNodeInfo(remoteNode)
{
    if (!remoteNode.isValid()) {
        throw std::invalid_argument("Invalid NodeInfo provided to NetworkConnection");
    }

    // Initialize HTTP client
    std::string baseUrl = "http://" + remoteNode.ipAddress.toStdString() + ":" +
                         std::to_string(remoteNode.httpPort);
    httpClient = std::make_unique<httplib::Client>(baseUrl);
    httpClient->set_connection_timeout(5, 0);  // 5 seconds
    httpClient->set_read_timeout(5, 0);

    // Initialize UDP socket
    udpSocket = std::make_unique<juce::DatagramSocket>();
}

//==============================================================================
NetworkConnection::~NetworkConnection()
{
    disconnect();
}

//==============================================================================
void NetworkConnection::connect()
{
    std::lock_guard<std::mutex> lock(stateMutex);

    if (currentState != State::Disconnected) {
        juce::Logger::writeToLog("NetworkConnection::connect() called in invalid state: " +
                                toString(currentState));
        return;
    }

    setState(State::Connecting);

    // Perform handshake in background to avoid blocking
    juce::Thread::launch([this]() {
        performHandshake();
    });
}

//==============================================================================
void NetworkConnection::disconnect()
{
    std::lock_guard<std::mutex> lock(stateMutex);

    running = false;

    if (udpSocket) {
        udpSocket->shutdown();
    }

    httpClient.reset();
    udpSocket.reset();

    if (currentState != State::Disconnected) {
        setState(State::Disconnected);
    }
}

//==============================================================================
NetworkConnection::State NetworkConnection::getState() const
{
    return currentState.load();
}

//==============================================================================
NodeInfo NetworkConnection::getRemoteNode() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return remoteNodeInfo;
}

//==============================================================================
std::vector<DeviceInfo> NetworkConnection::getRemoteDevices() const
{
    // No mutex needed: remoteDevices is only written once during handshake
    // and this is a read-only copy operation
    return remoteDevices;
}

//==============================================================================
void NetworkConnection::sendMidiMessage(uint16_t deviceId,
                                       const std::vector<uint8_t>& data)
{
    if (currentState != State::Connected) {
        throw std::runtime_error("Cannot send MIDI message: not connected");
    }

    if (data.empty()) {
        throw std::invalid_argument("Cannot send empty MIDI message");
    }

    // TODO: Implement actual UDP MIDI packet transmission
    // For now, this is a placeholder that will be implemented with MidiPacket
    juce::Logger::writeToLog("NetworkConnection::sendMidiMessage() - "
                            "deviceId=" + juce::String(deviceId) +
                            ", bytes=" + juce::String((int)data.size()));
}

//==============================================================================
std::vector<MidiMessage> NetworkConnection::getReceivedMessages()
{
    std::lock_guard<std::mutex> lock(messageMutex);
    std::vector<MidiMessage> result = std::move(receivedMessages);
    receivedMessages.clear();
    return result;
}

//==============================================================================
int64_t NetworkConnection::getTimeSinceLastHeartbeat() const
{
    std::lock_guard<std::mutex> lock(heartbeatMutex);
    return juce::Time::getCurrentTime().toMilliseconds() - lastHeartbeatTime;
}

//==============================================================================
bool NetworkConnection::isAlive() const
{
    return getTimeSinceLastHeartbeat() < HEARTBEAT_TIMEOUT_MS;
}

//==============================================================================
void NetworkConnection::checkHeartbeat()
{
    if (currentState != State::Connected) {
        return;
    }

    if (!isAlive()) {
        juce::Logger::writeToLog("NetworkConnection: Heartbeat timeout for " +
                                remoteNodeInfo.name);

        std::lock_guard<std::mutex> lock(stateMutex);
        setState(State::Failed);

        if (onError) {
            onError("Heartbeat timeout - connection lost");
        }
    }
}

//==============================================================================
// Private methods
//==============================================================================

void NetworkConnection::performHandshake()
{
    try {
        // Bind UDP socket to any available port
        if (!udpSocket->bindToPort(0)) {  // 0 = any available port
            throw std::runtime_error("Failed to bind UDP socket");
        }

        int localUdpPort = udpSocket->getBoundPort();
        localUdpEndpoint = juce::IPAddress::getLocalAddress().toString() + ":" +
                          juce::String(localUdpPort);

        // Prepare handshake request
        std::ostringstream requestBody;
        requestBody << "{"
                   << "\"node_id\":\"" << juce::Uuid().toString().toStdString() << "\","
                   << "\"node_name\":\"local-node\","  // TODO: Get from NodeIdentity
                   << "\"udp_endpoint\":\"" << localUdpEndpoint.toStdString() << "\","
                   << "\"version\":\"1.0\""
                   << "}";

        // Send handshake request
        auto res = httpClient->Post("/network/handshake",
                                   requestBody.str(),
                                   "application/json");

        if (!res) {
            throw std::runtime_error("Failed to send handshake request: " +
                                   httplib::to_string(res.error()));
        }

        if (res->status != 200) {
            throw std::runtime_error("Handshake failed with HTTP " +
                                   std::to_string(res->status));
        }

        // Parse response (simple JSON parsing for MVP)
        std::string responseBody = res->body;

        // Extract UDP endpoint
        size_t udpPos = responseBody.find("\"udp_endpoint\":\"");
        if (udpPos != std::string::npos) {
            udpPos += 16;
            size_t endPos = responseBody.find("\"", udpPos);
            remoteUdpEndpoint = juce::String(responseBody.substr(udpPos, endPos - udpPos));
        } else {
            throw std::runtime_error("Handshake response missing udp_endpoint");
        }

        // Extract devices array (basic parsing)
        size_t devicesPos = responseBody.find("\"devices\":[");
        if (devicesPos != std::string::npos) {
            // TODO: Implement proper JSON parsing for device list
            // For now, we'll just note that devices were received
            juce::Logger::writeToLog("Received device list from " + remoteNodeInfo.name);

            // Notify callback
            if (onDevicesReceived) {
                onDevicesReceived(remoteDevices);
            }
        }

        // Handshake successful
        running = true;

        {
            std::lock_guard<std::mutex> lock(stateMutex);
            setState(State::Connected);
        }

        // Initialize heartbeat
        {
            std::lock_guard<std::mutex> lock(heartbeatMutex);
            lastHeartbeatTime = juce::Time::getCurrentTime().toMilliseconds();
        }

        juce::Logger::writeToLog("Successfully connected to " + remoteNodeInfo.name +
                                " (UDP: " + remoteUdpEndpoint + ")");

    } catch (const std::exception& e) {
        juce::Logger::writeToLog("Handshake failed: " + juce::String(e.what()));

        std::lock_guard<std::mutex> lock(stateMutex);
        setState(State::Failed);

        if (onError) {
            onError(juce::String("Handshake failed: ") + e.what());
        }
    }
}

//==============================================================================
void NetworkConnection::setState(State newState)
{
    State oldState = currentState.load();

    if (oldState == newState) {
        return;
    }

    currentState = newState;

    juce::Logger::writeToLog("NetworkConnection state changed: " +
                            toString(oldState) + " -> " + toString(newState));

    if (onStateChanged) {
        onStateChanged(oldState, newState);
    }
}

//==============================================================================
void NetworkConnection::handleUdpPacket(const void* data, int size,
                                       const juce::String& senderAddress,
                                       int senderPort)
{
    // TODO: Implement UDP packet handling with MidiPacket protocol
    // For now, just log receipt
    juce::Logger::writeToLog("Received UDP packet: " + juce::String(size) +
                            " bytes from " + senderAddress + ":" +
                            juce::String(senderPort));

    // Update heartbeat timestamp
    notifyHeartbeatReceived();
}

//==============================================================================
void NetworkConnection::sendHeartbeat()
{
    if (currentState != State::Connected) {
        return;
    }

    // TODO: Implement actual heartbeat packet transmission
    // For now, this is a placeholder
    juce::Logger::writeToLog("Sending heartbeat to " + remoteNodeInfo.name);
}

//==============================================================================
void NetworkConnection::notifyHeartbeatReceived()
{
    std::lock_guard<std::mutex> lock(heartbeatMutex);
    lastHeartbeatTime = juce::Time::getCurrentTime().toMilliseconds();
}

} // namespace NetworkMidi
