/**
 * RouteManager.cpp
 *
 * Implementation of thread-safe MIDI forwarding rule manager
 */

#include "RouteManager.h"
#include <algorithm>

namespace NetworkMidi {

RouteManager::RouteManager(DeviceRegistry& registry)
    : deviceRegistry(registry)
{
}

RouteManager::~RouteManager()
{
    std::lock_guard<std::mutex> lock(rulesMutex);
    rules.clear();
}

//==============================================================================
// Rule management (CRUD operations)

std::string RouteManager::addRule(const ForwardingRule& rule)
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    // Create a copy to potentially modify
    ForwardingRule newRule = rule;

    // Generate rule ID if not provided
    if (newRule.ruleId.isEmpty()) {
        newRule.ruleId = generateRuleId();
    }

    // Validate the rule
    std::string errorMsg;
    if (!validateRuleInternal(newRule, errorMsg)) {
        throw std::runtime_error("Invalid forwarding rule: " + errorMsg);
    }

    // Check for duplicate rule ID
    if (rules.find(newRule.ruleId.toStdString()) != rules.end()) {
        throw std::runtime_error("Rule with ID '" + newRule.ruleId.toStdString() + "' already exists");
    }

    // Add the rule
    rules[newRule.ruleId.toStdString()] = newRule;

    return newRule.ruleId.toStdString();
}

bool RouteManager::removeRule(const std::string& ruleId)
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    auto it = rules.find(ruleId);
    if (it == rules.end()) {
        return false;
    }

    rules.erase(it);
    return true;
}

bool RouteManager::updateRule(const std::string& ruleId, const ForwardingRule& rule)
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    // Check if rule exists
    auto it = rules.find(ruleId);
    if (it == rules.end()) {
        return false;
    }

    // Create updated rule with correct ID
    ForwardingRule updatedRule = rule;
    updatedRule.ruleId = juce::String(ruleId);

    // Validate the updated rule
    std::string errorMsg;
    if (!validateRuleInternal(updatedRule, errorMsg)) {
        throw std::runtime_error("Invalid forwarding rule update: " + errorMsg);
    }

    // Preserve statistics from old rule
    updatedRule.statistics = it->second.statistics;

    // Update the rule
    it->second = updatedRule;

    return true;
}

std::optional<ForwardingRule> RouteManager::getRule(const std::string& ruleId) const
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    auto it = rules.find(ruleId);
    if (it != rules.end()) {
        return it->second;
    }

    return std::nullopt;
}

std::vector<ForwardingRule> RouteManager::getAllRules() const
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    std::vector<ForwardingRule> result;
    result.reserve(rules.size());

    for (const auto& pair : rules) {
        result.push_back(pair.second);
    }

    return result;
}

void RouteManager::clearAllRules()
{
    std::lock_guard<std::mutex> lock(rulesMutex);
    rules.clear();
}

//==============================================================================
// Query operations

std::vector<ForwardingRule> RouteManager::getDestinations(const juce::Uuid& sourceNodeId,
                                                           uint16_t sourceDeviceId) const
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    std::vector<ForwardingRule> result;
    DeviceKey sourceKey(sourceNodeId, sourceDeviceId);

    // Find all rules with matching source
    for (const auto& pair : rules) {
        if (pair.second.sourceDevice == sourceKey && pair.second.enabled) {
            result.push_back(pair.second);
        }
    }

    // Sort by priority (highest first)
    std::sort(result.begin(), result.end(),
              [](const ForwardingRule& a, const ForwardingRule& b) {
                  return a.priority > b.priority;  // Higher priority first
              });

    return result;
}

std::vector<ForwardingRule> RouteManager::getSourceRules(const juce::Uuid& nodeId,
                                                          uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    std::vector<ForwardingRule> result;
    DeviceKey sourceKey(nodeId, deviceId);

    for (const auto& pair : rules) {
        if (pair.second.sourceDevice == sourceKey) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<ForwardingRule> RouteManager::getDestinationRules(const juce::Uuid& nodeId,
                                                               uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    std::vector<ForwardingRule> result;
    DeviceKey destKey(nodeId, deviceId);

    for (const auto& pair : rules) {
        if (pair.second.destinationDevice == destKey) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<ForwardingRule> RouteManager::getEnabledRules() const
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    std::vector<ForwardingRule> result;

    for (const auto& pair : rules) {
        if (pair.second.enabled) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<ForwardingRule> RouteManager::getDisabledRules() const
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    std::vector<ForwardingRule> result;

    for (const auto& pair : rules) {
        if (!pair.second.enabled) {
            result.push_back(pair.second);
        }
    }

    return result;
}

bool RouteManager::hasRule(const std::string& ruleId) const
{
    std::lock_guard<std::mutex> lock(rulesMutex);
    return rules.find(ruleId) != rules.end();
}

int RouteManager::getRuleCount() const
{
    std::lock_guard<std::mutex> lock(rulesMutex);
    return static_cast<int>(rules.size());
}

//==============================================================================
// Validation

bool RouteManager::validateRule(const ForwardingRule& rule, std::string& errorMsg) const
{
    std::lock_guard<std::mutex> lock(rulesMutex);
    return validateRuleInternal(rule, errorMsg);
}

bool RouteManager::validateRuleInternal(const ForwardingRule& rule, std::string& errorMsg) const
{
    // Check basic rule validity
    if (!rule.isValid()) {
        if (rule.ruleId.isEmpty()) {
            errorMsg = "Rule ID cannot be empty";
        } else if (rule.sourceDevice == rule.destinationDevice) {
            errorMsg = "Source and destination devices must be different";
        } else if (rule.channelFilter.has_value() && !rule.channelFilter->isValid()) {
            errorMsg = "Channel filter is invalid (must be 0-16)";
        } else {
            errorMsg = "Rule failed basic validation";
        }
        return false;
    }

    // Validate source device exists
    if (!deviceRegistry.hasDevice(rule.sourceDevice.ownerNode, rule.sourceDevice.deviceId)) {
        errorMsg = "Source device (" + rule.sourceDevice.ownerNode.toString().toStdString() +
                   ", " + std::to_string(rule.sourceDevice.deviceId) + ") does not exist";
        return false;
    }

    // Validate destination device exists
    if (!deviceRegistry.hasDevice(rule.destinationDevice.ownerNode, rule.destinationDevice.deviceId)) {
        errorMsg = "Destination device (" + rule.destinationDevice.ownerNode.toString().toStdString() +
                   ", " + std::to_string(rule.destinationDevice.deviceId) + ") does not exist";
        return false;
    }

    // Validate source and destination types are compatible
    auto sourceDevice = deviceRegistry.getDevice(rule.sourceDevice.ownerNode, rule.sourceDevice.deviceId);
    auto destDevice = deviceRegistry.getDevice(rule.destinationDevice.ownerNode, rule.destinationDevice.deviceId);

    if (sourceDevice.has_value() && destDevice.has_value()) {
        // Source must be an input (receives MIDI from hardware)
        // Destination must be an output (sends MIDI to hardware)
        if (sourceDevice->type != "input") {
            errorMsg = "Source device must be of type 'input', found '" + sourceDevice->type.toStdString() + "'";
            return false;
        }

        if (destDevice->type != "output") {
            errorMsg = "Destination device must be of type 'output', found '" + destDevice->type.toStdString() + "'";
            return false;
        }
    }

    errorMsg.clear();
    return true;
}

//==============================================================================
// Persistence

bool RouteManager::loadFromFile(const juce::File& file)
{
    if (!file.existsAsFile()) {
        return false;
    }

    try {
        // Read file contents
        juce::String jsonContent = file.loadFileAsString();
        auto parsed = juce::JSON::parse(jsonContent);

        if (!parsed.isArray()) {
            return false;
        }

        // Parse rules array
        std::vector<ForwardingRule> loadedRules;
        auto* rulesArray = parsed.getArray();

        for (const auto& ruleVar : *rulesArray) {
            ForwardingRule rule = ForwardingRule::fromVar(ruleVar);
            loadedRules.push_back(rule);
        }

        // Replace current rules with loaded rules
        std::lock_guard<std::mutex> lock(rulesMutex);
        rules.clear();

        for (const auto& rule : loadedRules) {
            rules[rule.ruleId.toStdString()] = rule;
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool RouteManager::saveToFile(const juce::File& file) const
{
    try {
        std::lock_guard<std::mutex> lock(rulesMutex);

        // Create JSON array of rules
        juce::Array<juce::var> rulesArray;

        for (const auto& pair : rules) {
            rulesArray.add(pair.second.toVar());
        }

        // Convert to JSON string
        juce::String jsonContent = juce::JSON::toString(juce::var(rulesArray), true);

        // Write to file
        file.replaceWithText(jsonContent);

        return true;
    } catch (...) {
        return false;
    }
}

//==============================================================================
// Statistics

RouteManager::Statistics RouteManager::getStatistics() const
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    Statistics stats;
    stats.totalRules = rules.size();

    for (const auto& pair : rules) {
        const auto& rule = pair.second;

        if (rule.enabled) {
            stats.enabledRules++;
        } else {
            stats.disabledRules++;
        }

        stats.totalMessagesForwarded += rule.statistics.messagesForwarded;
        stats.totalMessagesDropped += rule.statistics.messagesDropped;
    }

    return stats;
}

void RouteManager::resetStatistics()
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    for (auto& pair : rules) {
        pair.second.statistics.reset();
    }
}

void RouteManager::updateRuleStatistics(const std::string& ruleId, bool forwarded)
{
    std::lock_guard<std::mutex> lock(rulesMutex);

    auto it = rules.find(ruleId);
    if (it != rules.end()) {
        if (forwarded) {
            it->second.statistics.incrementForwarded();
        } else {
            it->second.statistics.incrementDropped();
        }
    }
}

//==============================================================================
// Private helper methods

std::string RouteManager::generateRuleId() const
{
    return juce::Uuid().toString().toStdString();
}

} // namespace NetworkMidi
