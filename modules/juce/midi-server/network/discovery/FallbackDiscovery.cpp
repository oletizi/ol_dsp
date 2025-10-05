#include "FallbackDiscovery.h"
#include <iostream>
#include <sstream>
#include <chrono>

//==============================================================================
FallbackDiscovery::FallbackDiscovery(const juce::Uuid& nodeId,
                                     const juce::String& nodeName,
                                     int httpPort,
                                     int udpPort,
                                     int deviceCount)
    : nodeId(nodeId)
    , nodeName(nodeName)
    , httpPort(httpPort)
    , udpPort(udpPort)
    , deviceCount(deviceCount)
{
}

FallbackDiscovery::~FallbackDiscovery()
{
    stopBroadcasting();
    stopListening();
}

//==============================================================================
// Broadcasting
//==============================================================================
bool FallbackDiscovery::startBroadcasting()
{
    std::lock_guard<std::mutex> lock(mutex);

    if (broadcastRunning)
    {
        std::cerr << "FallbackDiscovery: Already broadcasting" << std::endl;
        return true;
    }

    broadcastSocket = std::make_unique<juce::DatagramSocket>();

    if (!broadcastSocket->bindToPort(0))  // Bind to any available port
    {
        std::cerr << "FallbackDiscovery: Failed to bind broadcast socket" << std::endl;
        broadcastSocket.reset();
        return false;
    }

    broadcastRunning = true;
    broadcastThread = std::make_unique<std::thread>([this]() {
        broadcastLoop();
    });

    std::cout << "FallbackDiscovery: Broadcasting to " << MULTICAST_ADDRESS
              << ":" << MULTICAST_PORT << " every " << BROADCAST_INTERVAL_MS << "ms" << std::endl;

    return true;
}

void FallbackDiscovery::stopBroadcasting()
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!broadcastRunning)
        return;

    broadcastRunning = false;

    if (broadcastThread && broadcastThread->joinable())
    {
        broadcastThread->join();
        broadcastThread.reset();
    }

    broadcastSocket.reset();
    std::cout << "FallbackDiscovery: Stopped broadcasting" << std::endl;
}

bool FallbackDiscovery::isBroadcasting() const
{
    return broadcastRunning;
}

void FallbackDiscovery::broadcastLoop()
{
    while (broadcastRunning)
    {
        sendAnnouncement();

        // Sleep in small increments to allow for quick shutdown
        for (int i = 0; i < BROADCAST_INTERVAL_MS / 100 && broadcastRunning; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "FallbackDiscovery: Broadcast loop exited" << std::endl;
}

juce::String FallbackDiscovery::createAnnouncementJson()
{
    std::ostringstream oss;
    oss << "{"
        << "\"uuid\":\"" << nodeId.toString() << "\","
        << "\"name\":\"" << nodeName << "\","
        << "\"hostname\":\"" << juce::SystemStats::getComputerName() << "\","
        << "\"http_port\":" << httpPort << ","
        << "\"udp_port\":" << udpPort << ","
        << "\"version\":\"1.0\","
        << "\"devices\":" << deviceCount
        << "}";
    return juce::String(oss.str());
}

bool FallbackDiscovery::sendAnnouncement()
{
    if (!broadcastSocket)
        return false;

    juce::String announcement = createAnnouncementJson();
    int bytesSent = broadcastSocket->write(
        MULTICAST_ADDRESS,
        MULTICAST_PORT,
        announcement.toRawUTF8(),
        announcement.length()
    );

    if (bytesSent < 0)
    {
        std::cerr << "FallbackDiscovery: Failed to send announcement" << std::endl;
        return false;
    }

    return true;
}

//==============================================================================
// Listening
//==============================================================================
bool FallbackDiscovery::startListening(ServiceDiscoveredCallback onDiscovered,
                                       ServiceRemovedCallback onRemoved)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (listenRunning)
    {
        std::cerr << "FallbackDiscovery: Already listening" << std::endl;
        return true;
    }

    onDiscoveredCallback = onDiscovered;
    onRemovedCallback = onRemoved;

    listenSocket = std::make_unique<juce::DatagramSocket>();

    // Bind to multicast port
    if (!listenSocket->bindToPort(MULTICAST_PORT))
    {
        std::cerr << "FallbackDiscovery: Failed to bind listen socket to port " << MULTICAST_PORT << std::endl;
        listenSocket.reset();
        return false;
    }

    // Join multicast group
    if (!listenSocket->joinMulticast(MULTICAST_ADDRESS))
    {
        std::cerr << "FallbackDiscovery: Failed to join multicast group " << MULTICAST_ADDRESS << std::endl;
        listenSocket.reset();
        return false;
    }

    // Set socket to non-blocking mode with timeout
    listenSocket->setEnablePortReuse(true);

    listenRunning = true;
    listenThread = std::make_unique<std::thread>([this]() {
        listenLoop();
    });

    // Start timeout check thread
    timeoutCheckRunning = true;
    timeoutThread = std::make_unique<std::thread>([this]() {
        timeoutCheckLoop();
    });

    std::cout << "FallbackDiscovery: Listening on " << MULTICAST_ADDRESS
              << ":" << MULTICAST_PORT << std::endl;

    return true;
}

void FallbackDiscovery::stopListening()
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!listenRunning)
        return;

    listenRunning = false;
    timeoutCheckRunning = false;

    if (listenThread && listenThread->joinable())
    {
        listenThread->join();
        listenThread.reset();
    }

    if (timeoutThread && timeoutThread->joinable())
    {
        timeoutThread->join();
        timeoutThread.reset();
    }

    if (listenSocket)
    {
        listenSocket->leaveMulticast(MULTICAST_ADDRESS);
        listenSocket.reset();
    }

    onDiscoveredCallback = nullptr;
    onRemovedCallback = nullptr;
    discoveredNodes.clear();

    std::cout << "FallbackDiscovery: Stopped listening" << std::endl;
}

bool FallbackDiscovery::isListening() const
{
    return listenRunning;
}

void FallbackDiscovery::listenLoop()
{
    char buffer[2048];

    while (listenRunning)
    {
        juce::String senderAddress;
        int senderPort;

        int bytesRead = listenSocket->waitUntilReady(true, 1000);  // 1 second timeout

        if (bytesRead > 0)
        {
            bytesRead = listenSocket->read(buffer, sizeof(buffer) - 1, false, senderAddress, senderPort);

            if (bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                juce::String announcement(buffer);
                processAnnouncement(announcement, senderAddress);
            }
        }
    }

    std::cout << "FallbackDiscovery: Listen loop exited" << std::endl;
}

void FallbackDiscovery::processAnnouncement(const juce::String& announcement, const juce::String& fromAddress)
{
    NodeInfo nodeInfo = parseAnnouncementJson(announcement);

    if (!nodeInfo.isValid())
    {
        std::cerr << "FallbackDiscovery: Invalid announcement from " << fromAddress << std::endl;
        return;
    }

    // Skip self-discovery
    if (nodeInfo.uuid == nodeId)
    {
        return;
    }

    // Set IP address from sender
    nodeInfo.ipAddress = fromAddress;

    std::lock_guard<std::mutex> lock(mutex);

    // Check if this is a new node
    auto it = discoveredNodes.find(nodeInfo.uuid);
    bool isNewNode = (it == discoveredNodes.end());

    // Update or add node
    auto now = juce::Time::currentTimeMillis();
    discoveredNodes[nodeInfo.uuid] = DiscoveredNode{nodeInfo, now};

    if (isNewNode && onDiscoveredCallback)
    {
        std::cout << "FallbackDiscovery: Discovered node: " << nodeInfo.name
                  << " (UUID: " << nodeInfo.uuid.toString() << ") from " << fromAddress << std::endl;
        onDiscoveredCallback(nodeInfo);
    }
}

NodeInfo FallbackDiscovery::parseAnnouncementJson(const juce::String& json)
{
    NodeInfo info;

    // Simple JSON parsing (no external dependencies)
    auto getValue = [&json](const juce::String& key) -> juce::String {
        int start = json.indexOf("\"" + key + "\":");
        if (start < 0)
            return {};

        start = json.indexOfChar(start, ':') + 1;

        // Skip whitespace
        while (start < json.length() && (json[start] == ' ' || json[start] == '\t'))
            ++start;

        // String value
        if (json[start] == '"')
        {
            ++start;
            int end = json.indexOfChar(start, '"');
            if (end < 0)
                return {};
            return json.substring(start, end);
        }
        // Numeric value
        else
        {
            int end = start;
            while (end < json.length() &&
                   (juce::CharacterFunctions::isDigit(json[end]) || json[end] == '.'))
            {
                ++end;
            }
            return json.substring(start, end);
        }
    };

    info.uuid = juce::Uuid(getValue("uuid"));
    info.name = getValue("name");
    info.hostname = getValue("hostname");
    info.httpPort = getValue("http_port").getIntValue();
    info.udpPort = getValue("udp_port").getIntValue();
    info.version = getValue("version");
    info.deviceCount = getValue("devices").getIntValue();

    return info;
}

//==============================================================================
// Timeout Detection
//==============================================================================
void FallbackDiscovery::timeoutCheckLoop()
{
    while (timeoutCheckRunning)
    {
        checkForTimeouts();

        // Sleep in small increments to allow for quick shutdown
        for (int i = 0; i < 5000 / 100 && timeoutCheckRunning; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "FallbackDiscovery: Timeout check loop exited" << std::endl;
}

void FallbackDiscovery::checkForTimeouts()
{
    std::lock_guard<std::mutex> lock(mutex);

    auto now = juce::Time::currentTimeMillis();
    std::vector<juce::Uuid> timedOutNodes;

    for (auto it = discoveredNodes.begin(); it != discoveredNodes.end(); )
    {
        if (now - it->second.lastSeenTime > TIMEOUT_MS)
        {
            std::cout << "FallbackDiscovery: Node timed out: " << it->second.info.name
                      << " (UUID: " << it->first.toString() << ")" << std::endl;
            timedOutNodes.push_back(it->first);
            it = discoveredNodes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Call removal callbacks outside the loop
    if (onRemovedCallback)
    {
        for (const auto& uuid : timedOutNodes)
        {
            onRemovedCallback(uuid);
        }
    }
}

//==============================================================================
// Device Count Update
//==============================================================================
void FallbackDiscovery::updateDeviceCount(int count)
{
    deviceCount = count;
    // Next broadcast will include updated count
}
