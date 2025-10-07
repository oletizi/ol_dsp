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
 * Composite key for uniquely identifying routes across the mesh
 * Uses (nodeId, deviceId) pair to avoid ID conflicts between nodes
 */
struct RouteKey {
    juce::Uuid nodeId;  // Null UUID for local devices
    uint16_t deviceId;

    RouteKey() : deviceId(0) {}

    RouteKey(const juce::Uuid& node, uint16_t id)
        : nodeId(node), deviceId(id) {}

    bool isLocal() const {
        return nodeId.isNull();
    }

    bool operator<(const RouteKey& other) const {
        if (nodeId != other.nodeId)
            return nodeId < other.nodeId;
        return deviceId < other.deviceId;
    }

    bool operator==(const RouteKey& other) const {
        return nodeId == other.nodeId && deviceId == other.deviceId;
    }

    bool operator!=(const RouteKey& other) const {
        return !(*this == other);
    }
};

/**
 * Route entry mapping a device to its owner node
 */
struct Route {
    RouteKey key;                   // Composite key (nodeId, deviceId)
    juce::String deviceName;        // Human-readable device name
    juce::String deviceType;        // "input" or "output"

    Route() {}

    Route(const juce::Uuid& node,
          uint16_t id,
          const juce::String& name,
          const juce::String& type)
        : key(node, id)
        , deviceName(name)
        , deviceType(type)
    {}

    bool isLocal() const {
        return key.isLocal();
    }

    // Convenience accessors
    uint16_t deviceId() const { return key.deviceId; }
    const juce::Uuid& nodeId() const { return key.nodeId; }

    bool operator==(const Route& other) const {
        return key == other.key;
    }
};

/**
 * Thread-safe routing table for device-to-node mapping
 *
 * Design (Phase 1: Device ID Namespacing):
 * - Uses composite keys (nodeId, deviceId) to prevent ID conflicts
 * - Local devices have nodeId == Uuid::null()
 * - Remote devices have nodeId set to owning node's UUID
 * - Fast O(log n) lookup by composite key
 * - Backward-compatible APIs for local-only lookups
 * - Support for bulk operations (add/remove by node)
 */
class RoutingTable {
public:
    RoutingTable();
    ~RoutingTable();

    // Route management (updated for composite keys)
    void addRoute(const juce::Uuid& nodeId,
                  uint16_t deviceId,
                  const juce::String& deviceName,
                  const juce::String& deviceType);

    void removeRoute(const juce::Uuid& nodeId, uint16_t deviceId);
    void removeNodeRoutes(const juce::Uuid& nodeId);
    void clearAllRoutes();

    // Route queries (namespaced with composite keys)
    std::optional<Route> getRoute(const juce::Uuid& nodeId, uint16_t deviceId) const;
    std::optional<Route> getLocalRoute(uint16_t deviceId) const;
    std::vector<Route> getAllRoutes() const;
    std::vector<Route> getLocalRoutes() const;
    std::vector<Route> getRemoteRoutes() const;
    std::vector<Route> getNodeRoutes(const juce::Uuid& nodeId) const;

    // Route checks (namespaced)
    bool hasRoute(const juce::Uuid& nodeId, uint16_t deviceId) const;
    bool hasLocalRoute(uint16_t deviceId) const;
    bool isLocalDevice(const juce::Uuid& nodeId, uint16_t deviceId) const;
    bool isRemoteDevice(const juce::Uuid& nodeId, uint16_t deviceId) const;

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
    // Thread-safe route storage (now using composite keys)
    mutable std::mutex routeMutex;
    std::map<RouteKey, Route> routes;

    // Helper methods
    void addRouteInternal(const Route& route);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingTable)
};

} // namespace NetworkMidi
