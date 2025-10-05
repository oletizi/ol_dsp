#include "mdns_macos.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <sstream>

//==============================================================================
MacOSMdnsImpl::MacOSMdnsImpl(const juce::Uuid& nodeId,
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

MacOSMdnsImpl::~MacOSMdnsImpl()
{
    stopAdvertising();
    stopBrowsing();
}

//==============================================================================
// Service Registration (Advertisement)
//==============================================================================
bool MacOSMdnsImpl::advertise()
{
    std::lock_guard<std::mutex> lock(mutex);

    if (registerRef)
    {
        std::cerr << "MacOSMdnsImpl: Already advertising" << std::endl;
        return true;
    }

    auto txtRecord = createTxtRecord();

    DNSServiceErrorType err = DNSServiceRegister(
        &registerRef,
        0,                                  // flags
        0,                                  // interface (all)
        nodeName.toRawUTF8(),              // service name
        SERVICE_TYPE,                       // service type
        nullptr,                           // domain (default)
        nullptr,                           // host (default)
        htons(static_cast<uint16_t>(httpPort)),  // port (network byte order)
        static_cast<uint16_t>(txtRecord.size()), // TXT record length
        txtRecord.data(),                  // TXT record data
        registerCallback,                  // callback
        this                               // context
    );

    if (err != kDNSServiceErr_NoError)
    {
        std::cerr << "MacOSMdnsImpl: DNSServiceRegister failed with error: " << err << std::endl;
        registerRef = nullptr;
        return false;
    }

    // Start processing thread
    registerRunning = true;
    registerThread = std::make_unique<std::thread>([this]() {
        runServiceLoop(registerRef);
    });

    return true;
}

void MacOSMdnsImpl::stopAdvertising()
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!registerRef)
        return;

    registerRunning = false;
    DNSServiceRefDeallocate(registerRef);
    registerRef = nullptr;

    if (registerThread && registerThread->joinable())
    {
        registerThread->join();
        registerThread.reset();
    }
}

void DNSSD_API MacOSMdnsImpl::registerCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    DNSServiceErrorType errorCode,
    const char* name,
    const char* regtype,
    const char* domain,
    void* context)
{
    auto* impl = static_cast<MacOSMdnsImpl*>(context);
    impl->processRegisterCallback(errorCode, name);
}

void MacOSMdnsImpl::processRegisterCallback(DNSServiceErrorType errorCode, const char* name)
{
    if (errorCode == kDNSServiceErr_NoError)
    {
        std::cout << "MacOSMdnsImpl: Service registered successfully: " << name << std::endl;
    }
    else
    {
        std::cerr << "MacOSMdnsImpl: Registration error: " << errorCode << std::endl;
    }
}

//==============================================================================
// Service Browsing (Discovery)
//==============================================================================
bool MacOSMdnsImpl::startBrowsing(
    ServiceDiscovery::ServiceDiscoveredCallback onDiscovered,
    ServiceDiscovery::ServiceRemovedCallback onRemoved)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (browseRef)
    {
        std::cerr << "MacOSMdnsImpl: Already browsing" << std::endl;
        return true;
    }

    onDiscoveredCallback = onDiscovered;
    onRemovedCallback = onRemoved;

    DNSServiceErrorType err = DNSServiceBrowse(
        &browseRef,
        0,                  // flags
        0,                  // interface (all)
        SERVICE_TYPE,       // service type
        nullptr,           // domain (default)
        browseCallback,    // callback
        this               // context
    );

    if (err != kDNSServiceErr_NoError)
    {
        std::cerr << "MacOSMdnsImpl: DNSServiceBrowse failed with error: " << err << std::endl;
        browseRef = nullptr;
        return false;
    }

    // Start processing thread
    browseRunning = true;
    browseThread = std::make_unique<std::thread>([this]() {
        runServiceLoop(browseRef);
    });

    return true;
}

void MacOSMdnsImpl::stopBrowsing()
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!browseRef)
        return;

    browseRunning = false;
    DNSServiceRefDeallocate(browseRef);
    browseRef = nullptr;

    if (browseThread && browseThread->joinable())
    {
        browseThread->join();
        browseThread.reset();
    }

    onDiscoveredCallback = nullptr;
    onRemovedCallback = nullptr;
}

void DNSSD_API MacOSMdnsImpl::browseCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char* serviceName,
    const char* regtype,
    const char* replyDomain,
    void* context)
{
    auto* impl = static_cast<MacOSMdnsImpl*>(context);
    impl->processBrowseCallback(flags, interfaceIndex, errorCode, serviceName, replyDomain);
}

void MacOSMdnsImpl::processBrowseCallback(
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char* serviceName,
    const char* replyDomain)
{
    if (errorCode != kDNSServiceErr_NoError)
    {
        std::cerr << "MacOSMdnsImpl: Browse error: " << errorCode << std::endl;
        return;
    }

    if (flags & kDNSServiceFlagsAdd)
    {
        // Service added - need to resolve to get details
        std::cout << "MacOSMdnsImpl: Service found: " << serviceName << std::endl;

        DNSServiceRef resolveRef;
        DNSServiceErrorType err = DNSServiceResolve(
            &resolveRef,
            0,                  // flags
            interfaceIndex,     // interface
            serviceName,        // service name
            SERVICE_TYPE,       // service type
            replyDomain,        // domain
            resolveCallback,    // callback
            this               // context
        );

        if (err == kDNSServiceErr_NoError)
        {
            // Process resolve synchronously (blocking)
            DNSServiceProcessResult(resolveRef);
            DNSServiceRefDeallocate(resolveRef);
        }
        else
        {
            std::cerr << "MacOSMdnsImpl: DNSServiceResolve failed: " << err << std::endl;
        }
    }
    else
    {
        // Service removed
        std::cout << "MacOSMdnsImpl: Service removed: " << serviceName << std::endl;

        // We can't extract UUID from serviceName easily, so we'll need to track services
        // For now, we'll skip the removal callback - this would need a service cache
        // to map service names to UUIDs
    }
}

//==============================================================================
// Service Resolution
//==============================================================================
void DNSSD_API MacOSMdnsImpl::resolveCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char* fullname,
    const char* hosttarget,
    uint16_t port,
    uint16_t txtLen,
    const unsigned char* txtRecord,
    void* context)
{
    auto* impl = static_cast<MacOSMdnsImpl*>(context);
    impl->processResolveCallback(errorCode, fullname, hosttarget, port, txtLen, txtRecord);
}

void MacOSMdnsImpl::processResolveCallback(
    DNSServiceErrorType errorCode,
    const char* fullname,
    const char* hosttarget,
    uint16_t port,
    uint16_t txtLen,
    const unsigned char* txtRecord)
{
    if (errorCode != kDNSServiceErr_NoError)
    {
        std::cerr << "MacOSMdnsImpl: Resolve error: " << errorCode << std::endl;
        return;
    }

    // Parse TXT record to get node info
    NodeInfo nodeInfo = parseTxtRecord(txtLen, txtRecord);
    nodeInfo.hostname = juce::String(hosttarget);
    nodeInfo.httpPort = ntohs(port);

    // Skip self-discovery
    if (nodeInfo.uuid == nodeId)
    {
        std::cout << "MacOSMdnsImpl: Skipping self-discovery" << std::endl;
        return;
    }

    // Resolve hostname to IP address
    struct hostent* host = gethostbyname(hosttarget);
    if (host && host->h_addr_list[0])
    {
        struct in_addr addr;
        memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
        nodeInfo.ipAddress = juce::String(inet_ntoa(addr));
    }

    if (nodeInfo.isValid() && onDiscoveredCallback)
    {
        std::cout << "MacOSMdnsImpl: Discovered node: " << nodeInfo.name
                  << " (UUID: " << nodeInfo.uuid.toString() << ")" << std::endl;
        onDiscoveredCallback(nodeInfo);
    }
}

//==============================================================================
// TXT Record Management
//==============================================================================
std::vector<uint8_t> MacOSMdnsImpl::createTxtRecord()
{
    std::vector<uint8_t> txtRecord;

    // Helper to add key-value pair
    auto addTxtPair = [&txtRecord](const std::string& key, const std::string& value) {
        std::string pair = key + "=" + value;
        uint8_t len = static_cast<uint8_t>(pair.size());
        txtRecord.push_back(len);
        txtRecord.insert(txtRecord.end(), pair.begin(), pair.end());
    };

    addTxtPair("uuid", nodeId.toString().toStdString());
    addTxtPair("http_port", std::to_string(httpPort));
    addTxtPair("udp_port", std::to_string(udpPort));
    addTxtPair("hostname", juce::SystemStats::getComputerName().toStdString());
    addTxtPair("version", "1.0");
    addTxtPair("devices", std::to_string(deviceCount));

    return txtRecord;
}

NodeInfo MacOSMdnsImpl::parseTxtRecord(uint16_t txtLen, const unsigned char* txtRecord)
{
    NodeInfo info;

    // Parse TXT record format: [len][key=value][len][key=value]...
    const unsigned char* ptr = txtRecord;
    const unsigned char* end = txtRecord + txtLen;

    while (ptr < end)
    {
        uint8_t len = *ptr++;
        if (ptr + len > end)
            break;

        std::string pair(reinterpret_cast<const char*>(ptr), len);
        ptr += len;

        // Split key=value
        auto pos = pair.find('=');
        if (pos != std::string::npos)
        {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);

            if (key == "uuid")
                info.uuid = juce::Uuid(juce::String(value));
            else if (key == "http_port")
                info.httpPort = std::stoi(value);
            else if (key == "udp_port")
                info.udpPort = std::stoi(value);
            else if (key == "hostname")
                info.hostname = juce::String(value);
            else if (key == "version")
                info.version = juce::String(value);
            else if (key == "devices")
                info.deviceCount = std::stoi(value);
        }
    }

    // Service name is usually the node name
    info.name = nodeName;

    return info;
}

//==============================================================================
// Event Loop Processing
//==============================================================================
void MacOSMdnsImpl::runServiceLoop(DNSServiceRef serviceRef)
{
    int fd = DNSServiceRefSockFD(serviceRef);
    if (fd < 0)
    {
        std::cerr << "MacOSMdnsImpl: Invalid socket FD" << std::endl;
        return;
    }

    fd_set readfds;
    struct timeval tv;

    while ((serviceRef == registerRef && registerRunning) ||
           (serviceRef == browseRef && browseRunning))
    {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int result = select(fd + 1, &readfds, nullptr, nullptr, &tv);

        if (result > 0 && FD_ISSET(fd, &readfds))
        {
            DNSServiceErrorType err = DNSServiceProcessResult(serviceRef);
            if (err != kDNSServiceErr_NoError)
            {
                std::cerr << "MacOSMdnsImpl: DNSServiceProcessResult error: " << err << std::endl;
                break;
            }
        }
        else if (result < 0)
        {
            std::cerr << "MacOSMdnsImpl: select() error" << std::endl;
            break;
        }
    }

    std::cout << "MacOSMdnsImpl: Service loop exited" << std::endl;
}

void MacOSMdnsImpl::updateDeviceCount(int count)
{
    deviceCount = count;

    // To update TXT record, we need to re-register
    // This is a limitation of the DNS-SD API
    if (registerRef)
    {
        stopAdvertising();
        advertise();
    }
}
