/**
 * RouteManager.h
 *
 * Thread-safe manager for MIDI forwarding rules.
 * Provides CRUD operations, validation, persistence, and statistics.
 */

#pragma once

#include <juce_core/juce_core.h>
#include "ForwardingRule.h"
#include "DeviceRegistry.h"
#include <map>
#include <vector>
#include <mutex>
#include <optional>
#include <string>

namespace NetworkMidi {

/**
 * Thread-safe manager for MIDI forwarding rules
 *
 * Design (Phase 2: Routing Configuration):
 * - Manages collection of ForwardingRule objects
 * - Thread-safe CRUD operations (add, remove, update, query)
 * - Rule validation against DeviceRegistry
 * - File persistence (load/save JSON)
 * - Statistics aggregation
 * - Fast lookup by source device for routing decisions
 */
class RouteManager {
public:
    /**
     * Aggregate statistics for all routing rules
     */
    struct Statistics {
        uint64_t totalRules = 0;
        uint64_t enabledRules = 0;
        uint64_t disabledRules = 0;
        uint64_t totalMessagesForwarded = 0;
        uint64_t totalMessagesDropped = 0;

        juce::var toVar() const {
            auto obj = new juce::DynamicObject();
            obj->setProperty("totalRules", static_cast<juce::int64>(totalRules));
            obj->setProperty("enabledRules", static_cast<juce::int64>(enabledRules));
            obj->setProperty("disabledRules", static_cast<juce::int64>(disabledRules));
            obj->setProperty("totalMessagesForwarded", static_cast<juce::int64>(totalMessagesForwarded));
            obj->setProperty("totalMessagesDropped", static_cast<juce::int64>(totalMessagesDropped));
            return juce::var(obj);
        }
    };

    // Constructor
    explicit RouteManager(DeviceRegistry& registry);
    ~RouteManager();

    //==============================================================================
    // Rule management (CRUD operations)

    /**
     * Add a new forwarding rule
     * @param rule The rule to add (ruleId will be generated if empty)
     * @return Rule ID of the added rule
     * @throws std::runtime_error if rule is invalid
     */
    std::string addRule(const ForwardingRule& rule);

    /**
     * Remove a forwarding rule by ID
     * @param ruleId The ID of the rule to remove
     * @return true if rule was removed, false if not found
     */
    bool removeRule(const std::string& ruleId);

    /**
     * Update an existing forwarding rule
     * @param ruleId The ID of the rule to update
     * @param rule The new rule data
     * @return true if rule was updated, false if not found
     * @throws std::runtime_error if rule is invalid
     */
    bool updateRule(const std::string& ruleId, const ForwardingRule& rule);

    /**
     * Get a specific forwarding rule by ID
     * @param ruleId The ID of the rule to retrieve
     * @return The rule if found, std::nullopt otherwise
     */
    std::optional<ForwardingRule> getRule(const std::string& ruleId) const;

    /**
     * Get all forwarding rules
     * @return Vector of all rules
     */
    std::vector<ForwardingRule> getAllRules() const;

    /**
     * Clear all forwarding rules
     */
    void clearAllRules();

    //==============================================================================
    // Query operations

    /**
     * Get all destination rules for a source device
     * Used for fast routing lookups during message forwarding
     * @param sourceNodeId Source device's node ID
     * @param sourceDeviceId Source device's device ID
     * @return Vector of rules sorted by priority (highest first)
     */
    std::vector<ForwardingRule> getDestinations(const juce::Uuid& sourceNodeId,
                                                 uint16_t sourceDeviceId) const;

    /**
     * Get all rules where a device is the source
     * @param nodeId Node ID of the source device
     * @param deviceId Device ID of the source device
     * @return Vector of rules using this device as source
     */
    std::vector<ForwardingRule> getSourceRules(const juce::Uuid& nodeId,
                                                uint16_t deviceId) const;

    /**
     * Get all rules where a device is the destination
     * @param nodeId Node ID of the destination device
     * @param deviceId Device ID of the destination device
     * @return Vector of rules using this device as destination
     */
    std::vector<ForwardingRule> getDestinationRules(const juce::Uuid& nodeId,
                                                     uint16_t deviceId) const;

    /**
     * Get all enabled rules
     * @return Vector of enabled rules
     */
    std::vector<ForwardingRule> getEnabledRules() const;

    /**
     * Get all disabled rules
     * @return Vector of disabled rules
     */
    std::vector<ForwardingRule> getDisabledRules() const;

    /**
     * Check if a rule exists
     * @param ruleId The rule ID to check
     * @return true if rule exists, false otherwise
     */
    bool hasRule(const std::string& ruleId) const;

    /**
     * Get number of rules
     * @return Total number of rules
     */
    int getRuleCount() const;

    //==============================================================================
    // Validation

    /**
     * Validate a forwarding rule
     * Checks:
     * - Rule structure is valid (ruleId, source != destination)
     * - Source device exists in DeviceRegistry
     * - Destination device exists in DeviceRegistry
     * - Channel filter is valid if present
     *
     * @param rule The rule to validate
     * @param errorMsg Output parameter for error message
     * @return true if valid, false otherwise
     */
    bool validateRule(const ForwardingRule& rule, std::string& errorMsg) const;

    //==============================================================================
    // Persistence

    /**
     * Load rules from a JSON file
     * Replaces all current rules with loaded rules
     * @param file The file to load from
     * @return true if successful, false on error
     */
    bool loadFromFile(const juce::File& file);

    /**
     * Save rules to a JSON file
     * @param file The file to save to
     * @return true if successful, false on error
     */
    bool saveToFile(const juce::File& file) const;

    //==============================================================================
    // Statistics

    /**
     * Get aggregate statistics for all rules
     * @return Statistics structure with aggregated data
     */
    Statistics getStatistics() const;

    /**
     * Reset all rule statistics
     * Clears message counts and last forwarded timestamps
     */
    void resetStatistics();

    /**
     * Update statistics for a rule (called during message forwarding)
     * @param ruleId The rule ID to update
     * @param forwarded true if message was forwarded, false if dropped
     */
    void updateRuleStatistics(const std::string& ruleId, bool forwarded);

private:
    // Reference to device registry for validation
    DeviceRegistry& deviceRegistry;

    // Thread-safe rule storage (keyed by ruleId)
    mutable std::mutex rulesMutex;
    std::map<std::string, ForwardingRule> rules;

    // Performance optimization: Indexed lookup cache for fast destination queries
    // Maps source DeviceKey to vector of enabled rules, pre-sorted by priority
    // Rebuilt whenever rules are added/removed/updated
    // Trade-off: More memory, slower writes, much faster reads (O(log N) vs O(N))
    std::map<DeviceKey, std::vector<ForwardingRule>> sourceIndex;

    // Helper methods
    bool validateRuleInternal(const ForwardingRule& rule, std::string& errorMsg) const;
    std::string generateRuleId() const;

    /**
     * Rebuild the source device index for fast lookups
     * Called after any rule modification (add/remove/update)
     * Time complexity: O(N log N) where N = number of rules
     */
    void rebuildSourceIndex();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RouteManager)
};

} // namespace NetworkMidi
