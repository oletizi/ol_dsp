#pragma once

#include "../ServiceDiscovery.h"
#include <dns_sd.h>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

/**
 * @brief macOS-specific mDNS implementation using Bonjour (DNSServiceDiscovery API)
 *
 * This class implements service discovery using the native macOS Bonjour/mDNS
 * framework. It uses DNSServiceRegister for advertising and DNSServiceBrowse
 * for discovering other nodes.
 *
 * Thread Safety: All public methods are thread-safe
 */
class MacOSMdnsImpl
{
public:
    MacOSMdnsImpl(const juce::Uuid& nodeId,
                  const juce::String& nodeName,
                  int httpPort,
                  int udpPort,
                  int deviceCount);

    ~MacOSMdnsImpl();

    // Non-copyable, non-moveable (due to callbacks with 'this' pointer)
    MacOSMdnsImpl(const MacOSMdnsImpl&) = delete;
    MacOSMdnsImpl& operator=(const MacOSMdnsImpl&) = delete;
    MacOSMdnsImpl(MacOSMdnsImpl&&) = delete;
    MacOSMdnsImpl& operator=(MacOSMdnsImpl&&) = delete;

    bool advertise();
    void stopAdvertising();

    bool startBrowsing(ServiceDiscovery::ServiceDiscoveredCallback onDiscovered,
                       ServiceDiscovery::ServiceRemovedCallback onRemoved);
    void stopBrowsing();

    void updateDeviceCount(int count);

private:
    // Service registration
    static void DNSSD_API registerCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        DNSServiceErrorType errorCode,
        const char* name,
        const char* regtype,
        const char* domain,
        void* context);

    void processRegisterCallback(DNSServiceErrorType errorCode, const char* name);

    // Service browsing
    static void DNSSD_API browseCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* serviceName,
        const char* regtype,
        const char* replyDomain,
        void* context);

    void processBrowseCallback(
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* serviceName,
        const char* replyDomain);

    // Service resolution
    static void DNSSD_API resolveCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* fullname,
        const char* hosttarget,
        uint16_t port,
        uint16_t txtLen,
        const unsigned char* txtRecord,
        void* context);

    void processResolveCallback(
        DNSServiceErrorType errorCode,
        const char* fullname,
        const char* hosttarget,
        uint16_t port,
        uint16_t txtLen,
        const unsigned char* txtRecord);

    // TXT record management
    std::vector<uint8_t> createTxtRecord();
    NodeInfo parseTxtRecord(uint16_t txtLen, const unsigned char* txtRecord);

    // Event loop processing
    void runServiceLoop(DNSServiceRef serviceRef);
    void stopServiceLoop();

    // Node information
    juce::Uuid nodeId;
    juce::String nodeName;
    int httpPort;
    int udpPort;
    int deviceCount;

    // Service references
    DNSServiceRef registerRef = nullptr;
    DNSServiceRef browseRef = nullptr;
    std::unique_ptr<std::thread> registerThread;
    std::unique_ptr<std::thread> browseThread;
    std::atomic<bool> registerRunning{false};
    std::atomic<bool> browseRunning{false};

    // Callbacks
    ServiceDiscovery::ServiceDiscoveredCallback onDiscoveredCallback;
    ServiceDiscovery::ServiceRemovedCallback onRemovedCallback;

    // Thread synchronization
    mutable std::mutex mutex;

    // Service type constant
    static constexpr const char* SERVICE_TYPE = "_midi-network._tcp";
};
