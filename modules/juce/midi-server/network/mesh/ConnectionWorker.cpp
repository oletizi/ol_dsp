/**
 * ConnectionWorker.cpp
 *
 * Implementation of worker thread for NetworkConnection SEDA architecture.
 */

#include "ConnectionWorker.h"
#include <juce_core/juce_core.h>
#include <sstream>

namespace NetworkMidi {

ConnectionWorker::ConnectionWorker(NetworkConnectionQueue& queue,
                                   const NodeInfo& remoteNode,
                                   std::function<void(NetworkConnection::State, NetworkConnection::State)> stateCallback,
                                   std::function<void(const std::vector<DeviceInfo>&)> devicesCallback,
                                   std::function<void(const MidiMessage&)> midiCallback,
                                   std::function<void(const juce::String&)> errorCallback)
    : juce::Thread("ConnectionWorker")
    , commandQueue(queue)
    , remoteNodeInfo(remoteNode)
    , onStateChanged(std::move(stateCallback))
    , onDevicesReceived(std::move(devicesCallback))
    , onMidiMessageReceived(std::move(midiCallback))
    , onError(std::move(errorCallback))
{
}

ConnectionWorker::~ConnectionWorker()
{
    // Ensure thread stops before destruction
    stopThread(2000);  // 2 second timeout
}

void ConnectionWorker::run()
{
    juce::Logger::writeToLog("ConnectionWorker: Thread started for " + remoteNodeInfo.name);

    while (!threadShouldExit()) {
        // Wait for command with 100ms timeout
        // Timeout allows periodic checks of threadShouldExit()
        auto cmd = commandQueue.waitAndPop(100);

        if (cmd) {
            processCommand(std::move(cmd));
        }
    }

    juce::Logger::writeToLog("ConnectionWorker: Thread exiting");
}

void ConnectionWorker::processCommand(std::unique_ptr<Commands::Command> cmd)
{
    jassert(cmd != nullptr);

    switch (cmd->type) {
        case Commands::Command::Connect:
            handleConnectCommand();
            break;

        case Commands::Command::Disconnect:
            handleDisconnectCommand();
            break;

        case Commands::Command::CheckHeartbeat:
            handleCheckHeartbeatCommand();
            break;

        case Commands::Command::NotifyHeartbeat:
            handleNotifyHeartbeatCommand();
            break;

        case Commands::Command::SendMidi:
            handleSendMidiCommand(static_cast<Commands::SendMidiCommand*>(cmd.get()));
            break;

        case Commands::Command::GetState:
            handleGetStateQuery(static_cast<Commands::GetStateQuery*>(cmd.get()));
            break;

        case Commands::Command::GetRemoteNode:
            handleGetRemoteNodeQuery(static_cast<Commands::GetRemoteNodeQuery*>(cmd.get()));
            break;

        case Commands::Command::GetDevices:
            handleGetDevicesQuery(static_cast<Commands::GetDevicesQuery*>(cmd.get()));
            break;

        case Commands::Command::Shutdown:
            juce::Logger::writeToLog("ConnectionWorker: Shutdown command received");
            signalThreadShouldExit();
            break;

        default:
            juce::Logger::writeToLog("ConnectionWorker: Unknown command type: " +
                                    juce::String(static_cast<int>(cmd->type)));
            jassertfalse;
            break;
    }
}

//==============================================================================
// Command handlers

void ConnectionWorker::handleConnectCommand()
{
    // Check if already connecting or connected
    if (currentState == NetworkConnection::State::Connecting ||
        currentState == NetworkConnection::State::Connected) {
        juce::Logger::writeToLog("ConnectionWorker: Already connecting/connected, ignoring connect command");
        return;
    }

    setState(NetworkConnection::State::Connecting);

    try {
        // Initialize HTTP client
        std::string baseUrl = "http://" + remoteNodeInfo.ipAddress.toStdString() + ":" +
                             std::to_string(remoteNodeInfo.httpPort);
        httpClient = std::make_unique<httplib::Client>(baseUrl);
        httpClient->set_connection_timeout(5, 0);  // 5 seconds
        httpClient->set_read_timeout(5, 0);

        // Initialize and bind UDP socket to any available port
        udpSocket = std::make_unique<juce::DatagramSocket>();
        if (!udpSocket->bindToPort(0)) {  // 0 = any available port
            throw std::runtime_error("Failed to bind UDP socket");
        }

        // Get local UDP endpoint
        int localUdpPort = udpSocket->getBoundPort();
        localUdpEndpoint = juce::IPAddress::getLocalAddress().toString() + ":" +
                          juce::String(localUdpPort);

        // Prepare handshake JSON request body
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

        // Parse response - extract UDP endpoint
        std::string responseBody = res->body;
        size_t udpPos = responseBody.find("\"udp_endpoint\":\"");
        if (udpPos != std::string::npos) {
            udpPos += 16;
            size_t endPos = responseBody.find("\"", udpPos);
            remoteUdpEndpoint = juce::String(responseBody.substr(udpPos, endPos - udpPos));
        } else {
            throw std::runtime_error("Handshake response missing udp_endpoint");
        }

        // Parse devices array (basic parsing for now)
        size_t devicesPos = responseBody.find("\"devices\":[");
        if (devicesPos != std::string::npos) {
            // TODO: Implement proper JSON parsing for device list
            juce::Logger::writeToLog("Received device list from " + remoteNodeInfo.name);

            // Notify callback if devices were received
            if (onDevicesReceived) {
                onDevicesReceived(remoteDevices);
            }
        }

        // Connection successful
        running = true;
        setState(NetworkConnection::State::Connected);

        // Initialize heartbeat timestamp
        lastHeartbeatTime = juce::Time::getCurrentTime().toMilliseconds();
        updateSnapshots();

        juce::Logger::writeToLog("Successfully connected to " + remoteNodeInfo.name +
                                " (UDP: " + remoteUdpEndpoint + ")");

    } catch (const std::exception& e) {
        juce::Logger::writeToLog("Handshake failed: " + juce::String(e.what()));

        setState(NetworkConnection::State::Failed);
        updateSnapshots();

        if (onError) {
            onError(juce::String("Handshake failed: ") + e.what());
        }
    }
}

void ConnectionWorker::handleDisconnectCommand()
{
    juce::Logger::writeToLog("ConnectionWorker: Disconnecting from " + remoteNodeInfo.name);

    // Set running flag to false
    running = false;

    // Shutdown and reset network resources
    if (udpSocket) {
        udpSocket->shutdown();
        udpSocket.reset();
    }

    httpClient.reset();

    // Clear received messages
    receivedMessages.clear();

    // Reset endpoints
    localUdpEndpoint.clear();
    remoteUdpEndpoint.clear();

    // Update state
    setState(NetworkConnection::State::Disconnected);
    updateSnapshots();
}

void ConnectionWorker::handleCheckHeartbeatCommand()
{
    if (currentState != NetworkConnection::State::Connected) {
        return;
    }

    // Calculate time since last heartbeat
    juce::int64 currentTime = juce::Time::getCurrentTime().toMilliseconds();
    juce::int64 timeSinceHeartbeat = currentTime - lastHeartbeatTime;

    if (timeSinceHeartbeat > HEARTBEAT_TIMEOUT_MS) {
        juce::Logger::writeToLog("ConnectionWorker: Heartbeat timeout for " +
                                remoteNodeInfo.name + " (" +
                                juce::String(timeSinceHeartbeat) + "ms)");

        setState(NetworkConnection::State::Failed);
        updateSnapshots();

        if (onError) {
            onError("Heartbeat timeout - connection lost");
        }
    }
}

void ConnectionWorker::handleNotifyHeartbeatCommand()
{
    // Update heartbeat timestamp
    lastHeartbeatTime = juce::Time::getCurrentTime().toMilliseconds();
    heartbeatSnapshot.store(lastHeartbeatTime);

    juce::Logger::writeToLog("ConnectionWorker: Heartbeat received from " + remoteNodeInfo.name);
}

void ConnectionWorker::handleSendMidiCommand(Commands::SendMidiCommand* cmd)
{
    jassert(cmd != nullptr);

    // Validate connection state
    if (currentState != NetworkConnection::State::Connected) {
        juce::Logger::writeToLog("ConnectionWorker: Cannot send MIDI - not connected");
        return;
    }

    // Validate data
    if (cmd->data.empty()) {
        juce::Logger::writeToLog("ConnectionWorker: Cannot send empty MIDI message");
        return;
    }

    // TODO: Implement actual UDP MIDI packet transmission
    // This will use MidiPacket protocol when implemented
    juce::Logger::writeToLog("ConnectionWorker: Sending MIDI message - deviceId=" +
                            juce::String(cmd->deviceId) +
                            ", bytes=" + juce::String((int)cmd->data.size()));
}

void ConnectionWorker::handleGetStateQuery(Commands::GetStateQuery* query)
{
    jassert(query != nullptr);

    // Return current state
    query->result = currentState;

    // Signal response ready
    query->responseReady.signal();
}

void ConnectionWorker::handleGetRemoteNodeQuery(Commands::GetRemoteNodeQuery* query)
{
    jassert(query != nullptr);

    // Return remote node info
    query->result = remoteNodeInfo;

    // Signal response ready
    query->responseReady.signal();
}

void ConnectionWorker::handleGetDevicesQuery(Commands::GetDevicesQuery* query)
{
    jassert(query != nullptr);

    // Return device list
    query->result = remoteDevices;

    // Signal response ready
    query->responseReady.signal();
}

//==============================================================================
// Helper methods

void ConnectionWorker::setState(NetworkConnection::State newState)
{
    NetworkConnection::State oldState = currentState;

    // Check if state actually changed
    if (oldState == newState) {
        return;
    }

    // Update state
    currentState = newState;
    stateSnapshot.store(newState);

    // Log state transition
    juce::Logger::writeToLog("ConnectionWorker: State changed: " +
                            toString(oldState) + " -> " + toString(newState));

    // Invoke callback if set
    if (onStateChanged) {
        onStateChanged(oldState, newState);
    }
}

void ConnectionWorker::updateSnapshots()
{
    stateSnapshot.store(currentState);
    heartbeatSnapshot.store(lastHeartbeatTime);
}

void ConnectionWorker::handleUdpPacket(const void* data, int size,
                                      const juce::String& sender, int port)
{
    // TODO: Implement UDP packet handling with MidiPacket protocol
    // For now, just log receipt
    juce::Logger::writeToLog("ConnectionWorker: Received UDP packet: " +
                            juce::String(size) + " bytes from " +
                            sender + ":" + juce::String(port));

    // Update heartbeat timestamp
    lastHeartbeatTime = juce::Time::getCurrentTime().toMilliseconds();
    heartbeatSnapshot.store(lastHeartbeatTime);
}

} // namespace NetworkMidi
