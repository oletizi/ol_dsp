#include "ReliableTransport.h"

namespace NetworkMidi {

ReliableTransport::ReliableTransport(
    UdpMidiTransport& transport,
    const Config& config)
    : transport(transport)
    , config(config)
{
    // Create and start timeout checker
    timeoutChecker = std::make_unique<TimeoutChecker>(*this);
    timeoutChecker->startTimer(10);  // Check every 10ms
}

ReliableTransport::~ReliableTransport() {
    cancelAll();
    if (timeoutChecker != nullptr) {
        timeoutChecker->stopTimer();
    }
}

void ReliableTransport::sendReliable(
    const MidiPacket& packet,
    const juce::String& destAddress,
    int destPort,
    std::function<void()> onSuccess,
    std::function<void(const juce::String&)> onFailure)
{
    // Send the packet
    if (!transport.sendPacket(packet, destAddress, destPort)) {
        if (onFailure) {
            onFailure("Failed to send packet");
        }
        juce::ScopedLock lock(statsLock);
        stats.reliableFailed++;
        return;
    }

    // Store pending send
    {
        juce::ScopedLock lock(pendingLock);

        PendingSend pending;
        pending.packet = packet;
        pending.destAddress = destAddress;
        pending.destPort = destPort;
        pending.sendTime = juce::Time::getMillisecondCounter();
        pending.retryCount = 0;
        pending.onSuccess = onSuccess;
        pending.onFailure = onFailure;

        pendingSends[packet.getSequence()] = std::move(pending);
    }

    // Update statistics
    {
        juce::ScopedLock lock(statsLock);
        stats.reliableSent++;
    }
}

void ReliableTransport::sendUnreliable(
    const MidiPacket& packet,
    const juce::String& destAddress,
    int destPort)
{
    transport.sendPacket(packet, destAddress, destPort);
}

void ReliableTransport::handleAck(uint16_t ackSequence, const juce::Uuid& sourceNode) {
    succeedPacket(ackSequence);
}

void ReliableTransport::handleNack(uint16_t nackSequence, const juce::Uuid& sourceNode) {
    // NACK received - retry immediately
    retryPacket(nackSequence);
}

void ReliableTransport::cancelAll() {
    juce::ScopedLock lock(pendingLock);

    // Invoke failure callbacks for all pending sends
    for (auto& pair : pendingSends) {
        if (pair.second.onFailure) {
            pair.second.onFailure("Cancelled");
        }
    }

    pendingSends.clear();
}

int ReliableTransport::getPendingCount() const {
    juce::ScopedLock lock(pendingLock);
    return static_cast<int>(pendingSends.size());
}

void ReliableTransport::checkTimeouts() {
    juce::uint32 now = juce::Time::getMillisecondCounter();

    std::vector<uint16_t> timedOut;

    // Find timed-out packets
    {
        juce::ScopedLock lock(pendingLock);

        for (const auto& pair : pendingSends) {
            uint16_t sequence = pair.first;
            const PendingSend& pending = pair.second;

            // Calculate timeout with exponential backoff
            int timeout = config.timeoutMs + (pending.retryCount * config.retryBackoffMs);
            juce::uint32 elapsed = now - pending.sendTime;

            if (elapsed >= static_cast<juce::uint32>(timeout)) {
                timedOut.push_back(sequence);
            }
        }
    }

    // Process timeouts (outside lock to avoid deadlock)
    for (uint16_t sequence : timedOut) {
        retryPacket(sequence);
    }
}

void ReliableTransport::retryPacket(uint16_t sequence) {
    PendingSend pending;
    bool shouldRetry = false;
    bool shouldFail = false;

    // Get pending send and update retry count
    {
        juce::ScopedLock lock(pendingLock);

        auto it = pendingSends.find(sequence);
        if (it == pendingSends.end()) {
            return;  // Already handled
        }

        pending = it->second;
        pending.retryCount++;

        if (pending.retryCount <= config.maxRetries) {
            shouldRetry = true;
            // Update the pending send with new retry count and time
            it->second.retryCount = pending.retryCount;
            it->second.sendTime = juce::Time::getMillisecondCounter();
        } else {
            shouldFail = true;
            // Remove from pending sends
            pendingSends.erase(it);
        }
    }

    if (shouldRetry) {
        // Retry the send
        transport.sendPacket(pending.packet, pending.destAddress, pending.destPort);

        // Update statistics
        {
            juce::ScopedLock lock(statsLock);
            stats.retries++;
        }
    } else if (shouldFail) {
        // Max retries exceeded
        if (pending.onFailure) {
            pending.onFailure("Max retries exceeded");
        }

        // Update statistics
        {
            juce::ScopedLock lock(statsLock);
            stats.reliableFailed++;
            stats.timeouts++;
        }
    }
}

void ReliableTransport::failPacket(uint16_t sequence, const juce::String& reason) {
    PendingSend pending;
    bool found = false;

    // Remove from pending sends
    {
        juce::ScopedLock lock(pendingLock);

        auto it = pendingSends.find(sequence);
        if (it != pendingSends.end()) {
            pending = it->second;
            found = true;
            pendingSends.erase(it);
        }
    }

    if (found) {
        // Invoke failure callback
        if (pending.onFailure) {
            pending.onFailure(reason);
        }

        // Update statistics
        {
            juce::ScopedLock lock(statsLock);
            stats.reliableFailed++;
        }
    }
}

void ReliableTransport::succeedPacket(uint16_t sequence) {
    PendingSend pending;
    bool found = false;

    // Remove from pending sends
    {
        juce::ScopedLock lock(pendingLock);

        auto it = pendingSends.find(sequence);
        if (it != pendingSends.end()) {
            pending = it->second;
            found = true;
            pendingSends.erase(it);
        }
    }

    if (found) {
        // Invoke success callback
        if (pending.onSuccess) {
            pending.onSuccess();
        }

        // Update statistics
        {
            juce::ScopedLock lock(statsLock);
            stats.reliableAcked++;
        }
    }
}

ReliableTransport::Statistics ReliableTransport::getStatistics() const {
    juce::ScopedLock lock(statsLock);
    return stats;
}

void ReliableTransport::resetStatistics() {
    juce::ScopedLock lock(statsLock);
    stats = Statistics();
}

} // namespace NetworkMidi
