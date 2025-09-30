/**
 * JUCE-based MIDI HTTP Server using cpp-httplib
 *
 * Provides a robust HTTP API for Node.js to proxy MIDI operations through JUCE,
 * avoiding the limitations of Node.js MIDI libraries.
 */

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include "httplib.h"

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
    JsonBuilder& startObject() { ss << "{"; firstItem = true; return *this; }
    JsonBuilder& endObject() { ss << "}"; return *this; }
    JsonBuilder& startArray() { ss << "["; firstItem = true; return *this; }
    JsonBuilder& endArray() { ss << "]"; return *this; }

    JsonBuilder& key(const std::string& k) {
        if (!firstItem) ss << ",";
        ss << "\"" << k << "\":";
        firstItem = false;
        return *this;
    }

    JsonBuilder& value(const std::string& v) {
        ss << "\"" << v << "\"";
        return *this;
    }

    JsonBuilder& value(bool b) {
        ss << (b ? "true" : "false");
        return *this;
    }

    JsonBuilder& value(int i) {
        ss << i;
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

        if (data.size() > 0 && data[0] == 0xF0) {
            output->sendMessageNow(
                juce::MidiMessage::createSysExMessage(data.data(), (int)data.size())
            );
        } else {
            output->sendMessageNow(
                juce::MidiMessage(data.data(), (int)data.size())
            );
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
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                  const juce::MidiMessage& message) override {
        std::vector<uint8_t> data;

        if (message.isSysEx()) {
            auto sysexData = message.getSysExData();
            auto size = message.getSysExDataSize();
            // Add F0 header and F7 trailer
            data.push_back(0xF0);
            data.insert(data.end(), sysexData, sysexData + size);
            data.push_back(0xF7);
        } else {
            auto rawData = message.getRawData();
            auto size = message.getRawDataSize();
            data.insert(data.end(), rawData, rawData + size);
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        messageQueue.push(data);
    }

private:
    std::string portId;
    std::string portName;
    bool isInputPort;
    std::unique_ptr<juce::MidiInput> input;
    std::unique_ptr<juce::MidiOutput> output;
    std::queue<std::vector<uint8_t>> messageQueue;
    std::mutex queueMutex;
};

//==============================================================================
// HTTP Server class
class MidiHttpServer
{
public:
    MidiHttpServer(int port) : serverPort(port) {}

    ~MidiHttpServer() {
        stopServer();
    }

    void startServer() {
        server = std::make_unique<httplib::Server>();

        // Set up routes
        server->Get("/health", [this](const httplib::Request&, httplib::Response& res) {
            JsonBuilder json;
            json.startObject().key("status").value(std::string("ok")).endObject();
            res.set_content(json.toString(), "application/json");
        });

        server->Get("/ports", [this](const httplib::Request&, httplib::Response& res) {
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

        server->Delete("/port/:portId", [this](const httplib::Request& req, httplib::Response& res) {
            std::string portId = req.path_params.at("portId");

            std::lock_guard<std::mutex> lock(portsMutex);
            bool success = ports.erase(portId) > 0;

            JsonBuilder json;
            json.startObject().key("success").value(success).endObject();
            res.set_content(json.toString(), "application/json");
        });

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
                        message.push_back((uint8_t)std::stoi(token));
                    }
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

        // Start server in a separate thread
        serverThread = std::thread([this]() {
            std::cout << "HTTP Server listening on port " << serverPort << std::endl;
            server->listen("0.0.0.0", serverPort);
        });
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
    int serverPort;
    std::unique_ptr<httplib::Server> server;
    std::thread serverThread;
    std::map<std::string, std::unique_ptr<MidiPort>> ports;
    std::mutex portsMutex;
};

//==============================================================================
int main(int argc, char* argv[])
{
    // Parse port from command line
    int port = 7777;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\nJUCE MIDI HTTP Server (cpp-httplib)" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "Starting server on port " << port << "..." << std::endl;

    MidiHttpServer server(port);
    server.startServer();

    std::cout << "Server running. Press Ctrl+C to stop..." << std::endl;

    // Run until interrupted
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}