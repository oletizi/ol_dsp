/**
 * RoutingTable.cpp
 *
 * Implementation of thread-safe routing table
 */

#include "RoutingTable.h"
#include <algorithm>

namespace NetworkMidi {

RoutingTable::RoutingTable()
{
}

RoutingTable::~RoutingTable()
{
    std::lock_guard<std::mutex> lock(routeMutex);
    routes.clear();
}

//==============================================================================
// Route management

void RoutingTable::addRoute(const juce::Uuid& nodeId,
                            uint16_t deviceId,
                            const juce::String& deviceName,
                            const juce::String& deviceType)
{
    Route route(nodeId, deviceId, deviceName, deviceType);
    addRouteInternal(route);
}

void RoutingTable::removeRoute(const juce::Uuid& nodeId, uint16_t deviceId)
{
    std::lock_guard<std::mutex> lock(routeMutex);
    RouteKey key(nodeId, deviceId);
    routes.erase(key);
}

void RoutingTable::removeNodeRoutes(const juce::Uuid& nodeId)
{
    std::lock_guard<std::mutex> lock(routeMutex);

    // Remove all routes for specified node
    auto it = routes.begin();
    while (it != routes.end()) {
        if (it->second.key.nodeId == nodeId) {
            it = routes.erase(it);
        } else {
            ++it;
        }
    }
}

void RoutingTable::clearAllRoutes()
{
    std::lock_guard<std::mutex> lock(routeMutex);
    routes.clear();
}

//==============================================================================
// Route queries

std::optional<Route> RoutingTable::getRoute(const juce::Uuid& nodeId,
                                            uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    RouteKey key(nodeId, deviceId);
    auto it = routes.find(key);
    if (it != routes.end()) {
        return it->second;
    }

    return std::nullopt;
}

std::optional<Route> RoutingTable::getLocalRoute(uint16_t deviceId) const
{
    return getRoute(juce::Uuid::null(), deviceId);
}

std::vector<Route> RoutingTable::getAllRoutes() const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    std::vector<Route> result;
    result.reserve(routes.size());

    for (const auto& pair : routes) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<Route> RoutingTable::getLocalRoutes() const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    std::vector<Route> result;

    for (const auto& pair : routes) {
        if (pair.second.isLocal()) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<Route> RoutingTable::getRemoteRoutes() const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    std::vector<Route> result;

    for (const auto& pair : routes) {
        if (!pair.second.isLocal()) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<Route> RoutingTable::getNodeRoutes(const juce::Uuid& nodeId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    std::vector<Route> result;

    for (const auto& pair : routes) {
        if (pair.second.key.nodeId == nodeId) {
            result.push_back(pair.second);
        }
    }

    return result;
}

//==============================================================================
// Route checks

bool RoutingTable::hasRoute(const juce::Uuid& nodeId, uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);
    RouteKey key(nodeId, deviceId);
    return routes.find(key) != routes.end();
}

bool RoutingTable::hasLocalRoute(uint16_t deviceId) const
{
    return hasRoute(juce::Uuid::null(), deviceId);
}

bool RoutingTable::isLocalDevice(const juce::Uuid& nodeId, uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    RouteKey key(nodeId, deviceId);
    auto it = routes.find(key);
    if (it != routes.end()) {
        return it->second.isLocal();
    }

    return false;
}

bool RoutingTable::isRemoteDevice(const juce::Uuid& nodeId, uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    RouteKey key(nodeId, deviceId);
    auto it = routes.find(key);
    if (it != routes.end()) {
        return !it->second.isLocal();
    }

    return false;
}

//==============================================================================
// Statistics

int RoutingTable::getTotalRouteCount() const
{
    std::lock_guard<std::mutex> lock(routeMutex);
    return static_cast<int>(routes.size());
}

int RoutingTable::getLocalRouteCount() const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    int count = 0;
    for (const auto& pair : routes) {
        if (pair.second.isLocal()) {
            ++count;
        }
    }

    return count;
}

int RoutingTable::getRemoteRouteCount() const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    int count = 0;
    for (const auto& pair : routes) {
        if (!pair.second.isLocal()) {
            ++count;
        }
    }

    return count;
}

int RoutingTable::getNodeRouteCount(const juce::Uuid& nodeId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    int count = 0;
    for (const auto& pair : routes) {
        if (pair.second.key.nodeId == nodeId) {
            ++count;
        }
    }

    return count;
}

//==============================================================================
// Bulk operations

void RoutingTable::addRoutes(const std::vector<Route>& routeList)
{
    std::lock_guard<std::mutex> lock(routeMutex);

    for (const auto& route : routeList) {
        routes[route.key] = route;
    }
}

void RoutingTable::replaceNodeRoutes(const juce::Uuid& nodeId,
                                     const std::vector<Route>& routeList)
{
    std::lock_guard<std::mutex> lock(routeMutex);

    // First, remove all existing routes for this node
    auto it = routes.begin();
    while (it != routes.end()) {
        if (it->second.key.nodeId == nodeId) {
            it = routes.erase(it);
        } else {
            ++it;
        }
    }

    // Then add the new routes
    for (const auto& route : routeList) {
        routes[route.key] = route;
    }
}

//==============================================================================
// Private helper methods

void RoutingTable::addRouteInternal(const Route& route)
{
    std::lock_guard<std::mutex> lock(routeMutex);
    routes[route.key] = route;
}

} // namespace NetworkMidi
