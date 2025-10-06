# Network MIDI Mesh - Performance Benchmarks

This directory contains comprehensive performance benchmarking suite for the Network MIDI Mesh implementation.

## Quick Start

### Build

```bash
cd /Users/orion/work/ol_dsp-midi-server
cmake -B build -S .
cmake --build build --target network_midi_benchmarks
```

### Run

```bash
./build/modules/juce/midi-server/network_midi_benchmarks_artefacts/network_midi_benchmarks
```

### With Optimizations (Recommended for production benchmarking)

```bash
cmake -B build-release -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target network_midi_benchmarks
./build-release/modules/juce/midi-server/network_midi_benchmarks_artefacts/network_midi_benchmarks
```

## Benchmarks Included

### 1. Packet Serialization/Deserialization
- Tests minimal packets (3 bytes - Note On)
- Average packets (6 bytes - Control Change)
- Large packets (64 bytes - SysEx)
- Measures serialize() and deserialize() performance
- **Target**: < 1 μs per operation

### 2. UDP Transport Latency
- Measures packet preparation overhead (create + serialize)
- Note: Full RTT requires integration testing with running server
- **Target**: < 100 μs round-trip (localhost)

### 3. Message Buffer Reordering
- In-order delivery (best case)
- Out-of-order reordering (worst case - reverse order)
- Large buffer stress test (1000 packets)
- **Target**: < 5 μs per reordering operation

### 4. Device/Route Lookup Performance
- Tests DeviceRegistry and RoutingTable lookups
- Scales testing with 10, 50, 100 devices
- Measures O(log n) std::map performance
- **Target**: < 100 ns per lookup

### 5. Memory Footprint Analysis
- Data structure sizes
- Per-packet overhead
- Buffered message memory usage
- Device/route storage requirements

### 6. Message Throughput
- End-to-end packet processing (create → serialize → deserialize)
- Measures msgs/sec single-threaded
- 100,000 message stress test

## Performance Targets (from workplan)

| Component | Target | Status |
|-----------|--------|--------|
| Packet serialization | < 1 μs | ✓ PASS (516 ns avg) |
| Packet deserialization | < 1 μs | ✓ PASS (383-599 ns avg) |
| Buffer reordering | < 5 μs | ✓ PASS (251 ns avg) |
| Lookup operations | < 100 ns | ✗ MARGINAL (153-208 ns avg) |
| RTT (localhost) | < 100 μs | Integration test required |

## Results

See [BENCHMARK_RESULTS.md](./BENCHMARK_RESULTS.md) for detailed analysis and recommendations.

### Summary
- **Serialization**: 2x faster than target ✓
- **Deserialization**: 2x faster than target ✓
- **Buffer reordering**: 20x faster than target ✓
- **Throughput**: 646K msgs/sec single-threaded ✓
- **Lookups**: 1.5-2x slower than target (but < 1 μs) ⚠️

## Implementation Details

**File**: `PerformanceBenchmark.cpp` (467 lines)
**Dependencies**:
- MidiPacket (serialization)
- MessageBuffer (reordering)
- DeviceRegistry (device lookups)
- RoutingTable (route lookups)
- UdpMidiTransport (transport layer)

**Statistics Collection**:
- Uses C++17 `std::chrono::high_resolution_clock`
- Measures min, max, avg, p95, p99, stddev
- 1000-10000 iterations per benchmark
- Reports in nanoseconds/microseconds

## Optimization Recommendations

### 1. Enable Compiler Optimizations
For production builds, use `-O3` or `-Ofast`:
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
# Or for aggressive optimization:
cmake -B build -S . -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG"
```

Expected improvements:
- Serialization: ~250-300 ns (2x faster)
- Deserialization: ~200-250 ns (2x faster)
- Lookups: ~80-120 ns (may meet target)

### 2. Lookup Optimization (Optional)
If lookup performance is critical:
```cpp
// Replace std::map with std::unordered_map for O(1) lookups
std::unordered_map<uint16_t, MidiDevice> devices;
std::unordered_map<uint16_t, Route> routes;
```

### 3. Integration Testing
Next steps:
1. UDP round-trip latency with running server
2. Multi-threaded throughput testing
3. Network stress testing (packet loss, reordering)
4. Real MIDI hardware end-to-end latency

## Performance Profiling

### macOS (Instruments)
```bash
instruments -t "Time Profiler" ./network_midi_benchmarks
```

### Linux (perf)
```bash
perf record -g ./network_midi_benchmarks
perf report
```

### Valgrind (Memory/Cache profiling)
```bash
valgrind --tool=cachegrind ./network_midi_benchmarks
valgrind --tool=massif ./network_midi_benchmarks
```

## Contributing

When adding new benchmarks:
1. Follow the existing pattern (BenchmarkStats struct)
2. Report min/max/avg/p95/p99/stddev
3. Compare against performance targets
4. Keep benchmark functions focused (< 100 lines each)
5. Document expected performance in comments

## License

Part of the OL_DSP project. See parent LICENSE file.
