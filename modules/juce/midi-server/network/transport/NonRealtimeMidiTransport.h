#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <deque>
#include <mutex>
#include <map>
#include <vector>
#include <atomic>

namespace NetworkMidi {

/**
 * Non-real-time MIDI transport using TCP for reliable delivery.
 *
 * Features:
 * - Guaranteed delivery (ACK/retry)
 * - SysEx fragmentation (1KB chunks)
 * - Flow control
 * - 100% reliability target
 *
 * This transport uses TCP for reliable delivery of SysEx and bulk transfers.
 * Large messages are fragmented into 1KB chunks and reassembled on the receiver.
 * ACK/retry mechanism ensures 100% delivery (up to 3 retries with 1s timeout).
 *
 * Wire format:
 * - 2 bytes magic: 'M' 'N' (MIDI Non-real-time)
 * - 4 bytes sequence number (big-endian)
 * - 2 bytes fragment size (big-endian)
 * - N bytes fragment data
 *
 * ACK format:
 * - 3 bytes: 'A' 'C' 'K'
 * - 4 bytes sequence number
 */
class NonRealtimeMidiTransport : public juce::Thread {
public:
    struct MidiPacket {
        std::vector<uint8_t> data;
        uint16_t deviceId;
        uint32_t sequenceNumber;
        bool requiresAck;
        juce::Time sentTime;
        int retryCount{0};
    };

    /**
     * Create non-real-time MIDI transport.
     *
     * @param host Remote host address
     * @param port Remote TCP port
     */
    NonRealtimeMidiTransport(juce::String host, int port);
    ~NonRealtimeMidiTransport() override;

    /**
     * Send MIDI message (thread-safe).
     *
     * @param msg MIDI message to send
     * @param deviceId Device ID
     */
    void sendMessage(const juce::MidiMessage& msg, uint16_t deviceId);

    /**
     * Get received messages (thread-safe).
     *
     * @return Vector of received packets
     */
    std::vector<MidiPacket> getReceivedMessages();

    void run() override;

    struct Stats {
        uint64_t messagesSent{0};
        uint64_t messagesReceived{0};
        uint64_t fragmentsSent{0};
        uint64_t fragmentsReceived{0};
        uint64_t retries{0};
        uint64_t failures{0};
    };

    Stats getStats() const;

private:
    std::mutex queueMutex;
    std::deque<MidiPacket> sendQueue;
    std::deque<MidiPacket> receiveQueue;
    juce::WaitableEvent dataAvailable;

    juce::StreamingSocket tcpSocket;
    juce::String remoteHost;
    int remotePort;
    bool connected{false};

    uint32_t nextSequenceNumber{0};
    std::map<uint32_t, MidiPacket> pendingAcks;

    std::atomic<uint64_t> messagesSent{0};
    std::atomic<uint64_t> messagesReceived{0};
    std::atomic<uint64_t> fragmentsSent{0};
    std::atomic<uint64_t> fragmentsReceived{0};
    std::atomic<uint64_t> retries{0};
    std::atomic<uint64_t> failures{0};

    // Reassembly buffer for fragmented messages
    struct ReassemblyState {
        std::vector<uint8_t> data;
        size_t expectedSize{0};
        juce::Time firstFragmentTime;
    };
    std::map<uint32_t, ReassemblyState> reassemblyBuffer;

    void attemptConnection();
    void processSendQueue();
    void sendTcpPacket(const MidiPacket& packet);
    void receiveData();
    void sendAck(uint32_t seqNum);
    void retryUnacknowledged();
    void reassembleFragment(uint32_t seqNum, std::vector<uint8_t>&& fragmentData);
};

} // namespace NetworkMidi
