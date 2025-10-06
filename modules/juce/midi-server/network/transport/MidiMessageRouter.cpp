#include "MidiMessageRouter.h"
#include "../core/MidiPacket.h"

namespace NetworkMidi {

MidiMessageRouter::MidiMessageRouter(
    UdpMidiTransport& realtimeTransport,
    ReliableTransport& nonRealtimeTransport
)
    : realtimeTransport(realtimeTransport)
    , nonRealtimeTransport(nonRealtimeTransport)
    , realtimeMessagesSent(0)
    , nonRealtimeMessagesSent(0)
    , routingErrors(0)
    , totalBytesSent(0)
    , detailedTracking(false)
    , noteMessages(0)
    , controlChangeMessages(0)
    , clockMessages(0)
    , sysexMessages(0)
    , otherMessages(0)
{
}

MidiMessageRouter::~MidiMessageRouter()
{
}

bool MidiMessageRouter::routeMessage(
    const juce::MidiMessage& msg,
    uint16_t deviceId,
    const juce::Uuid& destNode,
    const juce::String& destAddress,
    int destPort
)
{
    // Classify the message
    const auto msgClass = classifyMidiMessage(msg);

    // Update detailed statistics if enabled
    if (detailedTracking.load(std::memory_order_relaxed)) {
        updateDetailedStats(msg);
    }

    // Update byte counter
    totalBytesSent.fetch_add(msg.getRawDataSize(), std::memory_order_relaxed);

    // Build packet
    MidiPacket packet = buildPacket(msg, deviceId, destNode);

    // Route based on classification
    bool success = false;

    if (msgClass == MidiMessageClass::RealTime) {
        // Real-time path: UDP (best-effort, low latency)
        success = realtimeTransport.sendPacket(packet, destAddress, destPort);

        if (success) {
            realtimeMessagesSent.fetch_add(1, std::memory_order_relaxed);
        }
    }
    else {
        // Non-real-time path: TCP (reliable, guaranteed delivery)
        nonRealtimeTransport.sendUnreliable(packet, destAddress, destPort);
        success = true;  // sendUnreliable queues the message

        nonRealtimeMessagesSent.fetch_add(1, std::memory_order_relaxed);
    }

    // Handle routing errors
    if (!success) {
        routingErrors.fetch_add(1, std::memory_order_relaxed);

        // Dispatch error callback asynchronously (don't block real-time thread)
        if (onRoutingError) {
            auto callback = onRoutingError;
            auto msgCopy = msg;
            juce::MessageManager::callAsync([callback, msgCopy]() {
                callback("Transport send failed", msgCopy);
            });
        }
    }

    return success;
}

bool MidiMessageRouter::routeMessageWithCallback(
    const juce::MidiMessage& msg,
    uint16_t deviceId,
    const juce::Uuid& destNode,
    const juce::String& destAddress,
    int destPort,
    std::function<void()> onDelivered,
    std::function<void(const juce::String& reason)> onFailed
)
{
    // Classify the message
    const auto msgClass = classifyMidiMessage(msg);

    // Update detailed statistics if enabled
    if (detailedTracking.load(std::memory_order_relaxed)) {
        updateDetailedStats(msg);
    }

    // Update byte counter
    totalBytesSent.fetch_add(msg.getRawDataSize(), std::memory_order_relaxed);

    // Build packet
    MidiPacket packet = buildPacket(msg, deviceId, destNode);

    // Route based on classification
    bool success = false;

    if (msgClass == MidiMessageClass::RealTime) {
        // Real-time path: UDP (best-effort, no callbacks)
        success = realtimeTransport.sendPacket(packet, destAddress, destPort);

        if (success) {
            realtimeMessagesSent.fetch_add(1, std::memory_order_relaxed);

            // Invoke success callback immediately for real-time messages
            // (no delivery confirmation available with UDP)
            if (onDelivered) {
                juce::MessageManager::callAsync(onDelivered);
            }
        }
        else if (onFailed) {
            // Invoke failure callback
            juce::MessageManager::callAsync([onFailed]() {
                onFailed("UDP send failed");
            });
        }
    }
    else {
        // Non-real-time path: TCP with delivery confirmation
        nonRealtimeTransport.sendReliable(
            packet,
            destAddress,
            destPort,
            onDelivered,
            onFailed
        );
        success = true;  // sendReliable queues the message

        nonRealtimeMessagesSent.fetch_add(1, std::memory_order_relaxed);
    }

    // Handle routing errors
    if (!success) {
        routingErrors.fetch_add(1, std::memory_order_relaxed);

        // Dispatch error callback asynchronously
        if (onRoutingError) {
            auto callback = onRoutingError;
            auto msgCopy = msg;
            juce::MessageManager::callAsync([callback, msgCopy]() {
                callback("Transport send failed", msgCopy);
            });
        }
    }

    return success;
}

MidiMessageRouter::Statistics MidiMessageRouter::getStatistics() const
{
    Statistics stats;
    stats.realtimeMessagesSent = realtimeMessagesSent.load(std::memory_order_relaxed);
    stats.nonRealtimeMessagesSent = nonRealtimeMessagesSent.load(std::memory_order_relaxed);
    stats.routingErrors = routingErrors.load(std::memory_order_relaxed);
    stats.totalBytesSent = totalBytesSent.load(std::memory_order_relaxed);

    // Detailed stats
    stats.noteMessages = noteMessages.load(std::memory_order_relaxed);
    stats.controlChangeMessages = controlChangeMessages.load(std::memory_order_relaxed);
    stats.clockMessages = clockMessages.load(std::memory_order_relaxed);
    stats.sysexMessages = sysexMessages.load(std::memory_order_relaxed);
    stats.otherMessages = otherMessages.load(std::memory_order_relaxed);

    return stats;
}

void MidiMessageRouter::resetStatistics()
{
    realtimeMessagesSent.store(0, std::memory_order_relaxed);
    nonRealtimeMessagesSent.store(0, std::memory_order_relaxed);
    routingErrors.store(0, std::memory_order_relaxed);
    totalBytesSent.store(0, std::memory_order_relaxed);

    noteMessages.store(0, std::memory_order_relaxed);
    controlChangeMessages.store(0, std::memory_order_relaxed);
    clockMessages.store(0, std::memory_order_relaxed);
    sysexMessages.store(0, std::memory_order_relaxed);
    otherMessages.store(0, std::memory_order_relaxed);
}

void MidiMessageRouter::updateDetailedStats(const juce::MidiMessage& msg)
{
    if (msg.isNoteOnOrOff()) {
        noteMessages.fetch_add(1, std::memory_order_relaxed);
    }
    else if (msg.isController()) {
        controlChangeMessages.fetch_add(1, std::memory_order_relaxed);
    }
    else if (msg.getRawDataSize() > 0 && msg.getRawData()[0] == 0xF8) {
        clockMessages.fetch_add(1, std::memory_order_relaxed);
    }
    else if (msg.isSysEx()) {
        sysexMessages.fetch_add(1, std::memory_order_relaxed);
    }
    else {
        otherMessages.fetch_add(1, std::memory_order_relaxed);
    }
}

MidiPacket MidiMessageRouter::buildPacket(
    const juce::MidiMessage& msg,
    uint16_t deviceId,
    const juce::Uuid& destNode
) const
{
    // Copy MIDI data
    const int dataSize = msg.getRawDataSize();
    std::vector<uint8_t> midiData(dataSize);
    std::memcpy(midiData.data(), msg.getRawData(), dataSize);

    // Create packet using factory method
    MidiPacket packet = MidiPacket::createDataPacket(
        realtimeTransport.getNodeId(),
        destNode,
        deviceId,
        midiData,
        0  // Sequence will be set by transport
    );

    // Set SysEx flag if applicable
    if (msg.isSysEx()) {
        packet.addFlag(MidiPacket::SysEx);
    }

    return packet;
}

} // namespace NetworkMidi
