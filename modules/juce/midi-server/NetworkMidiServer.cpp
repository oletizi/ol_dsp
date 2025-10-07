/**
 * Network MIDI Server - Zero-configuration network MIDI mesh
 *
 * Provides HTTP API for MIDI operations and supports auto-discovery
 * for multi-node network MIDI mesh topology.
 *
 * Uses cpp-httplib for robust HTTP server implementation.
 */

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include "httplib.h"
#include "network/core/NodeIdentity.h"
#include "network/core/InstanceManager.h"
#include "network/discovery/ServiceDiscovery.h"
#include "network/mesh/MeshManager.h"
#include "network/routing/MidiRouter.h"
#include "network/routing/DeviceRegistry.h"
#include "network/routing/RoutingTable.h"
#include "network/transport/UdpMidiTransport.h"

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include <string>
#include <thread>
#include <sstream>

//==============================================================================
// Simple JSON builder for responses
class JsonBuilder
{
public:
    JsonBuilder& startObject() {
        if (!firstItem) ss << ",";
        ss << "{";
        firstItem = true;
        return *this;
    }
    JsonBuilder& endObject() {
        ss << "}";
        firstItem = false;  // Next key/item in parent needs comma
        return *this;
    }
    JsonBuilder& startArray() {
        if (!firstItem) ss << ",";
        ss << "[";
        firstItem = true;
        return *this;
    }
    JsonBuilder& endArray() {
        ss << "]";
        firstItem = false;  // Next item in parent needs comma
        return *this;
    }

    JsonBuilder& key(const std::string& k) {
        if (!firstItem) ss << ",";
        ss << "\"" << k << "\":";
        firstItem = true;  // Next value (not key) should not have comma
        return *this;
    }

    JsonBuilder& value(const std::string& v) {
        ss << "\"" << v << "\"";
        firstItem = false;  // Next key needs comma
        return *this;
    }

    JsonBuilder& value(bool b) {
        ss << (b ? "true" : "false");
        firstItem = false;  // Next key needs comma
        return *this;
    }

    JsonBuilder& value(int i) {
        ss << i;
        firstItem = false;  // Next key needs comma
        return *this;
    }

    JsonBuilder& arrayValue(const std::string& v) {
        if (!firstItem) ss << ",";
        ss << "\"" << v << "\"";
        firstItem = false;
        return *this;
    }

    JsonBuilder& arrayValue(int i) {
        if (!firstItem) ss << ",";
        ss << i;
        firstItem = false;
        return *this;
    }

    std::string toString() { return ss.str(); }

private:
    std::stringstream ss;
    bool firstItem = true;
};

//==============================================================================
// JUCE MIDI port wrapper implementing MidiPortInterface
class JuceMidiPort : public NetworkMidi::MidiPortInterface
{
public:
    JuceMidiPort(const juce::String& deviceName, bool isInputPort)
        : name(deviceName), inputPort(isInputPort) {}

    void sendMessage(const std::vector<uint8_t>& data) override {
        if (!output || data.empty()) return;

        if (data[0] == 0xF0) {
            // SysEx message
            if (data.back() != 0xF7 || data.size() <= 2) {
                std::cerr << "Warning: Invalid SysEx message\n";
                return;
            }
            output->sendMessageNow(
                juce::MidiMessage::createSysExMessage(
                    data.data() + 1,
                    (int)data.size() - 2
                )
            );
        } else if (data.size() >= 1 && data.size() <= 3) {
            output->sendMessageNow(
                juce::MidiMessage(data.data(), (int)data.size())
            );
        }
    }

    std::vector<std::vector<uint8_t>> getMessages() override {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::vector<std::vector<uint8_t>> result;
        while (!messageQueue.empty()) {
            result.push_back(messageQueue.front());
            messageQueue.pop();
        }
        return result;
    }

    juce::String getName() const override { return name; }
    bool isInput() const override { return inputPort; }
    bool isOutput() const override { return !inputPort; }

    void setMidiInput(std::unique_ptr<juce::MidiInput> in) {
        input = std::move(in);
    }

    void setMidiOutput(std::unique_ptr<juce::MidiOutput> out) {
        output = std::move(out);
    }

    void queueMessage(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(queueMutex);
        messageQueue.push(data);
    }

private:
    juce::String name;
    bool inputPort;
    std::unique_ptr<juce::MidiInput> input;
    std::unique_ptr<juce::MidiOutput> output;
    std::queue<std::vector<uint8_t>> messageQueue;
    std::mutex queueMutex;
};

//==============================================================================
// Network transport adapter for MidiRouter
class NetworkTransportAdapter : public NetworkMidi::NetworkTransport
{
public:
    NetworkTransportAdapter(NetworkMidi::UdpMidiTransport& transport)
        : udpTransport(transport) {}

    void sendMidiMessage(const juce::Uuid& destNode,
                        uint16_t deviceId,
                        const std::vector<uint8_t>& midiData) override {
        // Get destination node info from mesh manager
        if (meshManager) {
            auto nodeInfo = meshManager->getNodeInfo(destNode);
            if (nodeInfo.isValid()) {
                udpTransport.sendMessage(
                    destNode,
                    nodeInfo.ipAddress,
                    nodeInfo.udpPort,
                    deviceId,
                    midiData
                );
            }
        }
    }

    void setMeshManager(NetworkMidi::MeshManager* manager) {
        meshManager = manager;
    }

private:
    NetworkMidi::UdpMidiTransport& udpTransport;
    NetworkMidi::MeshManager* meshManager = nullptr;
};

//==============================================================================
// Helper to convert between NodeInfo types (ServiceDiscovery vs NetworkMidi)
NetworkMidi::NodeInfo convertToMeshNodeInfo(const ::NodeInfo& discoveryNode) {
    NetworkMidi::NodeInfo meshNode;
    meshNode.uuid = discoveryNode.uuid;
    meshNode.name = discoveryNode.name;
    meshNode.hostname = discoveryNode.hostname;
    meshNode.ipAddress = discoveryNode.ipAddress;
    meshNode.httpPort = discoveryNode.httpPort;
    meshNode.udpPort = discoveryNode.udpPort;
    meshNode.version = discoveryNode.version;
    meshNode.deviceCount = discoveryNode.deviceCount;
    return meshNode;
}

//==============================================================================
// Network MIDI Server class with full mesh integration
class NetworkMidiServer : public juce::MidiInputCallback
{
public:
    NetworkMidiServer(const NetworkMidi::NodeIdentity& nodeIdentity, int port = 0)
        : identity(nodeIdentity), requestedPort(port), actualPort(0), udpPort(0) {}

    ~NetworkMidiServer() {
        stopServer();
    }

    int getActualPort() const {
        return actualPort;
    }

    int getUdpPort() const {
        return udpPort;
    }

    void startServer() {
        // 1. Create UDP transport (auto-assign port)
        udpTransport = std::make_unique<NetworkMidi::UdpMidiTransport>(0);
        udpTransport->setNodeId(identity.getNodeId());
        udpTransport->start();
        udpPort = udpTransport->getPort();

        std::cout << "UDP transport started on port " << udpPort << std::endl;

        // 2. Create routing infrastructure
        deviceRegistry = std::make_unique<NetworkMidi::DeviceRegistry>();
        routingTable = std::make_unique<NetworkMidi::RoutingTable>();

        // 3. Create MIDI router BEFORE registering devices
        midiRouter = std::make_unique<NetworkMidi::MidiRouter>(*deviceRegistry, *routingTable);
        networkAdapter = std::make_unique<NetworkTransportAdapter>(*udpTransport);
        midiRouter->setNetworkTransport(networkAdapter.get());

        // 4. Enumerate and register local MIDI devices (now that router exists)
        registerLocalMidiDevices();

        // Set up UDP packet reception callback
        udpTransport->onPacketReceived = [this](const NetworkMidi::MidiPacket& packet,
                                                const juce::String& /*sourceAddr*/,
                                                int /*sourcePort*/) {
            handleNetworkPacket(packet);
        };

        // 5. Create HTTP server
        server = std::make_unique<httplib::Server>();
        setupRoutes();

        // Start HTTP server in a separate thread
        serverThread = std::thread([this]() {
            if (requestedPort == 0) {
                actualPort = server->bind_to_any_port("0.0.0.0");
                if (actualPort < 0) {
                    std::cerr << "Failed to bind to any port" << std::endl;
                    return;
                }
            } else {
                if (!server->bind_to_port("0.0.0.0", requestedPort)) {
                    std::cerr << "Failed to bind to port " << requestedPort << std::endl;
                    return;
                }
                actualPort = requestedPort;
            }

            std::cout << "HTTP Server bound to port " << actualPort << std::endl;
            server->listen_after_bind();
        });

        // Wait for HTTP server to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 6. Create service discovery (advertise this node)
        int deviceCount = deviceRegistry->getLocalDeviceCount();
        serviceDiscovery = std::make_unique<ServiceDiscovery>(
            identity.getNodeId(),
            identity.getNodeName(),
            actualPort,
            udpPort,
            deviceCount
        );

        // Set up discovery callbacks
        setupDiscoveryCallbacks();

        serviceDiscovery->advertise();
        std::cout << "Started mDNS advertising" << std::endl;

        // 7. Create mesh manager (handle peer connections)
        meshManager = std::make_unique<NetworkMidi::MeshManager>(
            identity.getNodeId(),
            actualPort,
            udpPort
        );

        // Wire mesh callbacks
        setupMeshCallbacks();

        meshManager->start();
        std::cout << "Mesh manager started" << std::endl;

        // Connect network adapter to mesh manager
        networkAdapter->setMeshManager(meshManager.get());

        // 8. NOW start MIDI inputs (after everything is fully initialized)
        startMidiInputs();
    }

    void stopServer() {
        // Stop MIDI inputs first
        for (auto& input : midiInputs) {
            if (input) {
                input->stop();
            }
        }

        // Stop mesh and discovery
        if (meshManager) {
            meshManager->stop();
        }
        if (serviceDiscovery) {
            serviceDiscovery->stopAdvertising();
            serviceDiscovery->stopBrowsing();
        }

        // Stop UDP transport
        if (udpTransport) {
            udpTransport->stop();
        }

        // Stop HTTP server
        if (server) {
            server->stop();
        }
        if (serverThread.joinable()) {
            serverThread.join();
        }

        // Clean up MIDI ports
        if (midiRouter) {
            midiRouter->clearLocalPorts();
        }
        midiInputs.clear();
        midiOutputs.clear();
    }

    // MidiInputCallback interface
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override {
        // Convert JUCE message to raw bytes
        std::vector<uint8_t> data(
            message.getRawData(),
            message.getRawData() + message.getRawDataSize()
        );

        // Find device ID for this input
        auto deviceId = getDeviceIdForInput(source);
        if (deviceId != 0xFFFF && midiRouter) {
            // Route through MidiRouter (handles local and network delivery)
            midiRouter->sendMessage(deviceId, data);
        }
    }

private:
    void registerLocalMidiDevices() {
        uint16_t deviceId = 1;  // Start at 1 (0 reserved)

        // Register MIDI inputs (but don't start them yet)
        auto inputs = juce::MidiInput::getAvailableDevices();
        for (const auto& deviceInfo : inputs) {
            // Add to device registry
            deviceRegistry->addLocalDevice(
                deviceId,
                deviceInfo.name,
                "input",
                deviceInfo.identifier
            );

            // Add to routing table (local route has null UUID)
            routingTable->addRoute(
                juce::Uuid(),  // Local devices use null UUID
                deviceId,
                deviceInfo.name,
                "input"
            );

            // Open MIDI input but DON'T start it yet
            auto input = juce::MidiInput::openDevice(
                deviceInfo.identifier,
                this  // MidiInputCallback
            );
            if (input) {
                // Create port wrapper
                auto port = std::make_unique<JuceMidiPort>(deviceInfo.name, true);
                port->setMidiInput(std::move(input));

                // Store reference for device lookup
                inputDeviceMap[deviceInfo.identifier] = deviceId;

                // Register with router
                midiRouter->registerLocalPort(deviceId, std::move(port));
            }
            deviceId++;
        }

        // Register MIDI outputs
        auto outputs = juce::MidiOutput::getAvailableDevices();
        for (const auto& deviceInfo : outputs) {
            // Add to device registry
            deviceRegistry->addLocalDevice(
                deviceId,
                deviceInfo.name,
                "output",
                deviceInfo.identifier
            );

            // Add to routing table
            routingTable->addRoute(
                juce::Uuid(),  // Local devices use null UUID
                deviceId,
                deviceInfo.name,
                "output"
            );

            // Open MIDI output
            auto output = juce::MidiOutput::openDevice(deviceInfo.identifier);
            if (output) {
                // Create port wrapper
                auto port = std::make_unique<JuceMidiPort>(deviceInfo.name, false);
                port->setMidiOutput(std::move(output));

                // Register with router
                midiRouter->registerLocalPort(deviceId, std::move(port));
            }
            deviceId++;
        }

        std::cout << "Registered " << deviceRegistry->getLocalDeviceCount()
                  << " local MIDI devices" << std::endl;
    }

    void startMidiInputs() {
        // Start all MIDI inputs now that everything is initialized
        auto localDevices = deviceRegistry->getLocalDevices();
        for (const auto& device : localDevices) {
            if (device.type == "input") {
                // Find the corresponding MIDI input by device ID
                // This requires storing the inputs separately
                // For now, we can iterate through available devices again
                auto inputs = juce::MidiInput::getAvailableDevices();
                for (const auto& deviceInfo : inputs) {
                    if (inputDeviceMap.count(deviceInfo.identifier) > 0 &&
                        inputDeviceMap[deviceInfo.identifier] == device.id()) {
                        // Re-open and start the input
                        auto input = juce::MidiInput::openDevice(
                            deviceInfo.identifier,
                            this
                        );
                        if (input) {
                            input->start();
                            midiInputs.push_back(std::move(input));
                        }
                    }
                }
            }
        }
        std::cout << "Started " << midiInputs.size() << " MIDI inputs" << std::endl;
    }

    uint16_t getDeviceIdForInput(juce::MidiInput* source) {
        if (!source) return 0xFFFF;

        auto identifier = source->getIdentifier();
        auto it = inputDeviceMap.find(identifier);
        if (it != inputDeviceMap.end()) {
            return it->second;
        }
        return 0xFFFF;
    }

    void setupDiscoveryCallbacks() {
        serviceDiscovery->startBrowsing(
            [this](const ::NodeInfo& node) {
                std::cout << "Discovered peer: " << node.name.toStdString()
                          << " (UUID: " << node.uuid.toString().toStdString() << ")"
                          << " at " << node.ipAddress.toStdString()
                          << ":" << node.httpPort
                          << " (UDP: " << node.udpPort << ")"
                          << std::endl;

                // Convert to mesh NodeInfo and notify mesh manager
                if (meshManager) {
                    auto meshNode = convertToMeshNodeInfo(node);
                    meshManager->onNodeDiscovered(meshNode);
                }
            },
            [this](const juce::Uuid& nodeId) {
                std::cout << "Lost peer: " << nodeId.toString().toStdString() << std::endl;

                // Notify mesh manager
                if (meshManager) {
                    meshManager->onNodeRemoved(nodeId);
                }
            }
        );
    }

    void setupMeshCallbacks() {
        meshManager->onNodeConnected = [this](const NetworkMidi::NodeInfo& node) {
            std::cout << "Connected to node: " << node.name.toStdString()
                      << " (" << node.deviceCount << " devices)" << std::endl;
        };

        meshManager->onNodeDisconnected = [this](const juce::Uuid& nodeId,
                                                  const juce::String& reason) {
            std::cout << "Disconnected from node: " << nodeId.toString().toStdString()
                      << " - " << reason.toStdString() << std::endl;
        };

        meshManager->onConnectionFailed = [this](const NetworkMidi::NodeInfo& node,
                                                 const juce::String& error) {
            std::cerr << "Connection failed to " << node.name.toStdString()
                      << ": " << error.toStdString() << std::endl;
        };

        // Register remote devices when they are discovered
        meshManager->onRemoteDevicesDiscovered = [this](const juce::Uuid& nodeId,
                                                        const std::vector<NetworkMidi::DeviceInfo>& devices) {
            std::cout << "Registering " << devices.size() << " remote devices from node "
                      << nodeId.toString().toStdString() << std::endl;

            // Add each remote device to registry and routing table
            for (const auto& device : devices) {
                uint16_t remoteDeviceId = device.id;

                // Add to device registry as remote device
                deviceRegistry->addRemoteDevice(
                    nodeId,
                    remoteDeviceId,
                    device.name,
                    device.type
                );

                // Add route to routing table
                routingTable->addRoute(
                    nodeId,  // Remote node UUID
                    remoteDeviceId,
                    device.name,
                    device.type
                );

                std::cout << "  Registered remote device: " << device.name.toStdString()
                          << " (ID: " << remoteDeviceId << ", type: " << device.type.toStdString() << ")"
                          << std::endl;
            }
        };
    }

    void handleNetworkPacket(const NetworkMidi::MidiPacket& packet) {
        // Route received network MIDI to appropriate local device
        if (midiRouter) {
            // Use getter methods to access packet fields
            auto midiData = packet.getMidiData();
            midiRouter->onNetworkPacketReceived(
                packet.getSourceNode(),
                packet.getDeviceId(),
                midiData
            );
        }
    }

    void setupRoutes() {
        // Health check endpoint
        server->Get("/health", [](const httplib::Request&, httplib::Response& res) {
            JsonBuilder json;
            json.startObject().key("status").value(std::string("ok")).endObject();
            res.set_content(json.toString(), "application/json");
        });

        // Node information endpoint
        server->Get("/node/info", [this](const httplib::Request&, httplib::Response& res) {
            JsonBuilder json;
            json.startObject()
                .key("uuid").value(identity.getNodeId().toString().toStdString())
                .key("name").value(identity.getNodeName().toStdString())
                .key("hostname").value(identity.getHostname().toStdString())
                .key("http_port").value(actualPort)
                .key("udp_port").value(udpPort)
                .key("local_devices").value(deviceRegistry ? deviceRegistry->getLocalDeviceCount() : 0)
                .key("total_devices").value(deviceRegistry ? deviceRegistry->getTotalDeviceCount() : 0)
                .endObject();

            res.set_content(json.toString(), "application/json");
        });

        // List MIDI devices (integrated with device registry)
        server->Get("/midi/devices", [this](const httplib::Request&, httplib::Response& res) {
            JsonBuilder json;
            json.startObject();

            if (deviceRegistry) {
                auto devices = deviceRegistry->getAllDevices();

                json.key("devices").startArray();
                for (const auto& device : devices) {
                    json.startObject()
                        .key("id").value((int)device.id())
                        .key("name").value(device.name.toStdString())
                        .key("type").value(device.type.toStdString())
                        .key("is_local").value(device.isLocal())
                        .key("owner_node").value(device.ownerNode().toString().toStdString())
                        .endObject();
                }
                json.endArray();
            }

            json.endObject();
            res.set_content(json.toString(), "application/json");
        });

        // Network mesh status
        server->Get("/network/mesh", [this](const httplib::Request&, httplib::Response& res) {
            try {
                std::cerr << "DEBUG: /network/mesh: Starting" << std::endl;
                JsonBuilder json;
                json.startObject();

                if (meshManager) {
                    std::cerr << "DEBUG: /network/mesh: Getting statistics" << std::endl;
                    auto stats = meshManager->getStatistics();
                    std::cerr << "DEBUG: /network/mesh: Got statistics" << std::endl;
                    json.key("connected_nodes").value((int)stats.connectedNodes)
                        .key("total_nodes").value((int)stats.totalNodes)
                        .key("total_devices").value(stats.totalDevices);

                    std::cerr << "DEBUG: /network/mesh: Getting connected nodes" << std::endl;
                    json.key("nodes").startArray();
                    auto nodes = meshManager->getConnectedNodes();
                    std::cerr << "DEBUG: /network/mesh: Got " << nodes.size() << " nodes" << std::endl;
                    for (const auto& node : nodes) {
                        std::cerr << "DEBUG: /network/mesh: Processing node " << node.name.toStdString() << std::endl;
                        json.startObject()
                            .key("uuid").value(node.uuid.toString().toStdString())
                            .key("name").value(node.name.toStdString())
                            .key("ip").value(node.ipAddress.toStdString())
                            .key("http_port").value(node.httpPort)
                            .key("udp_port").value(node.udpPort)
                            .key("devices").value(node.deviceCount)
                            .endObject();
                        std::cerr << "DEBUG: /network/mesh: Finished node " << node.name.toStdString() << std::endl;
                    }
                    json.endArray();
                } else {
                    json.key("error").value(std::string("Mesh manager not initialized"));
                }

                std::cerr << "DEBUG: /network/mesh: Building JSON" << std::endl;
                json.endObject();
                std::cerr << "DEBUG: /network/mesh: Sending response" << std::endl;
                res.set_content(json.toString(), "application/json");
                res.status = 200;
                std::cerr << "DEBUG: /network/mesh: Done" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error in /network/mesh: " << e.what() << std::endl;
                res.set_content(std::string("Error: ") + e.what(), "text/plain");
                res.status = 500;
            }
        });

        // Router statistics
        server->Get("/network/stats", [this](const httplib::Request&, httplib::Response& res) {
            JsonBuilder json;
            json.startObject();

            if (midiRouter) {
                auto stats = midiRouter->getStatistics();
                json.key("local_sent").value((int)stats.localMessagesSent)
                    .key("local_received").value((int)stats.localMessagesReceived)
                    .key("network_sent").value((int)stats.networkMessagesSent)
                    .key("network_received").value((int)stats.networkMessagesReceived)
                    .key("routing_errors").value((int)stats.routingErrors);
            }

            if (udpTransport) {
                auto stats = udpTransport->getStatistics();
                json.key("packets_sent").value((int)stats.packetsSent)
                    .key("packets_received").value((int)stats.packetsReceived)
                    .key("bytes_sent").value((int)stats.bytesSent)
                    .key("bytes_received").value((int)stats.bytesReceived);
            }

            json.endObject();
            res.set_content(json.toString(), "application/json");
        });

        // Network handshake endpoint (for mesh connection establishment)
        server->Post("/network/handshake", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                // Parse incoming handshake request
                // Expected format: {"node_id":"...", "node_name":"...", "udp_endpoint":"...", "version":"..."}

                std::string nodeIdStr, nodeName, udpEndpoint, version;

                // Simple JSON parsing (extract values)
                auto body = req.body;
                size_t pos;

                // Extract node_id
                pos = body.find("\"node_id\":\"");
                if (pos != std::string::npos) {
                    size_t start = pos + 11;
                    size_t end = body.find("\"", start);
                    nodeIdStr = body.substr(start, end - start);
                }

                // Extract node_name
                pos = body.find("\"node_name\":\"");
                if (pos != std::string::npos) {
                    size_t start = pos + 13;
                    size_t end = body.find("\"", start);
                    nodeName = body.substr(start, end - start);
                }

                // Extract udp_endpoint
                pos = body.find("\"udp_endpoint\":\"");
                if (pos != std::string::npos) {
                    size_t start = pos + 16;
                    size_t end = body.find("\"", start);
                    udpEndpoint = body.substr(start, end - start);
                }

                // Extract version
                pos = body.find("\"version\":\"");
                if (pos != std::string::npos) {
                    size_t start = pos + 11;
                    size_t end = body.find("\"", start);
                    version = body.substr(start, end - start);
                }

                std::cout << "Handshake request from: " << nodeName
                          << " (UUID: " << nodeIdStr << ")"
                          << " UDP: " << udpEndpoint << std::endl;

                // Build handshake response with our node info and device list
                JsonBuilder json;
                json.startObject()
                    .key("node_id").value(identity.getNodeId().toString().toStdString())
                    .key("node_name").value(identity.getNodeName().toStdString())
                    .key("udp_endpoint").value(std::to_string(udpPort))
                    .key("version").value(std::string("1.0"));

                // Add device list
                if (deviceRegistry) {
                    auto devices = deviceRegistry->getLocalDevices();
                    json.key("devices").startArray();
                    for (const auto& device : devices) {
                        json.startObject()
                            .key("id").value((int)device.id())
                            .key("name").value(device.name.toStdString())
                            .key("type").value(device.type.toStdString())
                            .endObject();
                    }
                    json.endArray();
                }

                json.endObject();

                res.set_content(json.toString(), "application/json");
                res.status = 200;

            } catch (const std::exception& e) {
                std::cerr << "Handshake error: " << e.what() << std::endl;
                res.set_content(std::string("Handshake failed: ") + e.what(), "text/plain");
                res.status = 500;
            }
        });
    }

    const NetworkMidi::NodeIdentity& identity;
    int requestedPort;
    int actualPort;
    int udpPort;

    // Network components
    std::unique_ptr<ServiceDiscovery> serviceDiscovery;
    std::unique_ptr<NetworkMidi::UdpMidiTransport> udpTransport;
    std::unique_ptr<NetworkMidi::DeviceRegistry> deviceRegistry;
    std::unique_ptr<NetworkMidi::RoutingTable> routingTable;
    std::unique_ptr<NetworkMidi::MidiRouter> midiRouter;
    std::unique_ptr<NetworkMidi::MeshManager> meshManager;
    std::unique_ptr<NetworkTransportAdapter> networkAdapter;

    // HTTP server
    std::unique_ptr<httplib::Server> server;
    std::thread serverThread;

    // Local MIDI device tracking
    std::vector<std::unique_ptr<juce::MidiInput>> midiInputs;
    std::vector<std::unique_ptr<juce::MidiOutput>> midiOutputs;
    std::map<juce::String, uint16_t> inputDeviceMap;
};

//==============================================================================
int main(int argc, char* argv[])
{
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\nNetwork MIDI Server v1.0 - Full Mesh Integration" << std::endl;
    std::cout << "=================================================" << std::endl;

    // Initialize node identity (each instance gets a unique UUID)
    NetworkMidi::NodeIdentity identity;

    // Initialize instance manager (creates lock file and temp directory)
    std::unique_ptr<NetworkMidi::InstanceManager> instanceManager;
    try {
        instanceManager = std::make_unique<NetworkMidi::InstanceManager>(identity.getNodeId());
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        std::cerr << "Unexpected error during instance initialization." << std::endl;
        return 1;
    }

    // Parse port from command line (0 = auto-assign)
    int port = 0;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    std::cout << "\nStarting server..." << std::endl;
    if (port == 0) {
        std::cout << "  HTTP Port: auto-assigned" << std::endl;
    } else {
        std::cout << "  HTTP Port: " << port << std::endl;
    }

    NetworkMidiServer server(identity, port);
    server.startServer();

    int actualPort = server.getActualPort();
    int udpPort = server.getUdpPort();

    std::cout << "\nServer running:" << std::endl;
    std::cout << "  HTTP Port: " << actualPort << std::endl;
    std::cout << "  UDP Port: " << udpPort << std::endl;
    std::cout << "  Node: " << identity.getNodeName().toStdString() << std::endl;
    std::cout << "  UUID: " << identity.getNodeId().toString().toStdString() << std::endl;
    std::cout << "  Instance dir: " << instanceManager->getInstanceDirectory()
                                       .getFullPathName().toStdString() << std::endl;

    std::cout << "\nEndpoints:" << std::endl;
    std::cout << "  GET  /health          - Health check" << std::endl;
    std::cout << "  GET  /node/info       - Node information" << std::endl;
    std::cout << "  GET  /midi/devices    - List all MIDI devices (local + remote)" << std::endl;
    std::cout << "  GET  /network/mesh    - Network mesh status" << std::endl;
    std::cout << "  GET  /network/stats   - Network statistics" << std::endl;

    std::cout << "\nReady. Press Ctrl+C to stop..." << std::endl;

    // Run until interrupted
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
