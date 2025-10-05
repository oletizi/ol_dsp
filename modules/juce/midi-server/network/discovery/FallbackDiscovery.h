#pragma once

#include "ServiceDiscovery.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

/**
 * @brief Fallback service discovery using UDP multicast
 *
 * This class provides a simple UDP multicast-based discovery mechanism
 * for environments where mDNS/Bonjour is not available. It broadcasts
 * service announcements to a multicast group and listens for announcements
 * from other nodes.
 *
 * Multicast Group: 239.255.42.99:5353
 * Broadcast Interval: 5 seconds
 *
 * Payload Format (JSON):
 * {
 *   "uuid": "550e8400-e29b-41d4-a716-446655440000",
 *   "name": "studio-mac-a1b2c3d4",
 *   "hostname": "studio-mac.local",
 *   "http_port": 8234,
 *   "udp_port": 9876,
 *   "version": "1.0",
 *   "devices": 3
 * }
 *
 * Thread Safety: All methods are thread-safe
 * Real-time Safety: Not real-time safe (uses locks and network I/O)
 */
class FallbackDiscovery
{
public:
    /** Callback types for service discovery events */
    using ServiceDiscoveredCallback = std::function<void(const NodeInfo&)>;
    using ServiceRemovedCallback = std::function<void(const juce::Uuid&)>;

    /**
     * @brief Construct a new Fallback Discovery object
     *
     * @param nodeId Unique identifier for this node
     * @param nodeName Human-readable name for this node
     * @param httpPort HTTP API port for this node
     * @param udpPort UDP transport port for this node
     * @param deviceCount Number of local MIDI devices
     */
    FallbackDiscovery(const juce::Uuid& nodeId,
                      const juce::String& nodeName,
                      int httpPort,
                      int udpPort,
                      int deviceCount = 0);

    /** Destructor - stops broadcasting and listening */
    ~FallbackDiscovery();

    // Non-copyable, non-moveable
    FallbackDiscovery(const FallbackDiscovery&) = delete;
    FallbackDiscovery& operator=(const FallbackDiscovery&) = delete;

    /**
     * @brief Start broadcasting service announcements
     *
     * Sends UDP multicast packets every 5 seconds containing node information.
     *
     * @return true if broadcasting started successfully
     */
    bool startBroadcasting();

    /**
     * @brief Stop broadcasting service announcements
     */
    void stopBroadcasting();

    /**
     * @brief Check if currently broadcasting
     */
    bool isBroadcasting() const;

    /**
     * @brief Start listening for service announcements
     *
     * @param onDiscovered Callback when a new node is discovered
     * @param onRemoved Callback when a node disappears (timeout)
     * @return true if listening started successfully
     */
    bool startListening(ServiceDiscoveredCallback onDiscovered,
                        ServiceRemovedCallback onRemoved);

    /**
     * @brief Stop listening for announcements
     */
    void stopListening();

    /**
     * @brief Check if currently listening
     */
    bool isListening() const;

    /**
     * @brief Update the device count in broadcast announcements
     *
     * @param count New device count
     */
    void updateDeviceCount(int count);

    /**
     * @brief Get the multicast address being used
     */
    static juce::String getMulticastAddress() { return "239.255.42.99"; }

    /**
     * @brief Get the multicast port being used
     */
    static int getMulticastPort() { return 5353; }

    /**
     * @brief Get the broadcast interval in seconds
     */
    static int getBroadcastInterval() { return 5; }

private:
    // Broadcasting
    void broadcastLoop();
    juce::String createAnnouncementJson();
    bool sendAnnouncement();

    // Listening
    void listenLoop();
    void processAnnouncement(const juce::String& announcement, const juce::String& fromAddress);
    NodeInfo parseAnnouncementJson(const juce::String& json);

    // Timeout detection
    void timeoutCheckLoop();
    void checkForTimeouts();

    // Node information
    juce::Uuid nodeId;
    juce::String nodeName;
    int httpPort;
    int udpPort;
    int deviceCount;

    // Network
    std::unique_ptr<juce::DatagramSocket> broadcastSocket;
    std::unique_ptr<juce::DatagramSocket> listenSocket;

    // Threading
    std::unique_ptr<std::thread> broadcastThread;
    std::unique_ptr<std::thread> listenThread;
    std::unique_ptr<std::thread> timeoutThread;
    std::atomic<bool> broadcastRunning{false};
    std::atomic<bool> listenRunning{false};
    std::atomic<bool> timeoutCheckRunning{false};

    // Callbacks
    ServiceDiscoveredCallback onDiscoveredCallback;
    ServiceRemovedCallback onRemovedCallback;

    // Discovered nodes tracking (for timeout detection)
    struct DiscoveredNode
    {
        NodeInfo info;
        int64_t lastSeenTime;  // milliseconds since epoch
    };
    std::map<juce::Uuid, DiscoveredNode> discoveredNodes;

    // Thread synchronization
    mutable std::mutex mutex;

    // Constants
    static constexpr int BROADCAST_INTERVAL_MS = 5000;
    static constexpr int TIMEOUT_MS = 15000;  // 3 missed broadcasts
    static constexpr const char* MULTICAST_ADDRESS = "239.255.42.99";
    static constexpr int MULTICAST_PORT = 5353;
};
