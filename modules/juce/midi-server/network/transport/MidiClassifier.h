#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace NetworkMidi {

/**
 * Classification of MIDI messages by QoS requirements.
 *
 * Real-Time messages require ultra-low latency (<1ms) and can tolerate
 * occasional packet loss. Non-Real-Time messages require guaranteed
 * delivery but can tolerate higher latency (10-100ms).
 */
enum class MidiMessageClass {
    RealTime,      // Needs UDP transport (low latency, best-effort)
    NonRealTime    // Needs TCP transport (reliable, higher latency OK)
};

/**
 * Classify a MIDI message by its QoS requirements.
 *
 * Classification Rules:
 * - System Real-Time (0xF8-0xFF) -> RealTime
 *   Examples: MIDI Clock (0xF8), Start (0xFA), Stop (0xFC), Active Sensing (0xFE)
 *
 * - System Exclusive (0xF0...0xF7) -> NonRealTime
 *   Examples: SysEx messages, patch dumps, sample dumps
 *
 * - Channel Voice (0x80-0xEF) -> RealTime
 *   Examples: Note On/Off, Control Change, Pitch Bend, Aftertouch, Program Change
 *
 * - Default -> NonRealTime (for safety)
 *
 * Performance Considerations:
 * - This function is called on the MIDI input thread for every message
 * - Must be extremely fast (target: <100ns)
 * - No memory allocation
 * - No blocking operations
 * - Thread-safe (pure function)
 *
 * @param msg The MIDI message to classify
 * @return MidiMessageClass indicating appropriate transport
 */
inline MidiMessageClass classifyMidiMessage(const juce::MidiMessage& msg)
{
    // Safety check: ensure message has data
    if (msg.getRawDataSize() == 0) {
        return MidiMessageClass::NonRealTime;
    }

    const uint8_t statusByte = msg.getRawData()[0];

    // System Real-Time messages (0xF8 - 0xFF)
    // These are single-byte messages that can appear at any time
    if (statusByte >= 0xF8) {
        return MidiMessageClass::RealTime;
    }

    // System Exclusive (0xF0 ... 0xF7)
    // These can be very large (KB+) and require reliable delivery
    if (msg.isSysEx()) {
        return MidiMessageClass::NonRealTime;
    }

    // Channel Voice messages (0x80 - 0xEF)
    // Note On/Off, CC, Pitch Bend, Aftertouch, Program Change
    // These are time-critical performance messages
    if (statusByte >= 0x80 && statusByte < 0xF0) {
        return MidiMessageClass::RealTime;
    }

    // System Common messages (0xF1 - 0xF7, except 0xF0 SysEx)
    // Examples: MTC Quarter Frame (0xF1), Song Position (0xF2), etc.
    // Default to non-real-time for safety - these are less common
    return MidiMessageClass::NonRealTime;
}

/**
 * Get a human-readable description of the message class.
 *
 * Useful for debugging and logging.
 *
 * @param msgClass The message class
 * @return String description
 */
inline juce::String getMidiMessageClassName(MidiMessageClass msgClass)
{
    switch (msgClass) {
        case MidiMessageClass::RealTime:
            return "RealTime";
        case MidiMessageClass::NonRealTime:
            return "NonRealTime";
        default:
            return "Unknown";
    }
}

/**
 * Get detailed description of why a message was classified as it was.
 *
 * Useful for debugging classification logic.
 *
 * @param msg The MIDI message
 * @return String describing the classification reasoning
 */
inline juce::String explainClassification(const juce::MidiMessage& msg)
{
    if (msg.getRawDataSize() == 0) {
        return "Empty message -> NonRealTime (safety)";
    }

    const uint8_t statusByte = msg.getRawData()[0];

    if (statusByte >= 0xF8) {
        return juce::String("System Real-Time (0x") +
               juce::String::toHexString(statusByte) +
               ") -> RealTime";
    }

    if (msg.isSysEx()) {
        return juce::String("System Exclusive (size: ") +
               juce::String(msg.getRawDataSize()) +
               " bytes) -> NonRealTime";
    }

    if (statusByte >= 0x80 && statusByte < 0xF0) {
        return juce::String("Channel Voice (0x") +
               juce::String::toHexString(statusByte) +
               ") -> RealTime";
    }

    return juce::String("System Common (0x") +
           juce::String::toHexString(statusByte) +
           ") -> NonRealTime (safety)";
}

} // namespace NetworkMidi
