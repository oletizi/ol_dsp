#include "UdpMidiTransport.h"
#include <sstream>

namespace NetworkMidi {

UdpMidiTransport::UdpMidiTransport(int port)
    : port(port)
    , nextSequence(0)
    , running(false)
{
    socket = std::make_unique<juce::DatagramSocket>();
}

UdpMidiTransport::~UdpMidiTransport() {
    stop();
}

bool UdpMidiTransport::start() {
    if (running.load()) {
        return true;  // Already running
    }

    // Bind socket
    if (!socket->bindToPort(port)) {
        if (onError) {
            onError("Failed to bind UDP socket to port " + juce::String(port));
        }
        return false;
    }

    // Get actual port (in case port was 0)
    port = socket->getBoundPort();

    // Start receive thread
    running.store(true);
    receiveThread = std::make_unique<ReceiveThread>(*this);
    receiveThread->startThread();

    return true;
}

void UdpMidiTransport::stop() {
    if (!running.load()) {
        return;  // Already stopped
    }

    // Stop receive thread
    running.store(false);

    if (receiveThread != nullptr) {
        // Signal thread to stop by closing socket (unblocks receive)
        socket->shutdown();

        // Wait for thread to finish
        receiveThread->stopThread(2000);
        receiveThread.reset();
    }

    // Close socket
    socket.reset();
    socket = std::make_unique<juce::DatagramSocket>();
}

bool UdpMidiTransport::sendMessage(
    const juce::Uuid& destNode,
    const juce::String& destAddress,
    int destPort,
    uint16_t deviceId,
    const std::vector<uint8_t>& midiData)
{
    // Create packet
    MidiPacket packet = MidiPacket::createDataPacket(
        myNodeId,
        destNode,
        deviceId,
        midiData,
        nextSequence++
    );

    return sendPacket(packet, destAddress, destPort);
}

bool UdpMidiTransport::sendPacket(
    const MidiPacket& packet,
    const juce::String& destAddress,
    int destPort)
{
    if (!running.load()) {
        return false;
    }

    // Serialize packet
    auto serialized = packet.serialize();

    // Send via socket
    int bytesSent = socket->write(destAddress, destPort, serialized.data(),
                                   static_cast<int>(serialized.size()));

    // Update statistics
    {
        juce::ScopedLock lock(statsLock);
        if (bytesSent > 0) {
            stats.packetsSent++;
            stats.bytesSent += bytesSent;
        } else {
            stats.sendErrors++;
        }
    }

    if (bytesSent < 0) {
        if (onError) {
            onError("Failed to send packet to " + destAddress + ":" + juce::String(destPort));
        }
        return false;
    }

    return true;
}

void UdpMidiTransport::receiveLoop() {
    constexpr size_t BUFFER_SIZE = 2048;  // Large enough for most MIDI messages
    uint8_t buffer[BUFFER_SIZE];

    while (running.load()) {
        // Receive packet (blocking with timeout)
        juce::String sourceAddress;
        int sourcePort = 0;

        int bytesRead = socket->read(buffer, BUFFER_SIZE, false, sourceAddress, sourcePort);

        if (bytesRead > 0) {
            // Update statistics
            {
                juce::ScopedLock lock(statsLock);
                stats.bytesReceived += bytesRead;
            }

            // Try to deserialize packet
            MidiPacket packet;
            if (MidiPacket::tryDeserialize(buffer, bytesRead, packet)) {
                // Update statistics
                {
                    juce::ScopedLock lock(statsLock);
                    stats.packetsReceived++;
                }

                // Invoke callback
                if (onPacketReceived) {
                    onPacketReceived(packet, sourceAddress, sourcePort);
                }
            } else {
                // Invalid packet
                {
                    juce::ScopedLock lock(statsLock);
                    stats.invalidPackets++;
                    stats.receiveErrors++;
                }

                if (onError) {
                    onError("Received invalid packet from " + sourceAddress +
                           ":" + juce::String(sourcePort));
                }
            }
        } else if (bytesRead < 0) {
            // Error receiving
            if (running.load()) {  // Only report if we're supposed to be running
                juce::ScopedLock lock(statsLock);
                stats.receiveErrors++;
            }
        }

        // Small sleep to prevent tight loop on errors
        if (bytesRead <= 0) {
            juce::Thread::sleep(1);
        }
    }
}

UdpMidiTransport::Statistics UdpMidiTransport::getStatistics() const {
    juce::ScopedLock lock(statsLock);
    return stats;
}

void UdpMidiTransport::resetStatistics() {
    juce::ScopedLock lock(statsLock);
    stats = Statistics();
}

} // namespace NetworkMidi
