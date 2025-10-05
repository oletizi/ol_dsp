/**
 * @brief Test program for service discovery
 *
 * This program tests both mDNS (Bonjour) and fallback UDP multicast discovery.
 * It can be run in different modes to test various scenarios.
 */

#include "ServiceDiscovery.h"
#include "FallbackDiscovery.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <iostream>
#include <thread>
#include <chrono>

//==============================================================================
void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --mode <mdns|fallback|both>  Discovery mode (default: both)\n"
              << "  --name <name>                Node name (default: test-node-XXXX)\n"
              << "  --http-port <port>           HTTP port (default: 8080)\n"
              << "  --udp-port <port>            UDP port (default: 9090)\n"
              << "  --devices <count>            Device count (default: 3)\n"
              << "  --help                       Show this help\n\n"
              << "Examples:\n"
              << "  " << programName << " --mode mdns --name node1 --http-port 8080\n"
              << "  " << programName << " --mode fallback\n"
              << "  " << programName << " --mode both --devices 5\n"
              << std::endl;
}

//==============================================================================
int main(int argc, char* argv[])
{
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Parse command line arguments
    std::string mode = "both";
    std::string nodeName;
    int httpPort = 8080;
    int udpPort = 9090;
    int deviceCount = 3;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--mode" && i + 1 < argc)
        {
            mode = argv[++i];
            if (mode != "mdns" && mode != "fallback" && mode != "both")
            {
                std::cerr << "Error: Invalid mode. Use mdns, fallback, or both." << std::endl;
                return 1;
            }
        }
        else if (arg == "--name" && i + 1 < argc)
        {
            nodeName = argv[++i];
        }
        else if (arg == "--http-port" && i + 1 < argc)
        {
            httpPort = std::atoi(argv[++i]);
        }
        else if (arg == "--udp-port" && i + 1 < argc)
        {
            udpPort = std::atoi(argv[++i]);
        }
        else if (arg == "--devices" && i + 1 < argc)
        {
            deviceCount = std::atoi(argv[++i]);
        }
    }

    // Generate node ID and name
    juce::Uuid nodeId = juce::Uuid();
    if (nodeName.empty())
    {
        nodeName = "test-node-" + nodeId.toString().substring(0, 8).toStdString();
    }

    // Print configuration
    std::cout << "\n=== Network MIDI Discovery Test ===" << std::endl;
    std::cout << "Mode:         " << mode << std::endl;
    std::cout << "Node ID:      " << nodeId.toString() << std::endl;
    std::cout << "Node Name:    " << nodeName << std::endl;
    std::cout << "HTTP Port:    " << httpPort << std::endl;
    std::cout << "UDP Port:     " << udpPort << std::endl;
    std::cout << "Device Count: " << deviceCount << std::endl;
    std::cout << "===================================\n" << std::endl;

    // Discovery callbacks
    auto onDiscovered = [](const NodeInfo& node) {
        std::cout << "\n[DISCOVERED] Node: " << node.name << std::endl;
        std::cout << "  UUID:      " << node.uuid.toString() << std::endl;
        std::cout << "  IP:        " << node.ipAddress << std::endl;
        std::cout << "  HTTP Port: " << node.httpPort << std::endl;
        std::cout << "  UDP Port:  " << node.udpPort << std::endl;
        std::cout << "  Devices:   " << node.deviceCount << std::endl;
        std::cout << "  Version:   " << node.version << std::endl;
    };

    auto onRemoved = [](const juce::Uuid& uuid) {
        std::cout << "\n[REMOVED] Node UUID: " << uuid.toString() << std::endl;
    };

    // Create discovery objects
    std::unique_ptr<ServiceDiscovery> mdnsDiscovery;
    std::unique_ptr<FallbackDiscovery> fallbackDiscovery;

    if (mode == "mdns" || mode == "both")
    {
        std::cout << "Starting mDNS discovery..." << std::endl;
        mdnsDiscovery = std::make_unique<ServiceDiscovery>(
            nodeId,
            juce::String(nodeName),
            httpPort,
            udpPort,
            deviceCount
        );

        if (mdnsDiscovery->advertise())
        {
            std::cout << "✓ mDNS advertising started" << std::endl;
        }
        else
        {
            std::cout << "✗ mDNS advertising failed" << std::endl;
        }

        if (mdnsDiscovery->startBrowsing(onDiscovered, onRemoved))
        {
            std::cout << "✓ mDNS browsing started" << std::endl;
        }
        else
        {
            std::cout << "✗ mDNS browsing failed" << std::endl;
        }
    }

    if (mode == "fallback" || mode == "both")
    {
        std::cout << "\nStarting fallback discovery..." << std::endl;
        fallbackDiscovery = std::make_unique<FallbackDiscovery>(
            nodeId,
            juce::String(nodeName),
            httpPort,
            udpPort,
            deviceCount
        );

        if (fallbackDiscovery->startBroadcasting())
        {
            std::cout << "✓ Fallback broadcasting started" << std::endl;
        }
        else
        {
            std::cout << "✗ Fallback broadcasting failed" << std::endl;
        }

        if (fallbackDiscovery->startListening(onDiscovered, onRemoved))
        {
            std::cout << "✓ Fallback listening started" << std::endl;
        }
        else
        {
            std::cout << "✗ Fallback listening failed" << std::endl;
        }
    }

    // Run until interrupted
    std::cout << "\nDiscovery active. Press Ctrl+C to stop...\n" << std::endl;

    // Simulate device count changes every 30 seconds
    int counter = 0;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(30));

        ++counter;
        int newDeviceCount = deviceCount + counter;

        std::cout << "\n[UPDATE] Changing device count to " << newDeviceCount << std::endl;

        if (mdnsDiscovery)
        {
            mdnsDiscovery->updateDeviceCount(newDeviceCount);
        }
        if (fallbackDiscovery)
        {
            fallbackDiscovery->updateDeviceCount(newDeviceCount);
        }
    }

    return 0;
}
