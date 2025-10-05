#include "MessageBuffer.h"
#include <algorithm>

namespace NetworkMidi {

MessageBuffer::MessageBuffer(const Config& cfg)
    : config(cfg)
    , nextExpectedSequence(0)
{
    // Create and start timeout checker
    timeoutChecker = std::make_unique<TimeoutChecker>(*this);
    timeoutChecker->startTimer(100);  // Check every 100ms
}

MessageBuffer::~MessageBuffer() {
    if (timeoutChecker != nullptr) {
        timeoutChecker->stopTimer();
    }
}

void MessageBuffer::addPacket(const MidiPacket& packet) {
    juce::ScopedLock lock(bufferLock);

    uint16_t sequence = packet.getSequence();

    // Update statistics
    {
        juce::ScopedLock statsLock_(statsLock);
        stats.packetsReceived++;
    }

    // Check for duplicates
    auto receivedIt = std::find(receivedSequences.begin(), receivedSequences.end(), sequence);
    if (receivedIt != receivedSequences.end()) {
        // Duplicate detected
        {
            juce::ScopedLock statsLock_(statsLock);
            stats.duplicates++;
        }

        if (onDuplicateDetected) {
            onDuplicateDetected(sequence);
        }

        if (!config.allowDuplicates) {
            return;  // Discard duplicate
        }
    }

    // Add to received history (for duplicate detection)
    receivedSequences.push_back(sequence);
    if (receivedSequences.size() > MAX_RECEIVED_HISTORY) {
        receivedSequences.pop_front();
    }

    // Check if this is the next expected packet
    if (sequence == nextExpectedSequence) {
        // Deliver immediately
        if (onPacketReady) {
            onPacketReady(packet);
        }

        {
            juce::ScopedLock statsLock_(statsLock);
            stats.packetsDelivered++;
        }

        nextExpectedSequence++;

        // Deliver any buffered sequential packets
        deliverSequentialPackets();

    } else if (sequenceBefore(sequence, nextExpectedSequence)) {
        // Old packet (already processed or lost)
        {
            juce::ScopedLock statsLock_(statsLock);
            stats.packetsDropped++;
        }

    } else {
        // Out-of-order packet - buffer it
        int gap = sequenceDifference(nextExpectedSequence, sequence);

        if (gap > config.maxSequenceGap) {
            // Gap too large - assume packets lost, skip forward
            if (onGapDetected) {
                for (uint16_t missing = nextExpectedSequence; missing != sequence; missing++) {
                    onGapDetected(missing);
                }
            }

            {
                juce::ScopedLock statsLock_(statsLock);
                stats.gapsDetected += static_cast<uint64_t>(gap);
            }

            // Skip to this packet
            nextExpectedSequence = sequence;

            // Deliver this packet
            if (onPacketReady) {
                onPacketReady(packet);
            }

            {
                juce::ScopedLock statsLock_(statsLock);
                stats.packetsDelivered++;
            }

            nextExpectedSequence++;

            // Clear buffer (all buffered packets are now obsolete)
            buffer.clear();
            bufferedPacketTimes.clear();

        } else {
            // Buffer the packet
            if (buffer.size() >= config.maxBufferSize) {
                // Buffer full - drop oldest packet
                auto oldestIt = buffer.begin();
                buffer.erase(oldestIt);
                bufferedPacketTimes.erase(oldestIt->first);

                {
                    juce::ScopedLock statsLock_(statsLock);
                    stats.packetsDropped++;
                }
            }

            buffer[sequence] = packet;

            // Track arrival time
            BufferedPacketInfo info;
            info.arrivalTime = juce::Time::getMillisecondCounter();
            info.packet = packet;
            bufferedPacketTimes[sequence] = info;

            {
                juce::ScopedLock statsLock_(statsLock);
                stats.packetsReordered++;
                stats.currentBufferSize = buffer.size();
                if (buffer.size() > stats.maxBufferSizeReached) {
                    stats.maxBufferSizeReached = buffer.size();
                }
            }

            // Detect gap
            if (gap > 1) {
                if (onGapDetected) {
                    for (uint16_t missing = nextExpectedSequence;
                         sequenceBefore(missing, sequence);
                         missing++) {
                        onGapDetected(missing);
                    }
                }

                juce::ScopedLock statsLock_(statsLock);
                stats.gapsDetected++;
            }
        }
    }
}

void MessageBuffer::reset() {
    juce::ScopedLock lock(bufferLock);

    buffer.clear();
    bufferedPacketTimes.clear();
    receivedSequences.clear();
    nextExpectedSequence = 0;

    juce::ScopedLock statsLock_(statsLock);
    stats.currentBufferSize = 0;
}

void MessageBuffer::deliverSequentialPackets() {
    // Deliver all packets in sequence starting from nextExpectedSequence
    while (buffer.count(nextExpectedSequence) > 0) {
        const MidiPacket& packet = buffer[nextExpectedSequence];

        // Deliver packet
        if (onPacketReady) {
            onPacketReady(packet);
        }

        {
            juce::ScopedLock statsLock_(statsLock);
            stats.packetsDelivered++;
        }

        // Remove from buffer
        buffer.erase(nextExpectedSequence);
        bufferedPacketTimes.erase(nextExpectedSequence);

        nextExpectedSequence++;
    }

    // Update current buffer size
    {
        juce::ScopedLock statsLock_(statsLock);
        stats.currentBufferSize = buffer.size();
    }
}

bool MessageBuffer::sequenceBefore(uint16_t a, uint16_t b) const {
    // Handle sequence number wraparound
    // Assumes sequence numbers don't differ by more than 32768
    int diff = static_cast<int>(b) - static_cast<int>(a);
    if (diff > 32768) {
        diff -= 65536;
    } else if (diff < -32768) {
        diff += 65536;
    }
    return diff > 0;
}

int MessageBuffer::sequenceDifference(uint16_t a, uint16_t b) const {
    // Calculate b - a, handling wraparound
    int diff = static_cast<int>(b) - static_cast<int>(a);
    if (diff > 32768) {
        diff -= 65536;
    } else if (diff < -32768) {
        diff += 65536;
    }
    return diff;
}

void MessageBuffer::checkTimeouts() {
    juce::ScopedLock lock(bufferLock);

    juce::uint32 now = juce::Time::getMillisecondCounter();
    std::vector<uint16_t> timedOut;

    // Find timed-out packets
    for (const auto& pair : bufferedPacketTimes) {
        uint16_t sequence = pair.first;
        const BufferedPacketInfo& info = pair.second;

        juce::uint32 elapsed = now - info.arrivalTime;
        if (elapsed >= static_cast<juce::uint32>(config.deliveryTimeoutMs)) {
            timedOut.push_back(sequence);
        }
    }

    // Process timeouts
    for (uint16_t sequence : timedOut) {
        // If we're still waiting for earlier packets, skip this one
        if (sequenceBefore(nextExpectedSequence, sequence)) {
            // Assume missing packets are lost - skip forward
            if (onGapDetected) {
                for (uint16_t missing = nextExpectedSequence;
                     sequenceBefore(missing, sequence);
                     missing++) {
                    onGapDetected(missing);
                }
            }

            nextExpectedSequence = sequence;

            // Deliver this packet and any sequential ones
            deliverSequentialPackets();
        }
    }
}

MessageBuffer::Statistics MessageBuffer::getStatistics() const {
    juce::ScopedLock lock(statsLock);
    return stats;
}

void MessageBuffer::resetStatistics() {
    juce::ScopedLock lock(statsLock);
    Statistics oldStats = stats;
    stats = Statistics();
    stats.currentBufferSize = oldStats.currentBufferSize;  // Preserve current buffer size
}

} // namespace NetworkMidi
