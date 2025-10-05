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

void RoutingTable::addRoute(uint16_t deviceId,
                            const juce::Uuid& nodeId,
                            const juce::String& deviceName,
                            const juce::String& deviceType)
{
    Route route(deviceId, nodeId, deviceName, deviceType);
    addRouteInternal(route);
}

void RoutingTable::removeRoute(uint16_t deviceId)
{
    std::lock_guard<std::mutex> lock(routeMutex);
    routes.erase(deviceId);
}

void RoutingTable::removeNodeRoutes(const juce::Uuid& nodeId)
{
    std::lock_guard<std::mutex> lock(routeMutex);

    // Remove all routes for specified node
    auto it = routes.begin();
    while (it != routes.end()) {
        if (it->second.nodeId == nodeId) {
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

std::optional<Route> RoutingTable::getRoute(uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    auto it = routes.find(deviceId);
    if (it != routes.end()) {
        return it->second;
    }

    return std::nullopt;
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
        if (pair.second.nodeId == nodeId) {
            result.push_back(pair.second);
        }
    }

    return result;
}

//==============================================================================
// Route checks

bool RoutingTable::hasRoute(uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);
    return routes.find(deviceId) != routes.end();
}

bool RoutingTable::isLocalDevice(uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    auto it = routes.find(deviceId);
    if (it != routes.end()) {
        return it->second.isLocal();
    }

    return false;
}

bool RoutingTable::isRemoteDevice(uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(routeMutex);

    auto it = routes.find(deviceId);
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
        if (pair.second.nodeId == nodeId) {
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
        routes[route.deviceId] = route;
    }
}

void RoutingTable::replaceNodeRoutes(const juce::Uuid& nodeId,
                                     const std::vector<Route>& routeList)
{
    std::lock_guard<std::mutex> lock(routeMutex);

    // First, remove all existing routes for this node
    auto it = routes.begin();
    while (it != routes.end()) {
        if (it->second.nodeId == nodeId) {
            it = routes.erase(it);
        } else {
            ++it;
        }
    }

    // Then add the new routes
    for (const auto& route : routeList) {
        routes[route.deviceId] = route;
    }
}

//==============================================================================
// Private helper methods

void RoutingTable::addRouteInternal(const Route& route)
{
    std::lock_guard<std::mutex> lock(routeMutex);
    routes[route.deviceId] = route;
}

} // namespace NetworkMidi
