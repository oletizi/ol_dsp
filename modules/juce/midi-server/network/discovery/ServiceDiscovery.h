#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <memory>
#include <string>

/**
 * @brief Information about a discovered network MIDI node
 */
struct NodeInfo
{
    juce::Uuid uuid;
    juce::String name;
    juce::String hostname;
    juce::String ipAddress;
    int httpPort = 0;
    int udpPort = 0;
    juce::String version;
    int deviceCount = 0;

    bool isValid() const {
        return !uuid.isNull() && httpPort > 0 && udpPort > 0;
    }
};

/**
 * @brief Service discovery for network MIDI mesh using mDNS/Bonjour
 *
 * This class provides zero-configuration service discovery using platform-specific
 * mDNS implementations (Bonjour on macOS, Avahi on Linux, Bonjour for Windows).
 *
 * Service Type: _midi-network._tcp.local.
 *
 * TXT Records:
 * - uuid: Unique node identifier
 * - http_port: HTTP API port
 * - udp_port: UDP MIDI transport port
 * - hostname: System hostname
 * - version: Protocol version
 * - devices: Number of local MIDI devices
 *
 * Thread Safety: All methods are thread-safe
 * Real-time Safety: Not real-time safe (uses locks and network I/O)
 */
class ServiceDiscovery
{
public:
    /** Callback types for service discovery events */
    using ServiceDiscoveredCallback = std::function<void(const NodeInfo&)>;
    using ServiceRemovedCallback = std::function<void(const juce::Uuid&)>;

    /**
     * @brief Construct a new Service Discovery object
     *
     * @param nodeId Unique identifier for this node
     * @param nodeName Human-readable name for this node
     * @param httpPort HTTP API port for this node
     * @param udpPort UDP transport port for this node
     * @param deviceCount Number of local MIDI devices
     */
    ServiceDiscovery(const juce::Uuid& nodeId,
                     const juce::String& nodeName,
                     int httpPort,
                     int udpPort,
                     int deviceCount = 0);

    /** Destructor - stops advertising and browsing */
    ~ServiceDiscovery();

    // Non-copyable, moveable
    ServiceDiscovery(const ServiceDiscovery&) = delete;
    ServiceDiscovery& operator=(const ServiceDiscovery&) = delete;
    ServiceDiscovery(ServiceDiscovery&&) noexcept;
    ServiceDiscovery& operator=(ServiceDiscovery&&) noexcept;

    /**
     * @brief Start advertising this node on the network
     *
     * Publishes mDNS service with TXT records containing node information.
     * Service type: _midi-network._tcp.local.
     *
     * @return true if advertising started successfully
     */
    bool advertise();

    /**
     * @brief Stop advertising this node
     */
    void stopAdvertising();

    /**
     * @brief Check if currently advertising
     */
    bool isAdvertising() const;

    /**
     * @brief Start browsing for other nodes on the network
     *
     * @param onDiscovered Callback when a new node is discovered
     * @param onRemoved Callback when a node disappears
     * @return true if browsing started successfully
     */
    bool startBrowsing(ServiceDiscoveredCallback onDiscovered,
                       ServiceRemovedCallback onRemoved);

    /**
     * @brief Stop browsing for nodes
     */
    void stopBrowsing();

    /**
     * @brief Check if currently browsing
     */
    bool isBrowsing() const;

    /**
     * @brief Update the device count in the advertised TXT record
     *
     * @param count New device count
     */
    void updateDeviceCount(int count);

    /**
     * @brief Get the node ID for this service
     */
    juce::Uuid getNodeId() const { return nodeId; }

    /**
     * @brief Get the node name for this service
     */
    juce::String getNodeName() const { return nodeName; }

private:
    // Platform-specific implementation (pImpl idiom)
    struct Impl;
    std::unique_ptr<Impl> impl;

    // Node information
    juce::Uuid nodeId;
    juce::String nodeName;
    int httpPort;
    int udpPort;
    int deviceCount;
};
