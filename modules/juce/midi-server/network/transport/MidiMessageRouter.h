#pragma once

#include "MidiClassifier.h"
#include "UdpMidiTransport.h"
#include "ReliableTransport.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <functional>

namespace NetworkMidi {

/**
 * Routes MIDI messages to appropriate transport based on message classification.
 *
 * This class is the integration point for dual-transport MIDI architecture.
 * It examines each MIDI message and routes it to either:
 * - Real-time path (UDP, low latency, best-effort)
 * - Non-real-time path (TCP via ReliableTransport, guaranteed delivery)
 *
 * Thread Safety:
 * - routeMessage() is thread-safe and can be called from MIDI input callbacks
 * - Statistics are updated atomically
 * - No locks on the routing hot path
 *
 * Performance:
 * - Designed for real-time use (audio thread safe)
 * - No memory allocation in routeMessage()
 * - No blocking operations
 * - Target latency: <1μs for routing decision + transport call
 */
class MidiMessageRouter {
public:
    /**
     * Constructor.
     *
     * @param realtimeTransport UDP transport for real-time messages
     * @param nonRealtimeTransport Reliable transport for non-real-time messages
     */
    MidiMessageRouter(
        UdpMidiTransport& realtimeTransport,
        ReliableTransport& nonRealtimeTransport
    );

    /**
     * Destructor.
     */
    ~MidiMessageRouter();

    // Non-copyable
    MidiMessageRouter(const MidiMessageRouter&) = delete;
    MidiMessageRouter& operator=(const MidiMessageRouter&) = delete;

    /**
     * Route a MIDI message to the appropriate transport.
     *
     * This is the main entry point called from MIDI input callbacks.
     * The message is classified and sent to either UDP or TCP transport.
     *
     * Thread-safe: Can be called from multiple MIDI input threads simultaneously.
     * Real-time safe: No allocation, no locks, no blocking.
     *
     * @param msg The MIDI message to route
     * @param deviceId Source device ID
     * @param destNode Destination node UUID
     * @param destAddress Destination IP address
     * @param destPort Destination port number
     * @return true if message was successfully queued for transmission
     */
    bool routeMessage(
        const juce::MidiMessage& msg,
        uint16_t deviceId,
        const juce::Uuid& destNode,
        const juce::String& destAddress,
        int destPort
    );

    /**
     * Route a message with guaranteed delivery callback.
     *
     * Only applies to non-real-time messages (SysEx). Real-time messages
     * are sent best-effort and callbacks are ignored.
     *
     * @param msg The MIDI message to route
     * @param deviceId Source device ID
     * @param destNode Destination node UUID
     * @param destAddress Destination IP address
     * @param destPort Destination port number
     * @param onDelivered Callback invoked when message is acknowledged (non-real-time only)
     * @param onFailed Callback invoked when delivery fails (non-real-time only)
     * @return true if message was successfully queued
     */
    bool routeMessageWithCallback(
        const juce::MidiMessage& msg,
        uint16_t deviceId,
        const juce::Uuid& destNode,
        const juce::String& destAddress,
        int destPort,
        std::function<void()> onDelivered,
        std::function<void(const juce::String& reason)> onFailed
    );

    /**
     * Get routing statistics.
     */
    struct Statistics {
        uint64_t realtimeMessagesSent = 0;
        uint64_t nonRealtimeMessagesSent = 0;
        uint64_t routingErrors = 0;
        uint64_t totalBytesSent = 0;

        // Breakdown by message type
        uint64_t noteMessages = 0;
        uint64_t controlChangeMessages = 0;
        uint64_t clockMessages = 0;
        uint64_t sysexMessages = 0;
        uint64_t otherMessages = 0;
    };

    /**
     * Get current statistics (thread-safe).
     */
    Statistics getStatistics() const;

    /**
     * Reset statistics counters.
     */
    void resetStatistics();

    /**
     * Enable/disable detailed message type tracking.
     *
     * When enabled, statistics include breakdown by message type.
     * Slight performance overhead (~10ns per message).
     *
     * @param enabled true to enable detailed tracking
     */
    void setDetailedTracking(bool enabled) { detailedTracking.store(enabled); }

    /**
     * Check if detailed tracking is enabled.
     */
    bool isDetailedTrackingEnabled() const { return detailedTracking.load(); }

    /**
     * Error callback for routing failures.
     *
     * Called when a message cannot be routed (e.g., transport failure).
     * Not called from real-time thread - dispatched asynchronously.
     */
    std::function<void(const juce::String& error, const juce::MidiMessage& msg)> onRoutingError;

private:
    // Helper to update detailed statistics
    void updateDetailedStats(const juce::MidiMessage& msg);

    // Helper to build MidiPacket from message
    MidiPacket buildPacket(
        const juce::MidiMessage& msg,
        uint16_t deviceId,
        const juce::Uuid& destNode
    ) const;

    // Transports
    UdpMidiTransport& realtimeTransport;
    ReliableTransport& nonRealtimeTransport;

    // Statistics (atomic for lock-free access)
    std::atomic<uint64_t> realtimeMessagesSent;
    std::atomic<uint64_t> nonRealtimeMessagesSent;
    std::atomic<uint64_t> routingErrors;
    std::atomic<uint64_t> totalBytesSent;

    // Detailed statistics (optional)
    std::atomic<bool> detailedTracking;
    std::atomic<uint64_t> noteMessages;
    std::atomic<uint64_t> controlChangeMessages;
    std::atomic<uint64_t> clockMessages;
    std::atomic<uint64_t> sysexMessages;
    std::atomic<uint64_t> otherMessages;
};

} // namespace NetworkMidi
