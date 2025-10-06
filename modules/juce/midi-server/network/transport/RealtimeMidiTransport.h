#pragma once

#include "RealtimeMidiBuffer.h"
#include <juce_core/juce_core.h>
#include <atomic>

namespace NetworkMidi {

/**
 * Real-time MIDI UDP transport thread.
 *
 * Priority: realtimeAudio (highest user-space priority)
 * Latency target: <1ms end-to-end
 * Delivery: Best-effort (no retries)
 *
 * This transport uses non-blocking UDP I/O to send real-time MIDI messages
 * with minimal latency. Messages are batched in groups of up to 32 for efficiency.
 *
 * Wire format:
 * - 2 bytes magic: 'M' 'R' (MIDI Real-time)
 * - 1 byte length
 * - 2 bytes device ID (big-endian)
 * - 4 bytes timestamp (microseconds)
 * - N bytes MIDI data (up to 4)
 */
class RealtimeMidiTransport : public juce::Thread {
public:
    /**
     * Create real-time MIDI transport.
     *
     * @param buf Reference to shared ring buffer
     * @param host Remote host address
     * @param port Remote UDP port
     */
    RealtimeMidiTransport(RealtimeMidiBuffer& buf, juce::String host, int port);
    ~RealtimeMidiTransport() override;

    void run() override;

    struct Stats {
        uint64_t packetsSent{0};
        uint64_t packetsReceived{0};
        uint64_t sendFailures{0};
        uint64_t receiveErrors{0};
    };

    Stats getStats() const;

private:
    RealtimeMidiBuffer& buffer;
    juce::DatagramSocket udpSocket;
    juce::String remoteHost;
    int remotePort;

    std::atomic<uint64_t> packetsSent{0};
    std::atomic<uint64_t> packetsReceived{0};
    std::atomic<uint64_t> sendFailures{0};
    std::atomic<uint64_t> receiveErrors{0};

    void sendPacket(const RealtimeMidiBuffer::MidiPacket& packet);
    void receivePackets();
};

} // namespace NetworkMidi
