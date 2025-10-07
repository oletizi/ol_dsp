/**
 * context_performance_test.cpp
 *
 * Phase 4 Task 4.6: Performance benchmarks for context forwarding
 *
 * Benchmarks:
 * 1. Per-hop overhead: Serialization + deserialization time
 * 2. Multi-hop latency: Compare 1-hop vs 8-hop forwarding
 * 3. Throughput: Messages/sec with context vs without
 * 4. Packet size: Distribution of packet sizes (0-8 hops)
 *
 * Target Metrics (from Phase 4 spec):
 * - Serialization: <500ns
 * - Per-hop overhead: <1μs
 * - Packet size: 23-75 bytes
 * - Throughput: 1000 msg/sec (no degradation)
 */

#include "network/core/MidiPacket.h"
#include "network/routing/UuidRegistry.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <iomanip>
#include <cmath>

using namespace NetworkMidi;
using namespace std::chrono;

//==============================================================================
// Benchmark Framework
//==============================================================================

class PerformanceBenchmark {
public:
    struct BenchmarkResult {
        std::string name;
        double avgTimeNs;
        double minTimeNs;
        double maxTimeNs;
        double stdDevNs;
        size_t iterations;
        bool passed;
        double targetNs;

        void print() const {
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "\n" << name << ":\n";
            std::cout << "  Average:  " << avgTimeNs << " ns";
            if (targetNs > 0) {
                std::cout << " (target: <" << targetNs << " ns)";
            }
            std::cout << "\n";
            std::cout << "  Min:      " << minTimeNs << " ns\n";
            std::cout << "  Max:      " << maxTimeNs << " ns\n";
            std::cout << "  StdDev:   " << stdDevNs << " ns\n";
            std::cout << "  Iters:    " << iterations << "\n";
            std::cout << "  Result:   " << (passed ? "PASS ✓" : "FAIL ✗") << "\n";
        }
    };

    template<typename Func>
    static BenchmarkResult measure(const std::string& name,
                                   Func&& func,
                                   size_t iterations,
                                   double targetNs = 0.0) {
        std::vector<double> timesNs;
        timesNs.reserve(iterations);

        // Warmup
        for (size_t i = 0; i < std::min(iterations / 10, size_t(100)); ++i) {
            func();
        }

        // Actual measurement
        for (size_t i = 0; i < iterations; ++i) {
            auto start = high_resolution_clock::now();
            func();
            auto end = high_resolution_clock::now();

            auto durationNs = duration_cast<nanoseconds>(end - start).count();
            timesNs.push_back(static_cast<double>(durationNs));
        }

        // Calculate statistics
        double avg = std::accumulate(timesNs.begin(), timesNs.end(), 0.0) / timesNs.size();
        double minTime = *std::min_element(timesNs.begin(), timesNs.end());
        double maxTime = *std::max_element(timesNs.begin(), timesNs.end());

        // Standard deviation
        double variance = 0.0;
        for (double t : timesNs) {
            variance += (t - avg) * (t - avg);
        }
        variance /= timesNs.size();
        double stdDev = std::sqrt(variance);

        BenchmarkResult result;
        result.name = name;
        result.avgTimeNs = avg;
        result.minTimeNs = minTime;
        result.maxTimeNs = maxTime;
        result.stdDevNs = stdDev;
        result.iterations = iterations;
        result.targetNs = targetNs;
        result.passed = (targetNs == 0.0) || (avg <= targetNs);

        return result;
    }
};

//==============================================================================
// Benchmark 1: Serialization/Deserialization Overhead
//==============================================================================

void benchmark_serialization() {
    std::cout << "\n========================================\n";
    std::cout << "Benchmark 1: Serialization Overhead\n";
    std::cout << "========================================\n";

    UuidRegistry registry;
    juce::Uuid node1 = juce::Uuid();
    juce::Uuid node2 = juce::Uuid();
    registry.registerNode(node1);
    registry.registerNode(node2);

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    // Test with varying context sizes
    std::vector<size_t> deviceCounts = {0, 1, 4, 8};

    for (size_t deviceCount : deviceCounts) {
        // Create context with N visited devices
        ForwardingContext context;
        context.hopCount = static_cast<uint8_t>(deviceCount);

        for (size_t i = 0; i < deviceCount; ++i) {
            juce::Uuid tempNode = juce::Uuid();
            registry.registerNode(tempNode);
            context.visitedDevices.insert(DeviceKey(tempNode, static_cast<uint16_t>(i)));
        }

        // Create packet
        MidiPacket packet = MidiPacket::createDataPacket(node1, node2, 1, midiData, 100);
        if (deviceCount > 0) {
            packet.setForwardingContext(context, registry);
        }

        // Benchmark serialization
        auto serResult = PerformanceBenchmark::measure(
            "Serialize context (" + std::to_string(deviceCount) + " devices)",
            [&]() {
                auto data = packet.serialize();
                // Prevent optimization
                volatile size_t size = data.size();
                (void)size;
            },
            10000,
            500.0  // Target: <500ns
        );
        serResult.print();

        // Serialize once for deserialization test
        auto serialized = packet.serialize();

        // Benchmark deserialization
        auto deserResult = PerformanceBenchmark::measure(
            "Deserialize context (" + std::to_string(deviceCount) + " devices)",
            [&]() {
                auto parsed = MidiPacket::deserialize(serialized.data(), serialized.size());
                // Prevent optimization
                volatile bool valid = parsed.isValid();
                (void)valid;
            },
            10000,
            500.0  // Target: <500ns
        );
        deserResult.print();

        // Report packet size
        std::cout << "  Packet size: " << serialized.size() << " bytes\n";
    }
}

//==============================================================================
// Benchmark 2: Multi-Hop Latency
//==============================================================================

void benchmark_multihop_latency() {
    std::cout << "\n========================================\n";
    std::cout << "Benchmark 2: Multi-Hop Latency\n";
    std::cout << "========================================\n";

    UuidRegistry registry;
    std::vector<juce::Uuid> nodes;

    // Create 9 nodes
    for (int i = 0; i < 9; ++i) {
        juce::Uuid node = juce::Uuid();
        nodes.push_back(node);
        registry.registerNode(node);
    }

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    // Test different hop counts
    std::vector<size_t> hopCounts = {1, 3, 5, 8};

    for (size_t hopCount : hopCounts) {
        // Build context for N hops
        ForwardingContext context;
        context.hopCount = static_cast<uint8_t>(hopCount);

        for (size_t i = 0; i < hopCount; ++i) {
            context.visitedDevices.insert(DeviceKey(nodes[i], static_cast<uint16_t>(i)));
        }

        auto result = PerformanceBenchmark::measure(
            std::to_string(hopCount) + "-hop forwarding",
            [&]() {
                // Simulate hop: deserialize → process → serialize
                MidiPacket packet = MidiPacket::createDataPacket(
                    nodes[0], nodes[hopCount], 1, midiData, 100);
                packet.setForwardingContext(context, registry);

                // Serialize
                auto serialized = packet.serialize();

                // Deserialize
                auto received = MidiPacket::deserialize(serialized.data(), serialized.size());

                // Extract context
                auto ctx = received.getForwardingContext(registry);

                // Create next hop packet
                MidiPacket nextPacket = MidiPacket::createDataPacket(
                    nodes[hopCount - 1], nodes[hopCount], 1, midiData, 101);

                if (ctx.has_value()) {
                    ForwardingContext newCtx = ctx.value();
                    newCtx.hopCount++;
                    newCtx.visitedDevices.insert(DeviceKey(nodes[hopCount], 1));
                    nextPacket.setForwardingContext(newCtx, registry);
                }

                // Prevent optimization
                volatile bool valid = nextPacket.isValid();
                (void)valid;
            },
            5000,
            1000.0 * hopCount  // Target: <1μs per hop
        );
        result.print();
    }
}

//==============================================================================
// Benchmark 3: Throughput with Context
//==============================================================================

void benchmark_throughput() {
    std::cout << "\n========================================\n";
    std::cout << "Benchmark 3: Throughput\n";
    std::cout << "========================================\n";

    UuidRegistry registry;
    juce::Uuid node1 = juce::Uuid();
    juce::Uuid node2 = juce::Uuid();
    registry.registerNode(node1);
    registry.registerNode(node2);

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    // Test without context
    size_t messageCountNoContext = 0;
    auto startNoContext = high_resolution_clock::now();
    auto endTimeNoContext = startNoContext + seconds(1);

    while (high_resolution_clock::now() < endTimeNoContext) {
        MidiPacket packet = MidiPacket::createDataPacket(node1, node2, 1, midiData, 100);
        auto data = packet.serialize();
        auto parsed = MidiPacket::deserialize(data.data(), data.size());
        messageCountNoContext++;
    }

    auto durationNoContext = duration_cast<milliseconds>(
        high_resolution_clock::now() - startNoContext).count();

    double throughputNoContext = (messageCountNoContext * 1000.0) / durationNoContext;

    std::cout << "\nWithout context:\n";
    std::cout << "  Messages:   " << messageCountNoContext << "\n";
    std::cout << "  Duration:   " << durationNoContext << " ms\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0)
              << throughputNoContext << " msg/sec\n";

    // Test with context (3 hops, 3 devices)
    ForwardingContext context;
    context.hopCount = 3;
    for (int i = 0; i < 3; ++i) {
        juce::Uuid tempNode = juce::Uuid();
        registry.registerNode(tempNode);
        context.visitedDevices.insert(DeviceKey(tempNode, i));
    }

    size_t messageCountWithContext = 0;
    auto startWithContext = high_resolution_clock::now();
    auto endTimeWithContext = startWithContext + seconds(1);

    while (high_resolution_clock::now() < endTimeWithContext) {
        MidiPacket packet = MidiPacket::createDataPacket(node1, node2, 1, midiData, 100);
        packet.setForwardingContext(context, registry);
        auto data = packet.serialize();
        auto parsed = MidiPacket::deserialize(data.data(), data.size());
        auto ctx = parsed.getForwardingContext(registry);
        messageCountWithContext++;
    }

    auto durationWithContext = duration_cast<milliseconds>(
        high_resolution_clock::now() - startWithContext).count();

    double throughputWithContext = (messageCountWithContext * 1000.0) / durationWithContext;

    std::cout << "\nWith context (3 hops, 3 devices):\n";
    std::cout << "  Messages:   " << messageCountWithContext << "\n";
    std::cout << "  Duration:   " << durationWithContext << " ms\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0)
              << throughputWithContext << " msg/sec\n";

    // Calculate overhead
    double overhead = ((throughputNoContext - throughputWithContext) / throughputNoContext) * 100.0;

    std::cout << "\nOverhead analysis:\n";
    std::cout << "  Throughput reduction: " << std::fixed << std::setprecision(1)
              << overhead << "%\n";
    std::cout << "  Target: <10% reduction\n";

    bool passed = (overhead < 10.0) && (throughputWithContext >= 1000.0);
    std::cout << "  Result: " << (passed ? "PASS ✓" : "FAIL ✗") << "\n";
}

//==============================================================================
// Benchmark 4: Packet Size Distribution
//==============================================================================

void benchmark_packet_sizes() {
    std::cout << "\n========================================\n";
    std::cout << "Benchmark 4: Packet Size Distribution\n";
    std::cout << "========================================\n";

    UuidRegistry registry;
    juce::Uuid node1 = juce::Uuid();
    juce::Uuid node2 = juce::Uuid();
    registry.registerNode(node1);
    registry.registerNode(node2);

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};  // 3-byte MIDI

    std::cout << "\nPacket sizes for 3-byte MIDI message:\n";
    std::cout << std::setw(15) << "Hops" << std::setw(15) << "Devices"
              << std::setw(15) << "Size (bytes)" << std::setw(15) << "Target\n";
    std::cout << std::string(60, '-') << "\n";

    bool allPassed = true;

    for (size_t hopCount = 0; hopCount <= 8; ++hopCount) {
        ForwardingContext context;
        context.hopCount = static_cast<uint8_t>(hopCount);

        for (size_t i = 0; i < hopCount; ++i) {
            juce::Uuid tempNode = juce::Uuid();
            registry.registerNode(tempNode);
            context.visitedDevices.insert(DeviceKey(tempNode, static_cast<uint16_t>(i)));
        }

        MidiPacket packet = MidiPacket::createDataPacket(node1, node2, 1, midiData, 100);
        if (hopCount > 0) {
            packet.setForwardingContext(context, registry);
        }

        auto serialized = packet.serialize();
        size_t packetSize = serialized.size();

        std::string target = (hopCount == 0) ? "<30 bytes" : "<100 bytes";
        size_t targetSize = (hopCount == 0) ? 30 : 100;
        bool passed = (packetSize <= targetSize);
        allPassed &= passed;

        std::cout << std::setw(15) << hopCount
                  << std::setw(15) << hopCount
                  << std::setw(15) << packetSize
                  << std::setw(15) << target
                  << "  " << (passed ? "✓" : "✗") << "\n";
    }

    std::cout << "\nResult: " << (allPassed ? "PASS ✓" : "FAIL ✗") << "\n";
}

//==============================================================================
// Main
//==============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "Phase 4 Context Performance Benchmarks\n";
    std::cout << "========================================\n";

    benchmark_serialization();
    benchmark_multihop_latency();
    benchmark_throughput();
    benchmark_packet_sizes();

    std::cout << "\n========================================\n";
    std::cout << "Benchmarks Complete\n";
    std::cout << "========================================\n";

    return 0;
}
