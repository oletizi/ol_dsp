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
#include "network/routing/RouteManager.h"
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
        if (input) {
            std::cout << "DEBUG: Starting MIDI input: " << input->getName().toStdString() << std::endl;
            std::cout << "  Identifier: " << input->getIdentifier().toStdString() << std::endl;
            input->start();
            std::cout << "  Started successfully!" << std::endl;
        }
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

    void sendPacket(const NetworkMidi::MidiPacket& packet) override {
        // Phase 4.5: Send packet with full context via UDP
        if (meshManager) {
            auto nodeInfo = meshManager->getNodeInfo(packet.getDestNode());
            if (nodeInfo.isValid()) {
                udpTransport.sendPacket(
                    packet,
                    nodeInfo.ipAddress,
                    nodeInfo.udpPort
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
    NetworkMidiServer(const NetworkMidi::NodeIdentity& nodeIdentity,
                     NetworkMidi::InstanceManager* instanceMgr,
                     int port = 0)
        : identity(nodeIdentity)
        , instanceManager(instanceMgr)
        , requestedPort(port)
        , actualPort(0)
        , udpPort(0) {}

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
        routeManager = std::make_unique<NetworkMidi::RouteManager>(*deviceRegistry);

        // 3. Create MIDI router BEFORE registering devices
        midiRouter = std::make_unique<NetworkMidi::MidiRouter>(*deviceRegistry, *routingTable);
        networkAdapter = std::make_unique<NetworkTransportAdapter>(*udpTransport);
        midiRouter->setNetworkTransport(networkAdapter.get());
        midiRouter->setRouteManager(routeManager.get());  // Connect router to route manager

        // 4. Create virtual MIDI ports (Phase 1: Virtual MIDI Ports)
        createVirtualMidiPorts();

        // 5. Register virtual MIDI ports (Phase 2: Device Registration)
        registerVirtualMidiPorts();

        // 6. Enumerate and register local MIDI devices (now that router exists)
        registerLocalMidiDevices();

        // 7. Load persisted routes (after devices are registered)
        loadRoutes();

        // Set up UDP packet reception callback
        udpTransport->onPacketReceived = [this](const NetworkMidi::MidiPacket& packet,
                                                const juce::String& /*sourceAddr*/,
                                                int /*sourcePort*/) {
            handleNetworkPacket(packet);
        };

        // 8. Create HTTP server
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

        // 9. Create service discovery (advertise this node)
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

        // 10. Create mesh manager (handle peer connections)
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

        // Phase 4.5: Inject UuidRegistry and node ID into MidiRouter
        if (midiRouter) {
            midiRouter->setNodeId(identity.getNodeId());
            midiRouter->setUuidRegistry(&meshManager->getUuidRegistry());
        }

        // 11. NOW start MIDI inputs (after everything is fully initialized)
        startMidiInputs();
    }

    void stopServer() {
        // Save routes before shutdown
        saveRoutes();

        // Stop MIDI inputs first
        for (auto& input : midiInputs) {
            if (input) {
                input->stop();
            }
        }

        // Stop virtual MIDI ports
        if (virtualInput) {
            virtualInput->stop();
            virtualInput.reset();
        }
        if (virtualOutput) {
            virtualOutput.reset();
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
        std::cout << "=== MIDI CALLBACK INVOKED ===" << std::endl;
        std::cout << "DEBUG: MIDI message received from " << (source ? source->getName().toStdString() : "unknown") << std::endl;
        std::cout << "Message bytes: " << message.getRawDataSize() << std::endl;

        // Convert JUCE message to raw bytes
        std::vector<uint8_t> data(
            message.getRawData(),
            message.getRawData() + message.getRawDataSize()
        );

        // Find device ID for this input
        auto deviceId = getDeviceIdForInput(source);
        std::cout << "DEBUG: Device ID = " << deviceId << std::endl;

        if (deviceId != 0xFFFF && midiRouter) {
            std::cout << "DEBUG: Routing MIDI message through midiRouter (using RouteManager)" << std::endl;
            // Route through MidiRouter using RouteManager (handles local and network delivery)
            // For local devices, use null UUID as source node
            juce::Uuid localNodeId(juce::String("00000000-0000-0000-0000-000000000000"));
            midiRouter->forwardMessage(localNodeId, deviceId, data);
        } else {
            std::cout << "DEBUG: NOT routing - deviceId=" << deviceId << ", midiRouter=" << (midiRouter ? "exists" : "null") << std::endl;
        }
    }

private:
    //==============================================================================
    // Phase 1: Virtual MIDI Port Creation
    void createVirtualMidiPorts() {
        std::cout << "DEBUG: createVirtualMidiPorts() called" << std::endl;

        // Get short UUID for naming (first 8 chars)
        auto shortUuid = identity.getNodeId().toString().substring(0, 8);
        std::cout << "DEBUG: Short UUID = " << shortUuid.toStdString() << std::endl;

        // Create virtual input (receive from other apps)
        auto inputName = juce::String("Network MIDI Node ") + shortUuid + " In";
        virtualInput = juce::MidiInput::createNewDevice(inputName, this);
        if (virtualInput) {
            virtualInput->start();
            std::cout << "Created virtual MIDI input: " << inputName.toStdString() << std::endl;
        } else {
            std::cerr << "Warning: Failed to create virtual MIDI input" << std::endl;
        }

        // Create virtual output (send to other apps)
        auto outputName = juce::String("Network MIDI Node ") + shortUuid + " Out";
        virtualOutput = juce::MidiOutput::createNewDevice(outputName);
        if (virtualOutput) {
            std::cout << "Created virtual MIDI output: " << outputName.toStdString() << std::endl;
        } else {
            std::cerr << "Warning: Failed to create virtual MIDI output" << std::endl;
        }
    }

    //==============================================================================
    // Phase 2: Virtual MIDI Port Registration
    void registerVirtualMidiPorts() {
        std::cout << "DEBUG: registerVirtualMidiPorts() called" << std::endl;

        // Register virtual input as device ID 1
        if (virtualInput) {
            std::cout << "Registering virtual input as device ID 1" << std::endl;

            // Add to device registry
            deviceRegistry->addLocalDevice(
                1,
                virtualInput->getName(),
                "input",
                virtualInput->getIdentifier()
            );

            // Add to routing table (local devices use null UUID)
            routingTable->addRoute(
                juce::Uuid(),  // Local device
                1,
                virtualInput->getName(),
                "input"
            );

            // Store mapping for input device lookup
            inputDeviceMap[virtualInput->getIdentifier()] = 1;

            std::cout << "  Registered virtual input: " << virtualInput->getName().toStdString() << std::endl;
        }

        // Register virtual output as device ID 2
        if (virtualOutput) {
            std::cout << "Registering virtual output as device ID 2" << std::endl;

            // Add to device registry
            deviceRegistry->addLocalDevice(
                2,
                virtualOutput->getName(),
                "output",
                juce::String()  // Virtual outputs don't have identifiers in same way
            );

            // Add to routing table (local devices use null UUID)
            routingTable->addRoute(
                juce::Uuid(),  // Local device
                2,
                virtualOutput->getName(),
                "output"
            );

            std::cout << "  Registered virtual output: " << virtualOutput->getName().toStdString() << std::endl;
        }

        std::cout << "Virtual MIDI ports registered successfully" << std::endl;
    }

    void registerLocalMidiDevices() {
        // Phase 2: Start at device ID 3 (IDs 1-2 are reserved for virtual ports)
        uint16_t deviceId = 3;

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
        // MIDI inputs are now started when registered in JuceMidiPort::setMidiInput()
        // This function is kept for compatibility but no longer opens duplicate inputs
        auto localDevices = deviceRegistry->getLocalDevices();
        int inputCount = 0;
        for (const auto& device : localDevices) {
            if (device.type == "input") {
                inputCount++;
            }
        }
        std::cout << "Started " << inputCount << " MIDI inputs (via port wrappers)" << std::endl;
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
            // Use Phase 4.3 packet handling (with ForwardingContext support)
            midiRouter->onNetworkPacketReceived(packet);
        }
    }

    void loadRoutes() {
        if (!routeManager || !instanceManager) {
            return;
        }

        auto routesFile = instanceManager->getStateFile("routes.json");
        if (routesFile.existsAsFile()) {
            std::cout << "Loading routes from " << routesFile.getFullPathName().toStdString() << std::endl;
            if (routeManager->loadFromFile(routesFile)) {
                int count = routeManager->getRuleCount();
                std::cout << "Loaded " << count << " routing rule(s)" << std::endl;
            } else {
                std::cerr << "Warning: Failed to load routes from file" << std::endl;
            }
        } else {
            std::cout << "No routes file found, starting with empty routing table" << std::endl;
        }
    }

    void saveRoutes() {
        if (!routeManager || !instanceManager) {
            return;
        }

        auto routesFile = instanceManager->getStateFile("routes.json");
        std::cout << "Saving routes to " << routesFile.getFullPathName().toStdString() << std::endl;
        if (routeManager->saveToFile(routesFile)) {
            int count = routeManager->getRuleCount();
            std::cout << "Saved " << count << " routing rule(s)" << std::endl;
        } else {
            std::cerr << "Warning: Failed to save routes to file" << std::endl;
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

        //=================================================================================
        // MIDI Routing Configuration API (Phase 2)
        //=================================================================================

        // Helper to parse node ID from string ("local" -> null UUID)
        auto parseNodeId = [](const std::string& str) -> juce::Uuid {
            std::cout << "DEBUG parseNodeId: Input string = '" << str << "'" << std::endl;

            if (str == "local" || str.empty()) {
                std::cout << "DEBUG parseNodeId: Treating as local (null UUID)" << std::endl;
                return juce::Uuid(juce::String("00000000-0000-0000-0000-000000000000"));
            }

            // Create JUCE String and parse UUID
            juce::String juceStr(str);
            std::cout << "DEBUG parseNodeId: Created juce::String = '" << juceStr.toStdString() << "'" << std::endl;

            // JUCE Uuid constructor expects dashed format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
            // It will generate a RANDOM UUID if parsing fails!
            juce::Uuid parsed(juceStr);
            std::cout << "DEBUG parseNodeId: Parsed UUID = " << parsed.toString().toStdString() << std::endl;

            // Verify parsing worked by checking if it's null (shouldn't be for valid UUID)
            if (parsed.isNull() && !str.empty() && str != "local") {
                std::cerr << "WARNING parseNodeId: UUID parsing may have failed! Got null UUID from: " << str << std::endl;
            }

            return parsed;
        };

        // 1. GET /routing/routes - List all routing rules
        server->Get("/routing/routes", [this](const httplib::Request&, httplib::Response& res) {
            try {
                if (!routeManager) {
                    res.set_content("{\"error\":\"Route manager not initialized\"}", "application/json");
                    res.status = 500;
                    return;
                }

                auto rules = routeManager->getAllRules();
                auto stats = routeManager->getStatistics();

                JsonBuilder json;
                json.startObject();

                // Build routes array
                json.key("routes").startArray();
                for (const auto& rule : rules) {
                    json.startObject()
                        .key("route_id").value(rule.ruleId.toStdString())
                        .key("enabled").value(rule.enabled)
                        .key("priority").value(rule.priority);

                    // Source device
                    json.key("source").startObject()
                        .key("node_id").value(rule.sourceNodeId().toString().toStdString())
                        .key("device_id").value((int)rule.sourceDeviceId())
                        .endObject();

                    // Destination device
                    json.key("destination").startObject()
                        .key("node_id").value(rule.destinationNodeId().toString().toStdString())
                        .key("device_id").value((int)rule.destinationDeviceId())
                        .endObject();

                    // Statistics
                    json.key("messages_forwarded").value((int)rule.statistics.messagesForwarded)
                        .key("messages_dropped").value((int)rule.statistics.messagesDropped)
                        .endObject();
                }
                json.endArray();

                // Summary statistics
                json.key("total").value((int)stats.totalRules)
                    .key("enabled").value((int)stats.enabledRules)
                    .key("disabled").value((int)stats.disabledRules)
                    .endObject();

                res.set_content(json.toString(), "application/json");
                res.status = 200;

            } catch (const std::exception& e) {
                std::cerr << "Error in /routing/routes: " << e.what() << std::endl;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
                res.status = 500;
            }
        });

        // 2. POST /routing/routes - Create a new routing rule
        server->Post("/routing/routes", [this, parseNodeId](const httplib::Request& req, httplib::Response& res) {
            try {
                if (!routeManager) {
                    res.set_content("{\"error\":\"Route manager not initialized\"}", "application/json");
                    res.status = 500;
                    return;
                }

                // Parse JSON body (simple extraction)
                auto body = req.body;
                std::cout << "DEBUG POST /routing/routes: Body = " << body << std::endl;

                std::string srcNodeStr, dstNodeStr;
                int srcDeviceId = 0, dstDeviceId = 0;
                bool enabled = true;
                int priority = 100;

                // Extract fields
                auto extractString = [&body](const std::string& key) -> std::string {
                    std::string pattern = "\"" + key + "\"";
                    size_t pos = body.find(pattern);
                    if (pos != std::string::npos) {
                        // Skip past the key and find the colon
                        size_t colonPos = body.find(":", pos);
                        if (colonPos != std::string::npos) {
                            // Skip whitespace after colon and find opening quote
                            size_t quotePos = body.find("\"", colonPos);
                            if (quotePos != std::string::npos) {
                                size_t start = quotePos + 1;
                                size_t end = body.find("\"", start);
                                if (end != std::string::npos) {
                                    std::string extracted = body.substr(start, end - start);
                                    std::cout << "DEBUG extractString: key='" << key << "' extracted='" << extracted << "'" << std::endl;
                                    return extracted;
                                }
                            }
                        }
                    }
                    std::cout << "DEBUG extractString: key='" << key << "' NOT FOUND" << std::endl;
                    return "";
                };

                auto extractInt = [&body](const std::string& key) -> int {
                    std::string pattern = "\"" + key + "\":";
                    size_t pos = body.find(pattern);
                    if (pos != std::string::npos) {
                        size_t start = pos + pattern.length();
                        size_t end = body.find_first_of(",}", start);
                        if (end != std::string::npos) {
                            int extracted = std::stoi(body.substr(start, end - start));
                            std::cout << "DEBUG extractInt: key='" << key << "' extracted=" << extracted << std::endl;
                            return extracted;
                        }
                    }
                    std::cout << "DEBUG extractInt: key='" << key << "' NOT FOUND, returning 0" << std::endl;
                    return 0;
                };

                auto extractBool = [&body](const std::string& key) -> bool {
                    std::string pattern = "\"" + key + "\":";
                    size_t pos = body.find(pattern);
                    if (pos != std::string::npos) {
                        size_t start = pos + pattern.length();
                        return body.substr(start, 4) == "true";
                    }
                    return true;
                };

                srcNodeStr = extractString("source_node_id");
                srcDeviceId = extractInt("source_device_id");
                dstNodeStr = extractString("destination_node_id");
                dstDeviceId = extractInt("destination_device_id");

                // Optional fields
                if (body.find("\"enabled\"") != std::string::npos) {
                    enabled = extractBool("enabled");
                }
                if (body.find("\"priority\"") != std::string::npos) {
                    priority = extractInt("priority");
                }

                std::cout << "DEBUG POST /routing/routes: About to parse source node ID: '" << srcNodeStr << "'" << std::endl;
                auto srcNodeId = parseNodeId(srcNodeStr);

                std::cout << "DEBUG POST /routing/routes: About to parse destination node ID: '" << dstNodeStr << "'" << std::endl;
                auto dstNodeId = parseNodeId(dstNodeStr);

                std::cout << "DEBUG POST /routing/routes: Creating rule with:" << std::endl;
                std::cout << "  Source: node=" << srcNodeId.toString().toStdString() << " device=" << srcDeviceId << std::endl;
                std::cout << "  Destination: node=" << dstNodeId.toString().toStdString() << " device=" << dstDeviceId << std::endl;

                // Create rule
                NetworkMidi::ForwardingRule rule(srcNodeId, (uint16_t)srcDeviceId,
                                                  dstNodeId, (uint16_t)dstDeviceId);
                rule.enabled = enabled;
                rule.priority = priority;

                // Add rule to manager
                std::string ruleId = routeManager->addRule(rule);

                std::cout << "DEBUG POST /routing/routes: Created rule with ID: " << ruleId << std::endl;

                // Build response
                JsonBuilder json;
                json.startObject()
                    .key("route_id").value(ruleId)
                    .key("status").value(std::string("created"))
                    .endObject();

                res.set_content(json.toString(), "application/json");
                res.status = 201;

            } catch (const std::exception& e) {
                std::cerr << "Error creating route: " << e.what() << std::endl;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
                res.status = 400;
            }
        });

        // 3. GET /routing/routes/:route_id - Get specific route details
        server->Get(R"(/routing/routes/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (!routeManager) {
                    res.set_content("{\"error\":\"Route manager not initialized\"}", "application/json");
                    res.status = 500;
                    return;
                }

                std::string ruleId = req.matches[1];
                auto rule = routeManager->getRule(ruleId);

                if (!rule.has_value()) {
                    res.set_content("{\"error\":\"Route not found\"}", "application/json");
                    res.status = 404;
                    return;
                }

                // Build detailed response
                JsonBuilder json;
                json.startObject()
                    .key("route_id").value(rule->ruleId.toStdString())
                    .key("enabled").value(rule->enabled)
                    .key("priority").value(rule->priority);

                // Source device
                json.key("source").startObject()
                    .key("node_id").value(rule->sourceNodeId().toString().toStdString())
                    .key("device_id").value((int)rule->sourceDeviceId())
                    .endObject();

                // Destination device
                json.key("destination").startObject()
                    .key("node_id").value(rule->destinationNodeId().toString().toStdString())
                    .key("device_id").value((int)rule->destinationDeviceId())
                    .endObject();

                // Statistics
                json.key("statistics").startObject()
                    .key("messages_forwarded").value((int)rule->statistics.messagesForwarded)
                    .key("messages_dropped").value((int)rule->statistics.messagesDropped)
                    .endObject();

                json.endObject();

                res.set_content(json.toString(), "application/json");
                res.status = 200;

            } catch (const std::exception& e) {
                std::cerr << "Error retrieving route: " << e.what() << std::endl;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
                res.status = 500;
            }
        });

        // 4. PUT /routing/routes/:route_id - Update a route
        server->Put(R"(/routing/routes/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (!routeManager) {
                    res.set_content("{\"error\":\"Route manager not initialized\"}", "application/json");
                    res.status = 500;
                    return;
                }

                std::string ruleId = req.matches[1];
                auto existingRule = routeManager->getRule(ruleId);

                if (!existingRule.has_value()) {
                    res.set_content("{\"error\":\"Route not found\"}", "application/json");
                    res.status = 404;
                    return;
                }

                // Parse JSON body for updates
                auto body = req.body;
                auto updatedRule = *existingRule;

                // Check for enabled field
                if (body.find("\"enabled\"") != std::string::npos) {
                    size_t pos = body.find("\"enabled\":");
                    if (pos != std::string::npos) {
                        size_t start = pos + 10;
                        updatedRule.enabled = (body.substr(start, 4) == "true");
                    }
                }

                // Check for priority field
                if (body.find("\"priority\"") != std::string::npos) {
                    size_t pos = body.find("\"priority\":");
                    if (pos != std::string::npos) {
                        size_t start = pos + 11;
                        size_t end = body.find_first_of(",}", start);
                        if (end != std::string::npos) {
                            updatedRule.priority = std::stoi(body.substr(start, end - start));
                        }
                    }
                }

                // Update the rule
                if (routeManager->updateRule(ruleId, updatedRule)) {
                    JsonBuilder json;
                    json.startObject()
                        .key("status").value(std::string("updated"))
                        .key("route_id").value(ruleId)
                        .endObject();

                    res.set_content(json.toString(), "application/json");
                    res.status = 200;
                } else {
                    res.set_content("{\"error\":\"Failed to update route\"}", "application/json");
                    res.status = 500;
                }

            } catch (const std::exception& e) {
                std::cerr << "Error updating route: " << e.what() << std::endl;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
                res.status = 400;
            }
        });

        // 5. DELETE /routing/routes/:route_id - Delete a route
        server->Delete(R"(/routing/routes/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (!routeManager) {
                    res.set_content("{\"error\":\"Route manager not initialized\"}", "application/json");
                    res.status = 500;
                    return;
                }

                std::string ruleId = req.matches[1];

                if (routeManager->removeRule(ruleId)) {
                    JsonBuilder json;
                    json.startObject()
                        .key("status").value(std::string("deleted"))
                        .key("route_id").value(ruleId)
                        .endObject();

                    res.set_content(json.toString(), "application/json");
                    res.status = 200;
                } else {
                    res.set_content("{\"error\":\"Route not found\"}", "application/json");
                    res.status = 404;
                }

            } catch (const std::exception& e) {
                std::cerr << "Error deleting route: " << e.what() << std::endl;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
                res.status = 500;
            }
        });

        // 6. GET /routing/table - Get complete routing state (debug endpoint)
        server->Get("/routing/table", [this](const httplib::Request&, httplib::Response& res) {
            try {
                JsonBuilder json;
                json.startObject();

                if (routeManager) {
                    auto rules = routeManager->getAllRules();
                    auto stats = routeManager->getStatistics();

                    json.key("total_rules").value((int)stats.totalRules)
                        .key("enabled_rules").value((int)stats.enabledRules)
                        .key("disabled_rules").value((int)stats.disabledRules)
                        .key("total_messages_forwarded").value((int)stats.totalMessagesForwarded)
                        .key("total_messages_dropped").value((int)stats.totalMessagesDropped);

                    json.key("rules").startArray();
                    for (const auto& rule : rules) {
                        json.startObject()
                            .key("route_id").value(rule.ruleId.toStdString())
                            .key("enabled").value(rule.enabled)
                            .key("priority").value(rule.priority)
                            .key("source_node").value(rule.sourceNodeId().toString().toStdString())
                            .key("source_device").value((int)rule.sourceDeviceId())
                            .key("dest_node").value(rule.destinationNodeId().toString().toStdString())
                            .key("dest_device").value((int)rule.destinationDeviceId())
                            .key("messages_forwarded").value((int)rule.statistics.messagesForwarded)
                            .key("messages_dropped").value((int)rule.statistics.messagesDropped)
                            .endObject();
                    }
                    json.endArray();
                }

                if (deviceRegistry) {
                    auto devices = deviceRegistry->getAllDevices();
                    json.key("devices").startArray();
                    for (const auto& device : devices) {
                        json.startObject()
                            .key("node_id").value(device.ownerNode().toString().toStdString())
                            .key("device_id").value((int)device.id())
                            .key("name").value(device.name.toStdString())
                            .key("type").value(device.type.toStdString())
                            .key("is_local").value(device.isLocal())
                            .endObject();
                    }
                    json.endArray();
                }

                json.endObject();
                res.set_content(json.toString(), "application/json");
                res.status = 200;

            } catch (const std::exception& e) {
                std::cerr << "Error in /routing/table: " << e.what() << std::endl;
                res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
                res.status = 500;
            }
        });
    }

    const NetworkMidi::NodeIdentity& identity;
    NetworkMidi::InstanceManager* instanceManager;
    int requestedPort;
    int actualPort;
    int udpPort;

    // Network components
    std::unique_ptr<ServiceDiscovery> serviceDiscovery;
    std::unique_ptr<NetworkMidi::UdpMidiTransport> udpTransport;
    std::unique_ptr<NetworkMidi::DeviceRegistry> deviceRegistry;
    std::unique_ptr<NetworkMidi::RoutingTable> routingTable;
    std::unique_ptr<NetworkMidi::RouteManager> routeManager;
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

    // Phase 1: Virtual MIDI ports
    std::unique_ptr<juce::MidiInput> virtualInput;
    std::unique_ptr<juce::MidiOutput> virtualOutput;
};

//==============================================================================
// Command-line argument parsing structure
struct CommandLineOptions
{
    juce::String nodeId;
    int port = 0;
    bool showHelp = false;
    bool hasError = false;
    juce::String errorMessage;

    bool parseArguments(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            juce::String arg(argv[i]);

            if (arg == "--help" || arg == "-h") {
                showHelp = true;
                return true;
            }

            if (arg.startsWith("--node-id=")) {
                nodeId = arg.substring(10);
                if (nodeId.isEmpty()) {
                    hasError = true;
                    errorMessage = "Error: --node-id argument requires a UUID value";
                    return false;
                }
                // Validate UUID format
                juce::Uuid testUuid(nodeId);
                if (testUuid.isNull()) {
                    hasError = true;
                    errorMessage = "Error: Invalid UUID format for --node-id: " + nodeId;
                    return false;
                }
            }
            else if (arg.startsWith("--port=")) {
                juce::String portStr = arg.substring(7);
                if (portStr.isEmpty()) {
                    hasError = true;
                    errorMessage = "Error: --port argument requires a numeric value";
                    return false;
                }

                port = portStr.getIntValue();
                if (port < 1024 || port > 65535) {
                    hasError = true;
                    errorMessage = "Error: Port must be in range 1024-65535, got: " + juce::String(port);
                    return false;
                }
            }
            else {
                hasError = true;
                errorMessage = "Error: Unknown argument: " + arg;
                return false;
            }
        }
        return true;
    }

    void printUsage() const {
        std::cout << "\nUsage: network_midi_server [OPTIONS]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --node-id=<uuid>    Override auto-generated node UUID\n";
        std::cout << "                      Example: --node-id=\"a1b2c3d4-e5f6-7890-1234-567890abcdef\"\n";
        std::cout << "  --port=<number>     Use specific HTTP port (1024-65535)\n";
        std::cout << "                      Example: --port=8001\n";
        std::cout << "  --help, -h          Show this help message\n\n";
        std::cout << "If no arguments are provided, the server will:\n";
        std::cout << "  - Auto-generate a unique node UUID\n";
        std::cout << "  - Auto-assign an available HTTP port\n\n";
    }
};

//==============================================================================
int main(int argc, char* argv[])
{
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\nNetwork MIDI Server v1.0 - Full Mesh Integration" << std::endl;
    std::cout << "=================================================" << std::endl;

    // Parse command-line arguments
    CommandLineOptions options;
    if (!options.parseArguments(argc, argv)) {
        std::cerr << "\n" << options.errorMessage.toStdString() << "\n";
        options.printUsage();
        return 1;
    }

    if (options.showHelp) {
        options.printUsage();
        return 0;
    }

    // Initialize node identity
    NetworkMidi::NodeIdentity identity;

    // Override node ID if specified via CLI
    if (options.nodeId.isNotEmpty()) {
        juce::Uuid customUuid(options.nodeId);
        std::cout << "\nUsing CLI-specified node ID: " << customUuid.toString().toStdString() << std::endl;
        identity = NetworkMidi::NodeIdentity::createWithUuid(customUuid);
    }

    // Initialize instance manager (creates lock file and temp directory)
    std::unique_ptr<NetworkMidi::InstanceManager> instanceManager;
    try {
        instanceManager = std::make_unique<NetworkMidi::InstanceManager>(identity.getNodeId());
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        std::cerr << "Unexpected error during instance initialization." << std::endl;
        return 1;
    }

    std::cout << "\nStarting server..." << std::endl;
    if (options.port == 0) {
        std::cout << "  HTTP Port: auto-assigned" << std::endl;
    } else {
        std::cout << "  HTTP Port: " << options.port << " (CLI-specified)" << std::endl;
    }

    NetworkMidiServer server(identity, instanceManager.get(), options.port);
    server.startServer();

    int actualPort = server.getActualPort();
    int udpPort = server.getUdpPort();

    std::cout << "\nServer running:" << std::endl;
    std::cout << "  HTTP Port: " << actualPort;
    if (options.port != 0) {
        std::cout << " (CLI-specified)";
    }
    std::cout << std::endl;
    std::cout << "  UDP Port: " << udpPort << std::endl;
    std::cout << "  Node: " << identity.getNodeName().toStdString() << std::endl;
    std::cout << "  UUID: " << identity.getNodeId().toString().toStdString();
    if (options.nodeId.isNotEmpty()) {
        std::cout << " (CLI-specified)";
    }
    std::cout << std::endl;
    std::cout << "  Instance dir: " << instanceManager->getInstanceDirectory()
                                       .getFullPathName().toStdString() << std::endl;

    std::cout << "\nEndpoints:" << std::endl;
    std::cout << "  GET    /health                - Health check" << std::endl;
    std::cout << "  GET    /node/info             - Node information" << std::endl;
    std::cout << "  GET    /midi/devices          - List all MIDI devices (local + remote)" << std::endl;
    std::cout << "  GET    /network/mesh          - Network mesh status" << std::endl;
    std::cout << "  GET    /network/stats         - Network statistics" << std::endl;
    std::cout << "  GET    /routing/routes        - List all routing rules" << std::endl;
    std::cout << "  POST   /routing/routes        - Create new routing rule" << std::endl;
    std::cout << "  GET    /routing/routes/:id    - Get specific routing rule" << std::endl;
    std::cout << "  PUT    /routing/routes/:id    - Update routing rule" << std::endl;
    std::cout << "  DELETE /routing/routes/:id    - Delete routing rule" << std::endl;
    std::cout << "  GET    /routing/table         - Get complete routing state (debug)" << std::endl;

    std::cout << "\nReady. Press Ctrl+C to stop..." << std::endl;

    // Run until interrupted
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
