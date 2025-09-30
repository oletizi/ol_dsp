/**
 * JUCE-based MIDI HTTP Server
 *
 * Provides an HTTP API for Node.js to proxy MIDI operations through JUCE,
 * avoiding the limitations of Node.js MIDI libraries.
 */

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

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

    ~MidiPort() { close(); }

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
// Simple HTTP Server using JUCE's network classes
class MidiHttpServer : public juce::Thread
{
public:
    MidiHttpServer(int port)
        : juce::Thread("HTTP Server"), serverPort(port) {}

    ~MidiHttpServer() {
        stopServer();
    }

    void startServer() {
        serverSocket = std::make_unique<juce::StreamingSocket>();
        if (serverSocket->createListener(serverPort, "0.0.0.0")) {
            std::cout << "HTTP Server listening on port " << serverPort << std::endl;
            startThread();
        } else {
            std::cerr << "Failed to create listener on port " << serverPort << std::endl;
        }
    }

    void stopServer() {
        signalThreadShouldExit();
        if (serverSocket) {
            serverSocket->close();
        }
        stopThread(5000);

        // Clean up ports
        std::lock_guard<std::mutex> lock(portsMutex);
        ports.clear();
    }

    void run() override {
        while (!threadShouldExit()) {
            auto clientSocket = std::unique_ptr<juce::StreamingSocket>(
                serverSocket->waitForNextConnection()
            );

            if (clientSocket && clientSocket->isConnected()) {
                handleRequest(std::move(clientSocket));
            }
        }
    }

private:
    void handleRequest(std::unique_ptr<juce::StreamingSocket> socket) {
        char buffer[4096];
        int bytesRead = socket->read(buffer, sizeof(buffer) - 1, false);

        if (bytesRead <= 0) return;

        buffer[bytesRead] = '\0';
        std::string request(buffer);

        // Parse HTTP request
        std::istringstream iss(request);
        std::string method, path, version;
        iss >> method >> path >> version;

        // Extract body if present
        std::string body;
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            body = request.substr(bodyStart + 4);
        }

        // Handle routes
        std::string response;
        if (method == "GET" && path == "/health") {
            response = handleHealth();
        } else if (method == "GET" && path == "/ports") {
            response = handleListPorts();
        } else if (method == "POST" && path.substr(0, 6) == "/port/") {
            std::string portId = path.substr(6);
            size_t slashPos = portId.find('/');
            if (slashPos != std::string::npos) {
                std::string action = portId.substr(slashPos + 1);
                portId = portId.substr(0, slashPos);

                if (action == "send") {
                    response = handleSendMessage(portId, body);
                }
            } else {
                response = handleOpenPort(portId, body);
            }
        } else if (method == "GET" && path.substr(0, 6) == "/port/") {
            std::string portId = path.substr(6);
            size_t slashPos = portId.find('/');
            if (slashPos != std::string::npos) {
                portId = portId.substr(0, slashPos);
                response = handleGetMessages(portId);
            }
        } else if (method == "DELETE" && path.substr(0, 6) == "/port/") {
            std::string portId = path.substr(6);
            response = handleClosePort(portId);
        } else {
            response = createHttpResponse(404, "{\"error\":\"Not found\"}");
        }

        socket->write(response.c_str(), (int)response.length());
    }

    std::string handleHealth() {
        return createHttpResponse(200, "{\"status\":\"ok\"}");
    }

    std::string handleListPorts() {
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

        return createHttpResponse(200, json.toString());
    }

    std::string handleOpenPort(const std::string& portId, const std::string& body) {
        // Parse JSON body (simple parsing)
        std::string name, type;

        size_t namePos = body.find("\"name\":\"");
        if (namePos != std::string::npos) {
            namePos += 8;
            size_t endPos = body.find("\"", namePos);
            name = body.substr(namePos, endPos - namePos);
        }

        size_t typePos = body.find("\"type\":\"");
        if (typePos != std::string::npos) {
            typePos += 8;
            size_t endPos = body.find("\"", typePos);
            type = body.substr(typePos, endPos - typePos);
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

        return createHttpResponse(200, json.toString());
    }

    std::string handleClosePort(const std::string& portId) {
        std::lock_guard<std::mutex> lock(portsMutex);
        bool success = ports.erase(portId) > 0;

        JsonBuilder json;
        json.startObject().key("success").value(success).endObject();

        return createHttpResponse(200, json.toString());
    }

    std::string handleSendMessage(const std::string& portId, const std::string& body) {
        std::lock_guard<std::mutex> lock(portsMutex);
        auto it = ports.find(portId);
        if (it == ports.end()) {
            return createHttpResponse(404, "{\"error\":\"Port not found\"}");
        }

        // Parse message array from JSON
        std::vector<uint8_t> message;
        size_t msgPos = body.find("\"message\":[");
        if (msgPos != std::string::npos) {
            msgPos += 11;
            size_t endPos = body.find("]", msgPos);
            std::string msgStr = body.substr(msgPos, endPos - msgPos);

            std::istringstream iss(msgStr);
            std::string token;
            while (std::getline(iss, token, ',')) {
                message.push_back((uint8_t)std::stoi(token));
            }
        }

        it->second->sendMessage(message);

        return createHttpResponse(200, "{\"success\":true}");
    }

    std::string handleGetMessages(const std::string& portId) {
        std::lock_guard<std::mutex> lock(portsMutex);
        auto it = ports.find(portId);
        if (it == ports.end()) {
            return createHttpResponse(404, "{\"error\":\"Port not found\"}");
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

        return createHttpResponse(200, json.toString());
    }

    std::string createHttpResponse(int status, const std::string& body) {
        std::stringstream response;
        response << "HTTP/1.1 " << status << " OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "\r\n";
        response << body;
        return response.str();
    }

    int serverPort;
    std::unique_ptr<juce::StreamingSocket> serverSocket;
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

    std::cout << "\nJUCE MIDI HTTP Server" << std::endl;
    std::cout << "=====================" << std::endl;
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