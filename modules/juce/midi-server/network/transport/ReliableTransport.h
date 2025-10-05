#pragma once

#include "../core/MidiPacket.h"
#include "UdpMidiTransport.h"
#include <JuceHeader.h>
#include <map>
#include <memory>
#include <functional>

namespace NetworkMidi {

/**
 * Reliable delivery layer for MIDI messages (particularly SysEx).
 *
 * Features:
 * - ACK/NACK mechanism for reliable packets
 * - Automatic retry with exponential backoff
 * - Timeout detection (default 100ms)
 * - Maximum retry limit (default 3 retries)
 * - Delivery confirmation callbacks
 */
class ReliableTransport {
public:
    /**
     * Configuration for reliable delivery.
     */
    struct Config {
        int timeoutMs = 100;        // ACK timeout in milliseconds
        int maxRetries = 3;         // Maximum number of retry attempts
        int retryBackoffMs = 50;    // Additional delay per retry
    };

    /**
     * Constructor.
     * @param transport Underlying UDP transport
     * @param config Reliability configuration
     */
    explicit ReliableTransport(
        UdpMidiTransport& transport,
        const Config& config = Config()
    );

    /**
     * Destructor - cancels all pending sends.
     */
    ~ReliableTransport();

    // Non-copyable
    ReliableTransport(const ReliableTransport&) = delete;
    ReliableTransport& operator=(const ReliableTransport&) = delete;

    /**
     * Send a message reliably (with ACK/retry).
     * @param packet The packet to send
     * @param destAddress Destination IP address
     * @param destPort Destination port
     * @param onSuccess Callback on successful delivery (ACK received)
     * @param onFailure Callback on delivery failure (timeout/max retries)
     */
    void sendReliable(
        const MidiPacket& packet,
        const juce::String& destAddress,
        int destPort,
        std::function<void()> onSuccess = nullptr,
        std::function<void(const juce::String& reason)> onFailure = nullptr
    );

    /**
     * Send a message unreliably (fire-and-forget).
     * @param packet The packet to send
     * @param destAddress Destination IP address
     * @param destPort Destination port
     */
    void sendUnreliable(
        const MidiPacket& packet,
        const juce::String& destAddress,
        int destPort
    );

    /**
     * Process an incoming ACK packet.
     * Called by the application when an ACK is received.
     * @param ackSequence The sequence number being acknowledged
     * @param sourceNode The node that sent the ACK
     */
    void handleAck(uint16_t ackSequence, const juce::Uuid& sourceNode);

    /**
     * Process an incoming NACK packet.
     * Called by the application when a NACK is received.
     * @param nackSequence The sequence number being negatively acknowledged
     * @param sourceNode The node that sent the NACK
     */
    void handleNack(uint16_t nackSequence, const juce::Uuid& sourceNode);

    /**
     * Cancel all pending reliable sends.
     */
    void cancelAll();

    /**
     * Get the number of packets currently waiting for ACK.
     */
    int getPendingCount() const;

    /**
     * Get statistics.
     */
    struct Statistics {
        uint64_t reliableSent = 0;
        uint64_t reliableAcked = 0;
        uint64_t reliableFailed = 0;
        uint64_t retries = 0;
        uint64_t timeouts = 0;
    };

    Statistics getStatistics() const;
    void resetStatistics();

private:
    /**
     * Pending send state.
     */
    struct PendingSend {
        MidiPacket packet;
        juce::String destAddress;
        int destPort;
        juce::uint32 sendTime;
        int retryCount;
        std::function<void()> onSuccess;
        std::function<void(const juce::String&)> onFailure;
    };

    // Check for timeouts and retry packets
    void checkTimeouts();

    // Retry a specific packet
    void retryPacket(uint16_t sequence);

    // Fail a specific packet
    void failPacket(uint16_t sequence, const juce::String& reason);

    // Succeed a specific packet
    void succeedPacket(uint16_t sequence);

    // Underlying transport
    UdpMidiTransport& transport;

    // Configuration
    Config config;

    // Pending sends (keyed by sequence number)
    mutable juce::CriticalSection pendingLock;
    std::map<uint16_t, PendingSend> pendingSends;

    // Timer for timeout checking
    std::unique_ptr<juce::Timer> timeoutTimer;

    // Statistics
    mutable juce::CriticalSection statsLock;
    Statistics stats;

    // Timer class for timeout checking
    class TimeoutChecker : public juce::Timer {
    public:
        TimeoutChecker(ReliableTransport& transport)
            : transport(transport)
        {
        }

        void timerCallback() override {
            transport.checkTimeouts();
        }

    private:
        ReliableTransport& transport;
    };

    std::unique_ptr<TimeoutChecker> timeoutChecker;
};

} // namespace NetworkMidi
