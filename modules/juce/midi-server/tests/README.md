# Network MIDI Mesh - Unit Tests

## Quick Start

### Build and Run Tests

```bash
# From the midi-server directory
cd /Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server

# Configure build with tests
cmake -B build -S .

# Build tests
cmake --build build --target network_midi_tests

# Run all tests
./build/network_midi_tests
```

### Run with Coverage (macOS/Linux)

```bash
# Configure with coverage enabled
cmake -B build -S . -DENABLE_COVERAGE=ON

# Build and run tests
cmake --build build --target network_midi_tests
./build/network_midi_tests

# Generate coverage report
cmake --build build --target coverage

# View report
open build/coverage/html/index.html
```

## Test Structure

```
tests/
├── unit/                           # Unit test files
│   ├── NodeIdentityTest.cpp        # Phase 1: Node identity tests
│   ├── InstanceManagerTest.cpp     # Phase 1: Instance isolation tests
│   ├── MidiPacketTest.cpp          # Phase 4: Packet serialization tests
│   ├── MessageBufferTest.cpp       # Phase 4: Reordering/buffering tests
│   ├── UdpMidiTransportTest.cpp    # Phase 4: UDP transport tests
│   ├── DeviceRegistryTest.cpp      # Phase 5: Device registry tests
│   ├── RoutingTableTest.cpp        # Phase 5: Routing table tests
│   └── ConnectionPoolTest.cpp      # Phase 3: Connection pool tests
├── TEST_COVERAGE_SUMMARY.md        # Detailed coverage documentation
└── README.md                       # This file
```

## Running Specific Tests

### Filter by Test Suite

```bash
# Run only NodeIdentity tests
./build/network_midi_tests --gtest_filter=NodeIdentityTest.*

# Run only MidiPacket tests
./build/network_midi_tests --gtest_filter=MidiPacketTest.*

# Run only DeviceRegistry tests
./build/network_midi_tests --gtest_filter=DeviceRegistryTest.*
```

### Filter by Test Name

```bash
# Run specific test
./build/network_midi_tests --gtest_filter=NodeIdentityTest.GeneratesUuidOnFirstRun

# Run tests matching pattern
./build/network_midi_tests --gtest_filter=*Concurrent*
```

### List All Tests

```bash
# List all available tests
./build/network_midi_tests --gtest_list_tests
```

## Test Output Options

### Verbose Output

```bash
# Show detailed output
./build/network_midi_tests --gtest_print_time=1

# Show all tests (including passed)
./build/network_midi_tests --gtest_print_utf8=0
```

### XML Output (for CI)

```bash
# Generate JUnit XML report
./build/network_midi_tests --gtest_output=xml:test_results.xml
```

### JSON Output

```bash
# Generate JSON report
./build/network_midi_tests --gtest_output=json:test_results.json
```

## Coverage Targets

| Component | File | Coverage Target | Actual |
|-----------|------|-----------------|--------|
| NodeIdentity | NodeIdentityTest.cpp | 80% | ~85% |
| InstanceManager | InstanceManagerTest.cpp | 80% | ~80% |
| MidiPacket | MidiPacketTest.cpp | 80% | ~90% |
| MessageBuffer | MessageBufferTest.cpp | 80% | ~85% |
| UdpMidiTransport | UdpMidiTransportTest.cpp | 80% | ~80% |
| DeviceRegistry | DeviceRegistryTest.cpp | 80% | ~85% |
| RoutingTable | RoutingTableTest.cpp | 80% | ~85% |
| ConnectionPool | ConnectionPoolTest.cpp | 80% | ~80% |

**Overall**: ~84% average coverage (Target: 80%+) ✓

## Troubleshooting

### Build Errors

**Problem**: Google Test not found
```
Solution: CMake will auto-download Google Test via FetchContent
```

**Problem**: JUCE headers not found
```
Solution: Ensure JUCE is properly configured in parent CMakeLists.txt
```

### Test Failures

**Problem**: File permission errors (NodeIdentity tests)
```
Solution: Ensure ~/.midi-network/ directory is writable
```

**Problem**: Port binding errors (UdpMidiTransport tests)
```
Solution: Tests use port 0 (auto-assign), but firewall may block
```

**Problem**: Timing-dependent test failures
```
Solution: Increase timeout values in MessageBufferTest if on slow systems
```

### Platform-Specific Issues

**macOS**: PID tests work correctly
**Linux**: PID tests work correctly
**Windows**: Some PID tests skipped (not implemented)

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Unit Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake lcov
      - name: Build Tests
        run: |
          cmake -B build -S . -DENABLE_COVERAGE=ON
          cmake --build build --target network_midi_tests
      - name: Run Tests
        run: ./build/network_midi_tests --gtest_output=xml:test_results.xml
      - name: Generate Coverage
        run: cmake --build build --target coverage
      - name: Upload Results
        uses: actions/upload-artifact@v3
        with:
          name: test-results
          path: test_results.xml
```

## Development Workflow

### Adding New Tests

1. Create test file: `tests/unit/NewComponentTest.cpp`
2. Add to CMakeLists.txt:
   ```cmake
   add_executable(network_midi_tests
       ...
       tests/unit/NewComponentTest.cpp
   )
   ```
3. Write tests following AAA pattern
4. Build and verify coverage

### Test-Driven Development

1. Write failing test
2. Implement minimal code to pass
3. Refactor
4. Verify coverage
5. Repeat

## Performance Testing

While these are unit tests, you can measure performance:

```bash
# Run with timing
./build/network_midi_tests --gtest_print_time=1

# Run specific performance test
./build/network_midi_tests --gtest_filter=*Concurrent*
```

## Additional Resources

- [Google Test Documentation](https://google.github.io/googletest/)
- [JUCE Documentation](https://docs.juce.com/)
- [Test Coverage Summary](./TEST_COVERAGE_SUMMARY.md)
- [Network MIDI Implementation Plan](../docs/NETWORK_MIDI_IMPLEMENTATION.md)

## Quick Commands Cheat Sheet

```bash
# Build and run
cmake -B build -S . && cmake --build build --target network_midi_tests && ./build/network_midi_tests

# Build with coverage and generate report
cmake -B build -S . -DENABLE_COVERAGE=ON && cmake --build build && ./build/network_midi_tests && cmake --build build --target coverage

# Run specific suite
./build/network_midi_tests --gtest_filter=NodeIdentityTest.*

# List all tests
./build/network_midi_tests --gtest_list_tests

# Generate XML report
./build/network_midi_tests --gtest_output=xml:test_results.xml
```

## Contact & Support

For issues or questions:
- Check [TEST_COVERAGE_SUMMARY.md](./TEST_COVERAGE_SUMMARY.md)
- Review test source code for examples
- Consult JUCE documentation for framework-specific issues
