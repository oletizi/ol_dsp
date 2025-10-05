#include "ServiceDiscovery.h"
#include <iostream>

//==============================================================================
// Platform-specific implementation forward declarations
#if JUCE_MAC || JUCE_IOS
    #include "platform/mdns_macos.h"
    using PlatformImpl = MacOSMdnsImpl;
#elif JUCE_LINUX
    #include "platform/mdns_linux.h"
    using PlatformImpl = LinuxMdnsImpl;
#elif JUCE_WINDOWS
    #include "platform/mdns_windows.h"
    using PlatformImpl = WindowsMdnsImpl;
#else
    #error "Unsupported platform for ServiceDiscovery"
#endif

//==============================================================================
// Implementation structure
struct ServiceDiscovery::Impl
{
    Impl(const juce::Uuid& nodeId,
         const juce::String& nodeName,
         int httpPort,
         int udpPort,
         int deviceCount)
        : platformImpl(nodeId, nodeName, httpPort, udpPort, deviceCount)
    {
    }

    PlatformImpl platformImpl;
    bool advertising = false;
    bool browsing = false;
};

//==============================================================================
ServiceDiscovery::ServiceDiscovery(const juce::Uuid& nodeId,
                                   const juce::String& nodeName,
                                   int httpPort,
                                   int udpPort,
                                   int deviceCount)
    : impl(std::make_unique<Impl>(nodeId, nodeName, httpPort, udpPort, deviceCount))
    , nodeId(nodeId)
    , nodeName(nodeName)
    , httpPort(httpPort)
    , udpPort(udpPort)
    , deviceCount(deviceCount)
{
}

ServiceDiscovery::~ServiceDiscovery()
{
    stopAdvertising();
    stopBrowsing();
}

ServiceDiscovery::ServiceDiscovery(ServiceDiscovery&&) noexcept = default;
ServiceDiscovery& ServiceDiscovery::operator=(ServiceDiscovery&&) noexcept = default;

//==============================================================================
bool ServiceDiscovery::advertise()
{
    if (impl->advertising)
    {
        std::cerr << "ServiceDiscovery: Already advertising" << std::endl;
        return true;
    }

    bool success = impl->platformImpl.advertise();
    if (success)
    {
        impl->advertising = true;
        std::cout << "ServiceDiscovery: Advertising as '" << nodeName
                  << "' (HTTP:" << httpPort << ", UDP:" << udpPort << ")" << std::endl;
    }
    else
    {
        std::cerr << "ServiceDiscovery: Failed to start advertising" << std::endl;
    }

    return success;
}

void ServiceDiscovery::stopAdvertising()
{
    if (!impl->advertising)
        return;

    impl->platformImpl.stopAdvertising();
    impl->advertising = false;
    std::cout << "ServiceDiscovery: Stopped advertising" << std::endl;
}

bool ServiceDiscovery::isAdvertising() const
{
    return impl->advertising;
}

//==============================================================================
bool ServiceDiscovery::startBrowsing(ServiceDiscoveredCallback onDiscovered,
                                     ServiceRemovedCallback onRemoved)
{
    if (impl->browsing)
    {
        std::cerr << "ServiceDiscovery: Already browsing" << std::endl;
        return true;
    }

    bool success = impl->platformImpl.startBrowsing(onDiscovered, onRemoved);
    if (success)
    {
        impl->browsing = true;
        std::cout << "ServiceDiscovery: Started browsing for services" << std::endl;
    }
    else
    {
        std::cerr << "ServiceDiscovery: Failed to start browsing" << std::endl;
    }

    return success;
}

void ServiceDiscovery::stopBrowsing()
{
    if (!impl->browsing)
        return;

    impl->platformImpl.stopBrowsing();
    impl->browsing = false;
    std::cout << "ServiceDiscovery: Stopped browsing" << std::endl;
}

bool ServiceDiscovery::isBrowsing() const
{
    return impl->browsing;
}

//==============================================================================
void ServiceDiscovery::updateDeviceCount(int count)
{
    deviceCount = count;
    if (impl->advertising)
    {
        impl->platformImpl.updateDeviceCount(count);
    }
}
