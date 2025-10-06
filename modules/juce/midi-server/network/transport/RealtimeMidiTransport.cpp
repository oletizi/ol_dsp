#include "RealtimeMidiTransport.h"
#include <cstring>

namespace NetworkMidi {

RealtimeMidiTransport::RealtimeMidiTransport(RealtimeMidiBuffer& buf,
                                              juce::String host, int port)
    : juce::Thread("RealtimeMidiUDP"), buffer(buf),
      remoteHost(host), remotePort(port)
{
    // Set real-time priority
    setPriority(juce::Thread::Priority::highest);
}

RealtimeMidiTransport::~RealtimeMidiTransport() {
    signalThreadShouldExit();
    stopThread(2000);  // 2 second timeout
}

void RealtimeMidiTransport::run() {
    // Bind socket to any available port (OS assigns)
    if (!udpSocket.bindToPort(0)) {
        juce::Logger::writeToLog("RealtimeMidiTransport: Failed to bind UDP socket");
        return;
    }

    juce::Logger::writeToLog("RealtimeMidiTransport: Started on port " +
                            juce::String(udpSocket.getBoundPort()));

    RealtimeMidiBuffer::MidiPacket packets[32];  // Batch buffer

    while (!threadShouldExit()) {
        // Read batch from ring buffer (lock-free)
        int count = buffer.readBatch(packets, 32);

        if (count > 0) {
            // Send each packet via UDP (non-blocking)
            for (int i = 0; i < count; ++i) {
                sendPacket(packets[i]);
            }
        } else {
            // No messages - yield CPU briefly
            // 1ms sleep is acceptable for real-time (still <1ms latency target)
            juce::Thread::sleep(1);
        }

        // Also receive incoming UDP messages (non-blocking)
        receivePackets();
    }

    juce::Logger::writeToLog("RealtimeMidiTransport: Stopped");
}

void RealtimeMidiTransport::sendPacket(const RealtimeMidiBuffer::MidiPacket& packet) {
    // Serialize packet (fixed-size binary format)
    uint8_t wireFormat[16];
    wireFormat[0] = 'M';  // Magic byte
    wireFormat[1] = 'R';  // Real-time marker
    wireFormat[2] = packet.length;
    wireFormat[3] = static_cast<uint8_t>((packet.deviceId >> 8) & 0xFF);
    wireFormat[4] = static_cast<uint8_t>(packet.deviceId & 0xFF);
    std::memcpy(&wireFormat[5], &packet.timestamp, 4);
    std::memcpy(&wireFormat[9], packet.data, packet.length);

    int totalSize = 9 + packet.length;

    // Send UDP (best-effort, no retry)
    int sent = udpSocket.write(remoteHost, remotePort, wireFormat, totalSize);
    if (sent == totalSize) {
        packetsSent.fetch_add(1, std::memory_order_relaxed);
    } else {
        sendFailures.fetch_add(1, std::memory_order_relaxed);
    }
}

void RealtimeMidiTransport::receivePackets() {
    uint8_t receiveBuffer[1024];
    juce::String senderHost;
    int senderPort;

    // Non-blocking read (returns immediately if no data)
    int received = udpSocket.read(receiveBuffer, sizeof(receiveBuffer), false,
                                   senderHost, senderPort);

    if (received > 0) {
        // Verify magic bytes
        if (received >= 9 && receiveBuffer[0] == 'M' && receiveBuffer[1] == 'R') {
            packetsReceived.fetch_add(1, std::memory_order_relaxed);

            // Parse packet (implementation-specific - could enqueue to input buffer)
            // For now, just count received packets
        } else {
            receiveErrors.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

RealtimeMidiTransport::Stats RealtimeMidiTransport::getStats() const {
    Stats s;
    s.packetsSent = packetsSent.load(std::memory_order_relaxed);
    s.packetsReceived = packetsReceived.load(std::memory_order_relaxed);
    s.sendFailures = sendFailures.load(std::memory_order_relaxed);
    s.receiveErrors = receiveErrors.load(std::memory_order_relaxed);
    return s;
}

} // namespace NetworkMidi
