#pragma once

#include "../core/MidiPacket.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <map>
#include <vector>
#include <functional>
#include <deque>

namespace NetworkMidi {

/**
 * Message buffering and reordering for out-of-order packet delivery.
 *
 * Features:
 * - Reorders packets based on sequence numbers
 * - Detects and handles sequence number wraparound
 * - Configurable buffer size
 * - Duplicate detection
 * - Gap detection and recovery
 */
class MessageBuffer {
public:
    /**
     * Configuration for message buffering.
     */
    struct Config {
        size_t maxBufferSize;
        int maxSequenceGap;
        bool allowDuplicates;
        int deliveryTimeoutMs;

        Config()
            : maxBufferSize(100)
            , maxSequenceGap(50)
            , allowDuplicates(false)
            , deliveryTimeoutMs(1000)
        {
        }
    };

    /**
     * Constructor.
     * @param config Buffer configuration
     */
    explicit MessageBuffer(const Config& config = Config());

    /**
     * Destructor.
     */
    ~MessageBuffer();

    // Non-copyable
    MessageBuffer(const MessageBuffer&) = delete;
    MessageBuffer& operator=(const MessageBuffer&) = delete;

    /**
     * Add a received packet to the buffer.
     * Packets are reordered and delivered in sequence order via callback.
     * @param packet The received packet
     */
    void addPacket(const MidiPacket& packet);

    /**
     * Reset the buffer and sequence tracking.
     * Clears all buffered packets and resets the expected sequence number.
     */
    void reset();

    /**
     * Get the next expected sequence number.
     */
    uint16_t getNextExpectedSequence() const { return nextExpectedSequence; }

    /**
     * Set the next expected sequence number (for manual sync).
     */
    void setNextExpectedSequence(uint16_t sequence) { nextExpectedSequence = sequence; }

    /**
     * Callback for in-order packet delivery.
     * Called when a packet can be delivered (either received in order or reordered).
     */
    std::function<void(const MidiPacket&)> onPacketReady;

    /**
     * Callback for gap detection.
     * Called when a sequence gap is detected (potential packet loss).
     */
    std::function<void(uint16_t missingSequence)> onGapDetected;

    /**
     * Callback for duplicate detection.
     * Called when a duplicate packet is received.
     */
    std::function<void(uint16_t duplicateSequence)> onDuplicateDetected;

    /**
     * Get statistics.
     */
    struct Statistics {
        uint64_t packetsReceived = 0;
        uint64_t packetsDelivered = 0;
        uint64_t packetsReordered = 0;
        uint64_t packetsDropped = 0;
        uint64_t duplicates = 0;
        uint64_t gapsDetected = 0;
        size_t currentBufferSize = 0;
        size_t maxBufferSizeReached = 0;
    };

    Statistics getStatistics() const;
    void resetStatistics();

private:
    // Deliver all sequential packets starting from nextExpectedSequence
    void deliverSequentialPackets();

    // Check if sequence 'a' comes before sequence 'b' (handles wraparound)
    bool sequenceBefore(uint16_t a, uint16_t b) const;

    // Calculate sequence difference (handles wraparound)
    int sequenceDifference(uint16_t a, uint16_t b) const;

    // Check for and handle timeouts
    void checkTimeouts();

    // Configuration
    Config config;

    // Next expected sequence number
    uint16_t nextExpectedSequence;

    // Buffer of out-of-order packets (keyed by sequence number)
    mutable juce::CriticalSection bufferLock;
    std::map<uint16_t, MidiPacket> buffer;

    // Track received sequence numbers to detect duplicates
    std::deque<uint16_t> receivedSequences;
    static constexpr size_t MAX_RECEIVED_HISTORY = 100;

    // Track packet arrival times for timeout detection
    struct BufferedPacketInfo {
        juce::uint32 arrivalTime;
        MidiPacket packet;
    };
    std::map<uint16_t, BufferedPacketInfo> bufferedPacketTimes;

    // Statistics
    mutable juce::CriticalSection statsLock;
    Statistics stats;

    // Timer for timeout checking
    class TimeoutChecker : public juce::Timer {
    public:
        explicit TimeoutChecker(MessageBuffer& parentBuffer)
            : ownerBuffer(parentBuffer)
        {
        }

        void timerCallback() override {
            ownerBuffer.checkTimeouts();
        }

    private:
        MessageBuffer& ownerBuffer;
    };

    std::unique_ptr<TimeoutChecker> timeoutChecker;
};

} // namespace NetworkMidi
