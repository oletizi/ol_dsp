#include "NonRealtimeMidiTransport.h"
#include <algorithm>

namespace NetworkMidi {

NonRealtimeMidiTransport::NonRealtimeMidiTransport(juce::String host, int port)
    : juce::Thread("NonRealtimeMidiTCP"), remoteHost(host), remotePort(port)
{
    // Normal priority (5) is fine for non-real-time
    setPriority(juce::Thread::Priority::normal);
}

NonRealtimeMidiTransport::~NonRealtimeMidiTransport() {
    signalThreadShouldExit();
    dataAvailable.signal();  // Wake up if waiting
    stopThread(2000);  // 2 second timeout
}

void NonRealtimeMidiTransport::sendMessage(const juce::MidiMessage& msg, uint16_t deviceId) {
    MidiPacket packet;
    packet.data.resize(msg.getRawDataSize());
    std::memcpy(packet.data.data(), msg.getRawData(), msg.getRawDataSize());
    packet.deviceId = deviceId;
    packet.requiresAck = true;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        packet.sequenceNumber = nextSequenceNumber++;
        sendQueue.push_back(std::move(packet));
    }

    dataAvailable.signal();
}

std::vector<NonRealtimeMidiTransport::MidiPacket>
NonRealtimeMidiTransport::getReceivedMessages() {
    std::lock_guard<std::mutex> lock(queueMutex);
    std::vector<MidiPacket> result;
    result.reserve(receiveQueue.size());

    while (!receiveQueue.empty()) {
        result.push_back(std::move(receiveQueue.front()));
        receiveQueue.pop_front();
    }

    return result;
}

void NonRealtimeMidiTransport::run() {
    juce::Logger::writeToLog("NonRealtimeMidiTransport: Started");

    while (!threadShouldExit()) {
        if (!connected) {
            attemptConnection();
            if (!connected) {
                juce::Thread::sleep(1000);  // Retry every 1s
                continue;
            }
        }

        // Process send queue
        processSendQueue();

        // Receive incoming data (non-blocking with timeout)
        receiveData();

        // Check for ACK timeouts and retry
        retryUnacknowledged();

        // Wait for more data or timeout
        dataAvailable.wait(10);  // 10ms poll interval
    }

    juce::Logger::writeToLog("NonRealtimeMidiTransport: Stopped");
}

void NonRealtimeMidiTransport::attemptConnection() {
    juce::Logger::writeToLog("NonRealtimeMidiTransport: Connecting to " +
                            remoteHost + ":" + juce::String(remotePort));

    if (tcpSocket.connect(remoteHost, remotePort, 2000)) {  // 2s timeout
        connected = true;
        juce::Logger::writeToLog("NonRealtimeMidiTransport: Connected");
    } else {
        juce::Logger::writeToLog("NonRealtimeMidiTransport: Connection failed");
    }
}

void NonRealtimeMidiTransport::processSendQueue() {
    std::vector<MidiPacket> toSend;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!sendQueue.empty() && toSend.size() < 16) {
            toSend.push_back(std::move(sendQueue.front()));
            sendQueue.pop_front();
        }
    }

    for (auto& packet : toSend) {
        packet.sentTime = juce::Time::getCurrentTime();
        sendTcpPacket(packet);

        if (packet.requiresAck) {
            pendingAcks[packet.sequenceNumber] = packet;
        }
    }
}

void NonRealtimeMidiTransport::sendTcpPacket(const MidiPacket& packet) {
    // Fragment if SysEx is too large (>1KB chunks)
    const size_t MAX_FRAGMENT = 1024;
    size_t offset = 0;

    while (offset < packet.data.size()) {
        size_t fragmentSize = std::min(MAX_FRAGMENT, packet.data.size() - offset);

        // Build TCP frame: [header][fragment]
        std::vector<uint8_t> frame;
        frame.push_back('M');  // Magic
        frame.push_back('N');  // Non-real-time
        frame.push_back(static_cast<uint8_t>((packet.sequenceNumber >> 24) & 0xFF));
        frame.push_back(static_cast<uint8_t>((packet.sequenceNumber >> 16) & 0xFF));
        frame.push_back(static_cast<uint8_t>((packet.sequenceNumber >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(packet.sequenceNumber & 0xFF));
        frame.push_back(static_cast<uint8_t>((fragmentSize >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(fragmentSize & 0xFF));
        frame.insert(frame.end(), packet.data.begin() + offset,
                    packet.data.begin() + offset + fragmentSize);

        int sent = tcpSocket.write(frame.data(), static_cast<int>(frame.size()));
        if (sent != static_cast<int>(frame.size())) {
            // Connection error - mark for retry
            juce::Logger::writeToLog("NonRealtimeMidiTransport: Send failed, disconnecting");
            connected = false;
            failures.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        fragmentsSent.fetch_add(1, std::memory_order_relaxed);
        offset += fragmentSize;
    }

    messagesSent.fetch_add(1, std::memory_order_relaxed);
}

void NonRealtimeMidiTransport::receiveData() {
    // Set socket to non-blocking for read
    tcpSocket.waitUntilReady(false, 10);  // 10ms timeout

    uint8_t headerBuffer[8];

    // Read TCP frame header
    int received = tcpSocket.read(headerBuffer, 8, false);
    if (received != 8) {
        if (received > 0) {
            juce::Logger::writeToLog("NonRealtimeMidiTransport: Incomplete header");
            connected = false;
        }
        return;
    }

    // Parse header
    if (headerBuffer[0] != 'M' || headerBuffer[1] != 'N') {
        // Protocol error
        juce::Logger::writeToLog("NonRealtimeMidiTransport: Invalid magic bytes");
        connected = false;
        return;
    }

    uint32_t seqNum = (static_cast<uint32_t>(headerBuffer[2]) << 24) |
                      (static_cast<uint32_t>(headerBuffer[3]) << 16) |
                      (static_cast<uint32_t>(headerBuffer[4]) << 8) |
                      static_cast<uint32_t>(headerBuffer[5]);
    uint16_t fragmentSize = (static_cast<uint16_t>(headerBuffer[6]) << 8) |
                            static_cast<uint16_t>(headerBuffer[7]);

    // Read fragment data
    std::vector<uint8_t> fragmentData(fragmentSize);
    received = tcpSocket.read(fragmentData.data(), fragmentSize, true);  // Blocking read
    if (received != fragmentSize) {
        juce::Logger::writeToLog("NonRealtimeMidiTransport: Incomplete fragment");
        connected = false;
        return;
    }

    fragmentsReceived.fetch_add(1, std::memory_order_relaxed);

    // Send ACK
    sendAck(seqNum);

    // Reassemble and enqueue
    reassembleFragment(seqNum, std::move(fragmentData));
}

void NonRealtimeMidiTransport::sendAck(uint32_t seqNum) {
    uint8_t ack[7] = {'A', 'C', 'K',
                     static_cast<uint8_t>((seqNum >> 24) & 0xFF),
                     static_cast<uint8_t>((seqNum >> 16) & 0xFF),
                     static_cast<uint8_t>((seqNum >> 8) & 0xFF),
                     static_cast<uint8_t>(seqNum & 0xFF)};
    tcpSocket.write(ack, 7);
}

void NonRealtimeMidiTransport::retryUnacknowledged() {
    auto now = juce::Time::getCurrentTime();
    std::vector<uint32_t> toRetry;

    for (auto& [seqNum, pending] : pendingAcks) {
        auto elapsed = now.toMilliseconds() - pending.sentTime.toMilliseconds();

        if (elapsed > 1000 && pending.retryCount < 3) {  // 1 second timeout, max 3 retries
            toRetry.push_back(seqNum);
        } else if (pending.retryCount >= 3) {
            // Give up - log error
            juce::Logger::writeToLog("NonRealtimeMidiTransport: Failed to deliver packet " +
                                    juce::String(seqNum) + " after 3 retries");
            pendingAcks.erase(seqNum);
            failures.fetch_add(1, std::memory_order_relaxed);
        }
    }

    for (uint32_t seqNum : toRetry) {
        auto& pending = pendingAcks[seqNum];
        pending.retryCount++;
        pending.sentTime = juce::Time::getCurrentTime();
        sendTcpPacket(pending);
        retries.fetch_add(1, std::memory_order_relaxed);
    }
}

void NonRealtimeMidiTransport::reassembleFragment(uint32_t seqNum,
                                                   std::vector<uint8_t>&& fragmentData) {
    // For simplicity, assume single-fragment messages for now
    // Full implementation would track multi-fragment reassembly
    MidiPacket packet;
    packet.data = std::move(fragmentData);
    packet.sequenceNumber = seqNum;
    packet.deviceId = 0;  // TODO: Extract from packet

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        receiveQueue.push_back(std::move(packet));
    }

    messagesReceived.fetch_add(1, std::memory_order_relaxed);
}

NonRealtimeMidiTransport::Stats NonRealtimeMidiTransport::getStats() const {
    Stats s;
    s.messagesSent = messagesSent.load(std::memory_order_relaxed);
    s.messagesReceived = messagesReceived.load(std::memory_order_relaxed);
    s.fragmentsSent = fragmentsSent.load(std::memory_order_relaxed);
    s.fragmentsReceived = fragmentsReceived.load(std::memory_order_relaxed);
    s.retries = retries.load(std::memory_order_relaxed);
    s.failures = failures.load(std::memory_order_relaxed);
    return s;
}

} // namespace NetworkMidi
