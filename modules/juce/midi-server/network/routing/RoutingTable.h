/**
 * RoutingTable.h
 *
 * Thread-safe routing table that maps device IDs to their owning nodes.
 * Used by MidiRouter to determine whether to deliver messages locally
 * or send them over the network.
 */

#pragma once

#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <mutex>
#include <optional>

namespace NetworkMidi {

/**
 * Route entry mapping a device to its owner node
 */
struct Route {
    uint16_t deviceId;              // Global device ID
    juce::Uuid nodeId;              // Owner node UUID (null for local devices)
    juce::String deviceName;        // Human-readable device name
    juce::String deviceType;        // "input" or "output"

    Route() : deviceId(0) {}

    Route(uint16_t id,
          const juce::Uuid& node,
          const juce::String& name,
          const juce::String& type)
        : deviceId(id)
        , nodeId(node)
        , deviceName(name)
        , deviceType(type)
    {}

    bool isLocal() const {
        return nodeId.isNull();
    }

    bool operator==(const Route& other) const {
        return deviceId == other.deviceId;
    }
};

/**
 * Thread-safe routing table for device-to-node mapping
 *
 * Design:
 * - Local devices have nodeId == Uuid::null()
 * - Remote devices have nodeId set to owning node's UUID
 * - Fast O(1) lookup by device ID
 * - Support for bulk operations (add/remove by node)
 */
class RoutingTable {
public:
    RoutingTable();
    ~RoutingTable();

    // Route management
    void addRoute(uint16_t deviceId,
                  const juce::Uuid& nodeId,
                  const juce::String& deviceName,
                  const juce::String& deviceType);

    void removeRoute(uint16_t deviceId);
    void removeNodeRoutes(const juce::Uuid& nodeId);
    void clearAllRoutes();

    // Route queries
    std::optional<Route> getRoute(uint16_t deviceId) const;
    std::vector<Route> getAllRoutes() const;
    std::vector<Route> getLocalRoutes() const;
    std::vector<Route> getRemoteRoutes() const;
    std::vector<Route> getNodeRoutes(const juce::Uuid& nodeId) const;

    // Route checks
    bool hasRoute(uint16_t deviceId) const;
    bool isLocalDevice(uint16_t deviceId) const;
    bool isRemoteDevice(uint16_t deviceId) const;

    // Statistics
    int getTotalRouteCount() const;
    int getLocalRouteCount() const;
    int getRemoteRouteCount() const;
    int getNodeRouteCount(const juce::Uuid& nodeId) const;

    // Bulk operations
    void addRoutes(const std::vector<Route>& routes);
    void replaceNodeRoutes(const juce::Uuid& nodeId,
                           const std::vector<Route>& routes);

private:
    // Thread-safe route storage
    mutable std::mutex routeMutex;
    std::map<uint16_t, Route> routes;

    // Helper methods
    void addRouteInternal(const Route& route);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingTable)
};

} // namespace NetworkMidi
