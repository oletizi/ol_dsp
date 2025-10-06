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
// MIDI Port wrapper with message queuing
class MidiPort : public juce::MidiInputCallback
{
public:
    MidiPort(const std::string& id, const std::string& name, bool isInput)
        : portId(id), portName(name), isInputPort(isInput) {}

    ~MidiPort() override { close(); }

    bool open() {
        if (isInputPort) {
            auto devices = juce::MidiInput::getAvailableDevices();
            for (const auto& device : devices) {
                if (device.name.toStdString().find(portName) != std::string::npos) {
                    input = juce::MidiInput::openDevice(device.identifier, this);
                    if (input) {
                        input->start();
                        return true;
                    }
                }
            }
        } else {
            auto devices = juce::MidiOutput::getAvailableDevices();
            for (const auto& device : devices) {
                if (device.name.toStdString().find(portName) != std::string::npos) {
                    output = juce::MidiOutput::openDevice(device.identifier);
                    return output != nullptr;
                }
            }
        }
        return false;
    }

    void close() {
        if (input) {
            input->stop();
            input.reset();
        }
        output.reset();
    }

    void sendMessage(const std::vector<uint8_t>& data) {
        if (!output) return;

        // Validate message has data
        if (data.empty()) {
            std::cerr << "Warning: Attempted to send empty MIDI message\n";
            return;
        }

        if (data[0] == 0xF0) {
            // For SysEx messages, validate they end with 0xF7
            if (data.back() != 0xF7) {
                std::cerr << "Warning: Invalid SysEx message (missing 0xF7)\n";
                return;
            }

            // createSysExMessage expects the data WITHOUT F0/F7 wrappers
            // So we need to pass just the middle part
            if (data.size() > 2) {
                output->sendMessageNow(
                    juce::MidiMessage::createSysExMessage(
                        data.data() + 1,  // Skip F0
                        (int)data.size() - 2  // Exclude F0 and F7
                    )
                );
            }
        } else if (data.size() >= 1 && data.size() <= 3) {
            // Valid short MIDI message (1-3 bytes)
            output->sendMessageNow(
                juce::MidiMessage(data.data(), (int)data.size())
            );
        } else {
            std::cerr << "Warning: Invalid MIDI message length: " << data.size() << " bytes\n";
        }
    }

    std::vector<std::vector<uint8_t>> getMessages() {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::vector<std::vector<uint8_t>> result;
        while (!messageQueue.empty()) {
            result.push_back(messageQueue.front());
            messageQueue.pop();
        }
        return result;
    }

    // MidiInputCallback interface
    void handleIncomingMidiMessage(juce::MidiInput* /*source*/,
                                  const juce::MidiMessage& message) override {
        std::vector<uint8_t> data;
        auto rawData = message.getRawData();
        auto size = message.getRawDataSize();

        std::lock_guard<std::mutex> lock(queueMutex);

        // Check if this is a SysEx fragment
        bool startsWithF0 = (size > 0 && rawData[0] == 0xF0);
        bool endsWithF7 = (size > 0 && rawData[size - 1] == 0xF7);
        bool isSysExRelated = message.isSysEx() || startsWithF0 ||
                              (sysexBuffering && size > 0);

        if (isSysExRelated) {
            // Handle SysEx message or fragment
            if (startsWithF0) {
                // Start of new SysEx - initialize buffer
                sysexBuffer.clear();
                sysexBuffer.insert(sysexBuffer.end(), rawData, rawData + size);
                sysexBuffering = true;

                // Check if it's a complete SysEx in one message
                if (endsWithF7) {
                    messageQueue.push(sysexBuffer);
                    sysexBuffer.clear();
                    sysexBuffering = false;
                }
            } else if (sysexBuffering) {
                // Continuation or end of SysEx
                sysexBuffer.insert(sysexBuffer.end(), rawData, rawData + size);

                if (endsWithF7) {
                    // Complete SysEx received
                    messageQueue.push(sysexBuffer);
                    sysexBuffer.clear();
                    sysexBuffering = false;
                }
                // Otherwise keep buffering
            } else if (message.isSysEx()) {
                // JUCE already assembled complete SysEx
                auto sysexData = message.getSysExData();
                auto sysexSize = message.getSysExDataSize();
                data.push_back(0xF0);
                data.insert(data.end(), sysexData, sysexData + sysexSize);
                data.push_back(0xF7);
                messageQueue.push(data);
            }
        } else {
            // Regular MIDI message (non-SysEx)
            data.insert(data.end(), rawData, rawData + size);
            messageQueue.push(data);
        }
    }

private:
    std::string portId;
    std::string portName;
    bool isInputPort;
    std::unique_ptr<juce::MidiInput> input;
    std::unique_ptr<juce::MidiOutput> output;
    std::queue<std::vector<uint8_t>> messageQueue;
    std::mutex queueMutex;

    // SysEx buffering for fragmented messages
    std::vector<uint8_t> sysexBuffer;
    bool sysexBuffering = false;
};

//==============================================================================
// Network MIDI Server class
class NetworkMidiServer
{
public:
    NetworkMidiServer(const NetworkMidi::NodeIdentity& nodeIdentity, int port = 0)
        : identity(nodeIdentity), requestedPort(port), actualPort(0) {}

    ~NetworkMidiServer() {
        stopServer();
    }

    int getActualPort() const {
        return actualPort;
    }

    void startServer() {
        server = std::make_unique<httplib::Server>();

        // Set up routes
        setupRoutes();

        // Start server in a separate thread
        serverThread = std::thread([this]() {
            if (requestedPort == 0) {
                // Auto port allocation - bind to port 0 and get assigned port
                actualPort = server->bind_to_any_port("0.0.0.0");
                if (actualPort < 0) {
                    std::cerr << "Failed to bind to any port" << std::endl;
                    return;
                }
            } else {
                // Specific port requested
                if (!server->bind_to_port("0.0.0.0", requestedPort)) {
                    std::cerr << "Failed to bind to port " << requestedPort << std::endl;
                    return;
                }
                actualPort = requestedPort;
            }

            std::cout << "HTTP Server bound to port " << actualPort << std::endl;

            server->listen_after_bind();
        });

        // Wait a bit for server to start and get actual port
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void stopServer() {
        if (server) {
            server->stop();
        }
        if (serverThread.joinable()) {
            serverThread.join();
        }

        // Clean up ports
        std::lock_guard<std::mutex> lock(portsMutex);
        ports.clear();
    }

private:
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
                .endObject();

            res.set_content(json.toString(), "application/json");
        });

        // List available MIDI ports
        server->Get("/ports", [](const httplib::Request&, httplib::Response& res) {
            JsonBuilder json;
            json.startObject();

            // List inputs
            json.key("inputs").startArray();
            auto inputs = juce::MidiInput::getAvailableDevices();
            for (const auto& device : inputs) {
                json.arrayValue(device.name.toStdString());
            }
            json.endArray();

            // List outputs
            json.key("outputs").startArray();
            auto outputs = juce::MidiOutput::getAvailableDevices();
            for (const auto& device : outputs) {
                json.arrayValue(device.name.toStdString());
            }
            json.endArray();

            json.endObject();

            res.set_content(json.toString(), "application/json");
        });

        // Open MIDI port
        server->Post("/port/:portId", [this](const httplib::Request& req, httplib::Response& res) {
            std::string portId = req.path_params.at("portId");

            try {
                // Parse JSON body (simple parsing)
                std::string name, type;

                size_t namePos = req.body.find("\"name\":\"");
                if (namePos != std::string::npos) {
                    namePos += 8;
                    size_t endPos = req.body.find("\"", namePos);
                    name = req.body.substr(namePos, endPos - namePos);
                }

                size_t typePos = req.body.find("\"type\":\"");
                if (typePos != std::string::npos) {
                    typePos += 8;
                    size_t endPos = req.body.find("\"", typePos);
                    type = req.body.substr(typePos, endPos - typePos);
                }

                bool isInput = (type == "input");
                auto port = std::make_unique<MidiPort>(portId, name, isInput);
                bool success = port->open();

                if (success) {
                    std::lock_guard<std::mutex> lock(portsMutex);
                    ports[portId] = std::move(port);
                }

                JsonBuilder json;
                json.startObject().key("success").value(success).endObject();
                res.set_content(json.toString(), "application/json");
            } catch (const std::exception& e) {
                JsonBuilder json;
                json.startObject().key("error").value(e.what()).endObject();
                res.status = 400;
                res.set_content(json.toString(), "application/json");
            }
        });

        // Close MIDI port
        server->Delete("/port/:portId", [this](const httplib::Request& req, httplib::Response& res) {
            std::string portId = req.path_params.at("portId");

            std::lock_guard<std::mutex> lock(portsMutex);
            bool success = ports.erase(portId) > 0;

            JsonBuilder json;
            json.startObject().key("success").value(success).endObject();
            res.set_content(json.toString(), "application/json");
        });

        // Send MIDI message
        server->Post("/port/:portId/send", [this](const httplib::Request& req, httplib::Response& res) {
            std::string portId = req.path_params.at("portId");

            std::lock_guard<std::mutex> lock(portsMutex);
            auto it = ports.find(portId);
            if (it == ports.end()) {
                JsonBuilder json;
                json.startObject().key("error").value(std::string("Port not found")).endObject();
                res.status = 404;
                res.set_content(json.toString(), "application/json");
                return;
            }

            try {
                // Parse message array from JSON
                std::vector<uint8_t> message;
                size_t msgPos = req.body.find("\"message\":[");
                if (msgPos != std::string::npos) {
                    msgPos += 11;
                    size_t endPos = req.body.find("]", msgPos);
                    std::string msgStr = req.body.substr(msgPos, endPos - msgPos);

                    std::istringstream iss(msgStr);
                    std::string token;
                    while (std::getline(iss, token, ',')) {
                        // Handle empty tokens (from trailing commas or empty arrays)
                        if (!token.empty()) {
                            message.push_back((uint8_t)std::stoi(token));
                        }
                    }
                }

                // Validate message before sending
                if (message.empty()) {
                    JsonBuilder json;
                    json.startObject()
                        .key("error").value(std::string("Invalid MIDI message: empty message"))
                        .key("success").value(false)
                        .endObject();
                    res.status = 400;
                    res.set_content(json.toString(), "application/json");
                    std::cerr << "Rejected empty MIDI message\n";
                    return;
                }

                // Reject incomplete SysEx (single 0xF0 byte)
                if (message.size() == 1 && message[0] == 0xF0) {
                    JsonBuilder json;
                    json.startObject()
                        .key("error").value(std::string("Invalid MIDI message: incomplete SysEx (0xF0 without 0xF7)"))
                        .key("success").value(false)
                        .endObject();
                    res.status = 400;
                    res.set_content(json.toString(), "application/json");
                    std::cerr << "Rejected incomplete SysEx (single 0xF0)\n";
                    return;
                }

                it->second->sendMessage(message);

                JsonBuilder json;
                json.startObject().key("success").value(true).endObject();
                res.set_content(json.toString(), "application/json");
            } catch (const std::exception& e) {
                JsonBuilder json;
                json.startObject().key("error").value(e.what()).endObject();
                res.status = 400;
                res.set_content(json.toString(), "application/json");
            }
        });

        // Receive MIDI messages
        server->Get("/port/:portId/messages", [this](const httplib::Request& req, httplib::Response& res) {
            std::string portId = req.path_params.at("portId");

            std::lock_guard<std::mutex> lock(portsMutex);
            auto it = ports.find(portId);
            if (it == ports.end()) {
                JsonBuilder json;
                json.startObject().key("error").value(std::string("Port not found")).endObject();
                res.status = 404;
                res.set_content(json.toString(), "application/json");
                return;
            }

            auto messages = it->second->getMessages();

            JsonBuilder json;
            json.startObject().key("messages").startArray();

            for (const auto& msg : messages) {
                json.startArray();
                for (uint8_t byte : msg) {
                    json.arrayValue((int)byte);
                }
                json.endArray();
            }

            json.endArray().endObject();

            res.set_content(json.toString(), "application/json");
        });
    }

    const NetworkMidi::NodeIdentity& identity;
    int requestedPort;
    int actualPort;
    std::unique_ptr<httplib::Server> server;
    std::thread serverThread;
    std::map<std::string, std::unique_ptr<MidiPort>> ports;
    std::mutex portsMutex;
};

//==============================================================================
int main(int argc, char* argv[])
{
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\nNetwork MIDI Server v1.0" << std::endl;
    std::cout << "========================" << std::endl;

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
        std::cout << "  Port: auto-assigned (bind to port 0)" << std::endl;
    } else {
        std::cout << "  Port: " << port << std::endl;
    }

    NetworkMidiServer server(identity, port);
    server.startServer();

    int actualPort = server.getActualPort();
    std::cout << "\nServer running on port " << actualPort << std::endl;
    std::cout << "Node: " << identity.getNodeName().toStdString() << std::endl;
    std::cout << "UUID: " << identity.getNodeId().toString().toStdString() << std::endl;
    std::cout << "Instance dir: " << instanceManager->getInstanceDirectory()
                                       .getFullPathName().toStdString() << std::endl;

    // List local MIDI devices
    auto inputs = juce::MidiInput::getAvailableDevices();
    auto outputs = juce::MidiOutput::getAvailableDevices();
    std::cout << "\nLocal MIDI devices: " << (inputs.size() + outputs.size()) << std::endl;

    std::cout << "\nReady. Press Ctrl+C to stop..." << std::endl;

    // Run until interrupted
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
