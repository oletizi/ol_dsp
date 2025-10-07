/**
 * ForwardingRule.h
 *
 * Defines MIDI routing rules for cross-node message forwarding.
 * Each rule specifies a source device, destination device, and optional filters.
 */

#pragma once

#include <juce_core/juce_core.h>
#include "DeviceRegistry.h"
#include <cstdint>
#include <optional>

namespace NetworkMidi {

/**
 * MIDI message type filter flags
 * Used to selectively filter which MIDI messages are forwarded
 */
enum class MidiMessageType : uint8_t {
    None = 0,
    NoteOff = 1 << 0,          // 0x80-0x8F
    NoteOn = 1 << 1,           // 0x90-0x9F
    PolyAftertouch = 1 << 2,   // 0xA0-0xAF
    ControlChange = 1 << 3,    // 0xB0-0xBF
    ProgramChange = 1 << 4,    // 0xC0-0xCF
    ChannelAftertouch = 1 << 5,// 0xD0-0xDF
    PitchBend = 1 << 6,        // 0xE0-0xEF
    SystemMessage = 1 << 7,    // 0xF0-0xFF (SysEx, Clock, etc)
    All = 0xFF
};

// Bitwise operators for MidiMessageType
inline MidiMessageType operator|(MidiMessageType a, MidiMessageType b) {
    return static_cast<MidiMessageType>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline MidiMessageType operator&(MidiMessageType a, MidiMessageType b) {
    return static_cast<MidiMessageType>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool hasMessageType(MidiMessageType flags, MidiMessageType type) {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(type)) != 0;
}

/**
 * MIDI channel filter (1-16, or 0 for "all channels")
 * Supports filtering on a specific MIDI channel
 */
struct ChannelFilter {
    uint8_t channel;  // 0 = all channels, 1-16 = specific channel

    ChannelFilter() : channel(0) {}
    explicit ChannelFilter(uint8_t ch) : channel(ch) {}

    bool matchesAll() const { return channel == 0; }
    bool isValid() const { return channel <= 16; }

    bool matches(uint8_t midiChannel) const {
        if (matchesAll()) return true;
        return channel == midiChannel;
    }

    juce::var toVar() const {
        return juce::var(static_cast<int>(channel));
    }

    static ChannelFilter fromVar(const juce::var& v) {
        return ChannelFilter(static_cast<uint8_t>(static_cast<int>(v)));
    }
};

/**
 * Forwarding rule statistics
 * Tracks usage and performance metrics for a routing rule
 */
struct ForwardingStatistics {
    uint64_t messagesForwarded = 0;
    uint64_t messagesDropped = 0;
    juce::Time lastForwardedTime;

    ForwardingStatistics() = default;

    void incrementForwarded() {
        messagesForwarded++;
        lastForwardedTime = juce::Time::getCurrentTime();
    }

    void incrementDropped() {
        messagesDropped++;
    }

    void reset() {
        messagesForwarded = 0;
        messagesDropped = 0;
        lastForwardedTime = juce::Time();
    }

    juce::var toVar() const {
        auto obj = new juce::DynamicObject();
        obj->setProperty("messagesForwarded", static_cast<juce::int64>(messagesForwarded));
        obj->setProperty("messagesDropped", static_cast<juce::int64>(messagesDropped));
        obj->setProperty("lastForwardedTime", lastForwardedTime.toMilliseconds());
        return juce::var(obj);
    }

    static ForwardingStatistics fromVar(const juce::var& v) {
        ForwardingStatistics stats;
        if (auto* obj = v.getDynamicObject()) {
            stats.messagesForwarded = static_cast<uint64_t>(static_cast<juce::int64>(obj->getProperty("messagesForwarded")));
            stats.messagesDropped = static_cast<uint64_t>(static_cast<juce::int64>(obj->getProperty("messagesDropped")));
            stats.lastForwardedTime = juce::Time(static_cast<juce::int64>(obj->getProperty("lastForwardedTime")));
        }
        return stats;
    }
};

/**
 * Forwarding rule for MIDI message routing
 *
 * Design (Phase 2: Routing Configuration):
 * - Maps source device (nodeId, deviceId) to destination device (nodeId, deviceId)
 * - Supports optional filters (channel, message type)
 * - Tracks statistics (messages forwarded, dropped, last forwarded time)
 * - Rule priority for conflict resolution (higher = higher priority)
 * - Enable/disable flag for temporary rule deactivation
 * - UUID-based rule ID for stable references
 */
struct ForwardingRule {
    // Rule identity
    juce::String ruleId;                 // UUID-based unique identifier
    bool enabled = true;                 // Rule active/inactive
    int priority = 100;                  // Higher = higher priority (default: 100)

    // Source device (where MIDI comes from)
    DeviceKey sourceDevice;

    // Destination device (where MIDI goes to)
    DeviceKey destinationDevice;

    // Optional filters
    std::optional<ChannelFilter> channelFilter;           // Filter by MIDI channel
    MidiMessageType messageTypeFilter = MidiMessageType::All;  // Filter by message type

    // Statistics
    ForwardingStatistics statistics;

    // Constructors
    ForwardingRule() = default;

    ForwardingRule(const juce::Uuid& srcNode, uint16_t srcDeviceId,
                   const juce::Uuid& dstNode, uint16_t dstDeviceId)
        : sourceDevice(srcNode, srcDeviceId)
        , destinationDevice(dstNode, dstDeviceId)
    {
        ruleId = juce::Uuid().toString();
    }

    // Convenience accessors
    const juce::Uuid& sourceNodeId() const { return sourceDevice.ownerNode; }
    uint16_t sourceDeviceId() const { return sourceDevice.deviceId; }
    const juce::Uuid& destinationNodeId() const { return destinationDevice.ownerNode; }
    uint16_t destinationDeviceId() const { return destinationDevice.deviceId; }

    // Validation
    bool isValid() const {
        // Rule ID must not be empty
        if (ruleId.isEmpty()) return false;

        // Source and destination must be different
        if (sourceDevice == destinationDevice) return false;

        // Channel filter must be valid if present
        if (channelFilter.has_value() && !channelFilter->isValid()) return false;

        return true;
    }

    // Filter matching
    bool matchesChannel(uint8_t midiChannel) const {
        if (!channelFilter.has_value()) return true;
        return channelFilter->matches(midiChannel);
    }

    bool matchesMessageType(MidiMessageType msgType) const {
        return hasMessageType(messageTypeFilter, msgType);
    }

    bool shouldForward(uint8_t midiChannel, MidiMessageType msgType) const {
        if (!enabled) return false;
        if (!matchesChannel(midiChannel)) return false;
        if (!matchesMessageType(msgType)) return false;
        return true;
    }

    // Comparison operators (for sorting by priority)
    bool operator<(const ForwardingRule& other) const {
        return priority < other.priority;  // Lower priority value = lower priority
    }

    bool operator==(const ForwardingRule& other) const {
        return ruleId == other.ruleId;
    }

    // JSON serialization
    juce::var toVar() const {
        auto obj = new juce::DynamicObject();

        obj->setProperty("ruleId", ruleId);
        obj->setProperty("enabled", enabled);
        obj->setProperty("priority", priority);

        // Source device
        auto srcObj = new juce::DynamicObject();
        srcObj->setProperty("nodeId", sourceDevice.ownerNode.toString());
        srcObj->setProperty("deviceId", static_cast<int>(sourceDevice.deviceId));
        obj->setProperty("source", juce::var(srcObj));

        // Destination device
        auto dstObj = new juce::DynamicObject();
        dstObj->setProperty("nodeId", destinationDevice.ownerNode.toString());
        dstObj->setProperty("deviceId", static_cast<int>(destinationDevice.deviceId));
        obj->setProperty("destination", juce::var(dstObj));

        // Filters
        if (channelFilter.has_value()) {
            obj->setProperty("channelFilter", channelFilter->toVar());
        }
        obj->setProperty("messageTypeFilter", static_cast<int>(messageTypeFilter));

        // Statistics
        obj->setProperty("statistics", statistics.toVar());

        return juce::var(obj);
    }

    static ForwardingRule fromVar(const juce::var& v) {
        ForwardingRule rule;

        if (auto* obj = v.getDynamicObject()) {
            rule.ruleId = obj->getProperty("ruleId").toString();
            rule.enabled = static_cast<bool>(obj->getProperty("enabled"));
            rule.priority = static_cast<int>(obj->getProperty("priority"));

            // Source device
            if (auto* srcObj = obj->getProperty("source").getDynamicObject()) {
                juce::Uuid srcNodeId(srcObj->getProperty("nodeId").toString());
                uint16_t srcDevId = static_cast<uint16_t>(static_cast<int>(srcObj->getProperty("deviceId")));
                rule.sourceDevice = DeviceKey(srcNodeId, srcDevId);
            }

            // Destination device
            if (auto* dstObj = obj->getProperty("destination").getDynamicObject()) {
                juce::Uuid dstNodeId(dstObj->getProperty("nodeId").toString());
                uint16_t dstDevId = static_cast<uint16_t>(static_cast<int>(dstObj->getProperty("deviceId")));
                rule.destinationDevice = DeviceKey(dstNodeId, dstDevId);
            }

            // Filters
            if (obj->hasProperty("channelFilter")) {
                rule.channelFilter = ChannelFilter::fromVar(obj->getProperty("channelFilter"));
            }
            rule.messageTypeFilter = static_cast<MidiMessageType>(static_cast<int>(obj->getProperty("messageTypeFilter")));

            // Statistics
            if (obj->hasProperty("statistics")) {
                rule.statistics = ForwardingStatistics::fromVar(obj->getProperty("statistics"));
            }
        }

        return rule;
    }

    juce::String toJsonString() const {
        return juce::JSON::toString(toVar(), true);
    }

    static ForwardingRule fromJsonString(const juce::String& json) {
        auto parsed = juce::JSON::parse(json);
        return fromVar(parsed);
    }
};

} // namespace NetworkMidi
