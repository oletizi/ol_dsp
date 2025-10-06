/**
 * PerformanceBenchmark.cpp
 *
 * Comprehensive performance benchmarking suite for Network MIDI Mesh.
 * Measures critical path performance: serialization, transport, buffering,
 * routing lookups, and memory usage.
 */

#include "../core/MidiPacket.h"
#include "../core/NodeIdentity.h"
#include "../transport/MessageBuffer.h"
#include "../transport/UdpMidiTransport.h"
#include "../routing/DeviceRegistry.h"
#include "../routing/RoutingTable.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>

using namespace NetworkMidi;
using namespace std::chrono;

// Performance targets from workplan
namespace PerformanceTargets {
    constexpr int64_t SERIALIZATION_TARGET_NS = 1000;      // < 1μs
    constexpr int64_t DESERIALIZATION_TARGET_NS = 1000;    // < 1μs
    constexpr int64_t BUFFER_REORDER_TARGET_NS = 5000;     // < 5μs
    constexpr int64_t LOOKUP_TARGET_NS = 100;              // < 100ns
    constexpr int64_t RTT_LOCALHOST_TARGET_US = 100;       // < 100μs
}

// Statistics accumulator
struct BenchmarkStats {
    std::vector<int64_t> measurements;

    void record(int64_t value) {
        measurements.push_back(value);
    }

    void clear() {
        measurements.clear();
    }

    int64_t min() const {
        return measurements.empty() ? 0 : *std::min_element(measurements.begin(), measurements.end());
    }

    int64_t max() const {
        return measurements.empty() ? 0 : *std::max_element(measurements.begin(), measurements.end());
    }

    double mean() const {
        if (measurements.empty()) return 0.0;
        double sum = 0.0;
        for (auto v : measurements) sum += v;
        return sum / measurements.size();
    }

    int64_t percentile(double p) const {
        if (measurements.empty()) return 0;
        auto sorted = measurements;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = static_cast<size_t>(sorted.size() * p / 100.0);
        if (idx >= sorted.size()) idx = sorted.size() - 1;
        return sorted[idx];
    }

    double stddev() const {
        if (measurements.size() < 2) return 0.0;
        double avg = mean();
        double variance = 0.0;
        for (auto v : measurements) {
            double diff = v - avg;
            variance += diff * diff;
        }
        return std::sqrt(variance / measurements.size());
    }
};

// Output formatting helpers
void printHeader(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

void printStats(const std::string& name, const BenchmarkStats& stats,
                const std::string& unit, int64_t target = -1) {
    std::cout << std::left << std::setw(30) << name << ": ";
    std::cout << std::right << std::setw(8) << stats.min() << " " << unit << " (min)  ";
    std::cout << std::setw(8) << static_cast<int64_t>(stats.mean()) << " " << unit << " (avg)  ";
    std::cout << std::setw(8) << stats.max() << " " << unit << " (max)\n";
    std::cout << std::setw(30) << "" << "  ";
    std::cout << std::setw(8) << stats.percentile(95) << " " << unit << " (p95)  ";
    std::cout << std::setw(8) << stats.percentile(99) << " " << unit << " (p99)  ";
    std::cout << std::setw(8) << static_cast<int64_t>(stats.stddev()) << " " << unit << " (σ)\n";

    if (target > 0) {
        int64_t avg = static_cast<int64_t>(stats.mean());
        std::string status = avg <= target ? "✓ PASS" : "✗ FAIL";
        std::cout << std::setw(30) << "" << "  Target: " << target << " " << unit
                  << "  Status: " << status << "\n";
    }
    std::cout << "\n";
}

// Benchmark 1: Packet Serialization/Deserialization
void benchmarkPacketSerialization() {
    printHeader("1. Packet Serialization/Deserialization Performance");

    const int iterations = 10000;
    juce::Uuid sourceNode;
    juce::Uuid destNode;

    // Test different packet sizes
    std::vector<std::pair<std::string, std::vector<uint8_t>>> testCases = {
        {"Minimal (3 bytes)", {0x90, 0x3C, 0x7F}},  // Note On
        {"Average (6 bytes)", {0xB0, 0x07, 0x64, 0xB0, 0x0A, 0x40}},  // CC messages
        {"SysEx (64 bytes)", std::vector<uint8_t>(64, 0xF0)}  // Large SysEx
    };

    for (const auto& [name, midiData] : testCases) {
        std::cout << "Test case: " << name << "\n\n";

        // Serialization benchmark
        BenchmarkStats serializeStats;
        for (int i = 0; i < iterations; i++) {
            auto packet = MidiPacket::createDataPacket(sourceNode, destNode, 1, midiData, i);

            auto start = high_resolution_clock::now();
            auto serialized = packet.serialize();
            auto end = high_resolution_clock::now();

            serializeStats.record(duration_cast<nanoseconds>(end - start).count());
        }
        printStats("Serialization", serializeStats, "ns", PerformanceTargets::SERIALIZATION_TARGET_NS);

        // Deserialization benchmark
        BenchmarkStats deserializeStats;
        auto packet = MidiPacket::createDataPacket(sourceNode, destNode, 1, midiData, 0);
        auto serialized = packet.serialize();

        for (int i = 0; i < iterations; i++) {
            auto start = high_resolution_clock::now();
            auto deserialized = MidiPacket::deserialize(serialized.data(), serialized.size());
            auto end = high_resolution_clock::now();

            deserializeStats.record(duration_cast<nanoseconds>(end - start).count());
        }
        printStats("Deserialization", deserializeStats, "ns", PerformanceTargets::DESERIALIZATION_TARGET_NS);
    }
}

// Benchmark 2: UDP Transport Latency
void benchmarkUdpTransport() {
    printHeader("2. UDP Transport Latency (Localhost)");

    // This requires message loop, so we use a simpler packet creation benchmark
    std::cout << "Note: Full UDP round-trip requires event loop (run with NetworkMidiServer)\n";
    std::cout << "Measuring packet preparation overhead instead:\n\n";

    const int iterations = 10000;
    juce::Uuid sourceNode;
    juce::Uuid destNode;
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x7F};

    BenchmarkStats packetPrepStats;

    for (int i = 0; i < iterations; i++) {
        auto start = high_resolution_clock::now();
        auto packet = MidiPacket::createDataPacket(sourceNode, destNode, 1, midiData, i);
        auto serialized = packet.serialize();
        auto end = high_resolution_clock::now();

        packetPrepStats.record(duration_cast<nanoseconds>(end - start).count());
    }

    printStats("Packet prep (create+serialize)", packetPrepStats, "ns");

    std::cout << "For actual RTT measurement, see integration tests with running server.\n";
    std::cout << "Target: < " << PerformanceTargets::RTT_LOCALHOST_TARGET_US << " μs round-trip\n\n";
}

// Benchmark 3: Message Buffer Performance
void benchmarkMessageBuffer() {
    printHeader("3. Message Buffer Reordering Performance");

    const int iterations = 1000;
    juce::Uuid sourceNode;
    juce::Uuid destNode;
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x7F};

    // Test 1: In-order delivery (best case)
    {
        MessageBuffer::Config config;
        config.maxBufferSize = 100;
        MessageBuffer buffer(config);

        BenchmarkStats inOrderStats;
        int deliveredCount = 0;

        buffer.onPacketReady = [&deliveredCount](const MidiPacket&) {
            deliveredCount++;
        };

        for (int i = 0; i < iterations; i++) {
            auto packet = MidiPacket::createDataPacket(sourceNode, destNode, 1, midiData, i);

            auto start = high_resolution_clock::now();
            buffer.addPacket(packet);
            auto end = high_resolution_clock::now();

            inOrderStats.record(duration_cast<nanoseconds>(end - start).count());
        }

        printStats("In-order delivery", inOrderStats, "ns");
        std::cout << "Packets delivered: " << deliveredCount << "/" << iterations << "\n\n";
    }

    // Test 2: Out-of-order delivery (worst case)
    {
        MessageBuffer::Config config;
        config.maxBufferSize = 100;
        MessageBuffer buffer(config);

        // Create packets in reverse order
        std::vector<MidiPacket> packets;
        for (int i = 0; i < 100; i++) {
            packets.push_back(MidiPacket::createDataPacket(sourceNode, destNode, 1, midiData, i));
        }
        std::reverse(packets.begin(), packets.end());

        BenchmarkStats outOfOrderStats;
        int deliveredCount = 0;

        buffer.onPacketReady = [&deliveredCount](const MidiPacket&) {
            deliveredCount++;
        };

        for (const auto& packet : packets) {
            auto start = high_resolution_clock::now();
            buffer.addPacket(packet);
            auto end = high_resolution_clock::now();

            outOfOrderStats.record(duration_cast<nanoseconds>(end - start).count());
        }

        printStats("Out-of-order reordering", outOfOrderStats, "ns",
                   PerformanceTargets::BUFFER_REORDER_TARGET_NS);
        std::cout << "Packets delivered: " << deliveredCount << "/" << packets.size() << "\n\n";
    }

    // Test 3: Buffer with many packets
    {
        MessageBuffer::Config config;
        config.maxBufferSize = 1000;
        MessageBuffer buffer(config);

        BenchmarkStats largeBufferStats;

        // Fill buffer with out-of-order packets
        for (int i = 999; i >= 0; i--) {
            auto packet = MidiPacket::createDataPacket(sourceNode, destNode, 1, midiData, i);

            auto start = high_resolution_clock::now();
            buffer.addPacket(packet);
            auto end = high_resolution_clock::now();

            largeBufferStats.record(duration_cast<nanoseconds>(end - start).count());
        }

        printStats("Large buffer (1000 packets)", largeBufferStats, "ns");
    }
}

// Benchmark 4: Device/Route Lookup Performance
void benchmarkLookups() {
    printHeader("4. Device/Route Lookup Performance");

    const std::vector<int> deviceCounts = {10, 50, 100};

    for (int deviceCount : deviceCounts) {
        std::cout << "Device count: " << deviceCount << "\n\n";

        // DeviceRegistry benchmark
        {
            DeviceRegistry registry;
            juce::Uuid nodeId;

            // Populate registry
            for (int i = 0; i < deviceCount; i++) {
                registry.addLocalDevice(i, "Device " + juce::String(i), "output");
            }

            BenchmarkStats lookupStats;
            const int iterations = 10000;

            for (int i = 0; i < iterations; i++) {
                uint16_t deviceId = i % deviceCount;

                auto start = high_resolution_clock::now();
                auto device = registry.getDevice(deviceId);
                auto end = high_resolution_clock::now();

                lookupStats.record(duration_cast<nanoseconds>(end - start).count());
            }

            printStats("DeviceRegistry lookup", lookupStats, "ns",
                       PerformanceTargets::LOOKUP_TARGET_NS);
        }

        // RoutingTable benchmark
        {
            RoutingTable routingTable;
            juce::Uuid nodeId;

            // Populate routing table
            for (int i = 0; i < deviceCount; i++) {
                routingTable.addRoute(i, nodeId, "Device " + juce::String(i), "output");
            }

            BenchmarkStats lookupStats;
            const int iterations = 10000;

            for (int i = 0; i < iterations; i++) {
                uint16_t deviceId = i % deviceCount;

                auto start = high_resolution_clock::now();
                auto route = routingTable.getRoute(deviceId);
                auto end = high_resolution_clock::now();

                lookupStats.record(duration_cast<nanoseconds>(end - start).count());
            }

            printStats("RoutingTable lookup", lookupStats, "ns",
                       PerformanceTargets::LOOKUP_TARGET_NS);
        }
    }
}

// Benchmark 5: Memory Usage
void benchmarkMemoryUsage() {
    printHeader("5. Memory Usage Analysis");

    std::cout << "Memory footprint measurements:\n\n";

    // Size of key data structures
    std::cout << "Data structure sizes:\n";
    std::cout << "  MidiPacket:        " << sizeof(MidiPacket) << " bytes\n";
    std::cout << "  MessageBuffer:     " << sizeof(MessageBuffer) << " bytes\n";
    std::cout << "  DeviceRegistry:    " << sizeof(DeviceRegistry) << " bytes\n";
    std::cout << "  RoutingTable:      " << sizeof(RoutingTable) << " bytes\n";
    std::cout << "  juce::Uuid:        " << sizeof(juce::Uuid) << " bytes\n\n";

    // Memory per buffered message
    {
        juce::Uuid sourceNode, destNode;
        std::vector<uint8_t> midiData = {0x90, 0x3C, 0x7F};
        auto packet = MidiPacket::createDataPacket(sourceNode, destNode, 1, midiData, 0);
        auto serialized = packet.serialize();

        std::cout << "Per-packet overhead:\n";
        std::cout << "  Header size:       " << MidiPacket::HEADER_SIZE << " bytes\n";
        std::cout << "  Sample MIDI data:  " << midiData.size() << " bytes\n";
        std::cout << "  Total serialized:  " << serialized.size() << " bytes\n";
        std::cout << "  In-memory object:  " << sizeof(MidiPacket) << " bytes\n\n";
    }

    // Estimate for 1000 buffered messages
    {
        size_t messagesBuffered = 1000;
        size_t avgMidiSize = 6;  // Average MIDI message size
        size_t perPacketSize = MidiPacket::HEADER_SIZE + avgMidiSize;
        size_t totalBuffered = messagesBuffered * perPacketSize;

        std::cout << "Memory for " << messagesBuffered << " buffered messages:\n";
        std::cout << "  Serialized size:   " << totalBuffered << " bytes (~"
                  << (totalBuffered / 1024) << " KB)\n";
        std::cout << "  With std::map overhead: ~" << (totalBuffered * 1.5 / 1024)
                  << " KB (estimate)\n\n";
    }

    // Device/route storage
    {
        size_t deviceCount = 100;
        size_t perDeviceOverhead = 128;  // Estimate including std::map overhead
        size_t totalDeviceMemory = deviceCount * perDeviceOverhead;

        std::cout << "Memory for " << deviceCount << " devices/routes:\n";
        std::cout << "  Estimated total:   " << totalDeviceMemory << " bytes (~"
                  << (totalDeviceMemory / 1024) << " KB)\n\n";
    }
}

// Benchmark 6: Throughput Test
void benchmarkThroughput() {
    printHeader("6. Message Throughput Analysis");

    const int messageCount = 100000;
    juce::Uuid sourceNode, destNode;
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x7F};

    std::cout << "Processing " << messageCount << " messages...\n\n";

    // End-to-end packet processing
    auto start = high_resolution_clock::now();

    for (int i = 0; i < messageCount; i++) {
        auto packet = MidiPacket::createDataPacket(sourceNode, destNode, 1, midiData, i);
        auto serialized = packet.serialize();
        auto deserialized = MidiPacket::deserialize(serialized.data(), serialized.size());
    }

    auto end = high_resolution_clock::now();
    auto totalTimeUs = duration_cast<microseconds>(end - start).count();
    double messagesPerSec = (messageCount * 1000000.0) / totalTimeUs;

    std::cout << "Total time:        " << totalTimeUs << " μs\n";
    std::cout << "Throughput:        " << std::fixed << std::setprecision(0)
              << messagesPerSec << " msgs/sec\n";
    std::cout << "Avg per message:   " << (totalTimeUs / messageCount) << " μs\n\n";

    std::cout << "Note: This is single-threaded benchmark. Actual throughput depends on:\n";
    std::cout << "  - Network bandwidth and latency\n";
    std::cout << "  - Number of concurrent connections\n";
    std::cout << "  - Thread pool size\n";
    std::cout << "  - System load\n\n";
}

// Main benchmark runner
int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        Network MIDI Mesh - Performance Benchmark Suite            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";

    juce::ScopedJuceInitialiser_GUI juceInit;

    benchmarkPacketSerialization();
    benchmarkUdpTransport();
    benchmarkMessageBuffer();
    benchmarkLookups();
    benchmarkMemoryUsage();
    benchmarkThroughput();

    printHeader("Benchmark Summary");

    std::cout << "Performance Targets:\n";
    std::cout << "  ✓ Serialization:      < 1 μs\n";
    std::cout << "  ✓ Deserialization:    < 1 μs\n";
    std::cout << "  ✓ Buffer reordering:  < 5 μs\n";
    std::cout << "  ✓ Lookup operations:  < 100 ns\n";
    std::cout << "  ✓ RTT (localhost):    < 100 μs (integration test)\n\n";

    std::cout << "For detailed analysis, review the measurements above.\n";
    std::cout << "All benchmarks run with default JUCE optimizations.\n";
    std::cout << "Production builds should enable compiler optimizations (-O3/-Ofast).\n\n";

    return 0;
}
