# SEDA Implementation Workplan

**Document Version**: 1.0
**Date**: 2025-10-06
**Status**: Ready for Execution
**Related Design**: [design.md](../planning/design.md)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Strategic Decision](#strategic-decision)
3. [Approach A: Mutex Fixes (Recommended Start)](#approach-a-mutex-fixes-recommended-start)
4. [Approach B: Full SEDA Migration](#approach-b-full-seda-migration)
5. [Decision Criteria](#decision-criteria)
6. [Integration with Current Phase 6](#integration-with-current-phase-6)
7. [Testing & Validation Strategy](#testing--validation-strategy)
8. [Timeline & Resources](#timeline--resources)
9. [Risk Assessment](#risk-assessment)
10. [Rollback Procedures](#rollback-procedures)

---

## Executive Summary

This workplan provides two parallel implementation strategies for resolving the mutex-based deadlock potential in `NetworkConnection`:

- **Approach A (Mutex Fixes)**: Quick, low-risk fixes to eliminate callback-under-lock and race conditions - **2-4 hours**
- **Approach B (Full SEDA)**: Complete architectural migration to event-driven stages - **8-14 hours**

**Recommendation**: Start with Approach A to eliminate immediate deadlock risks, then collect production metrics to determine if full SEDA migration is warranted.

### Key Metrics

| Metric | Current (Mutex) | After Fixes | Full SEDA | Target |
|--------|----------------|-------------|-----------|--------|
| Deadlock Risk | Medium | Low | Zero | Zero |
| Query Latency | ~100ns-10ms | ~100ns | ~10ns (atomic) | <1ms |
| Implementation Time | 0h | 2-4h | 8-14h | Minimize |
| Code Complexity | Low | Low | Medium-High | Balance |
| Maintainability | Good | Good | Medium | High |

---

## Strategic Decision

### Decision Framework

```
┌─────────────────────────────────────────────────────────┐
│                  Decision Tree                           │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  Are deadlocks occurring in production?                 │
│           │                                              │
│           ├── NO ──> Start with Approach A (Mutex Fixes)│
│           │          Monitor for 1-2 weeks               │
│           │          Collect metrics                     │
│           │                                              │
│           └── YES ──> Skip to Approach B (Full SEDA)    │
│                       High priority migration            │
│                                                           │
│  After Approach A:                                       │
│  ├── Metrics show <1% contention ──> DONE              │
│  ├── Metrics show >5% contention ──> Proceed to B      │
│  └── Deadlocks still occur ──> Emergency B              │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

### Current Status Assessment

Based on the design document analysis:

- **Deadlock Evidence**: Potential but not confirmed in production
- **Contention Evidence**: Not measured
- **Scale Requirements**: Current mesh typically <10 connections
- **Performance Requirements**: HTTP queries must complete <100ms

**Recommended Path**: **Approach A first**, with Approach B as planned follow-up if metrics justify it.

---

## Approach A: Mutex Fixes (Recommended Start)

### Overview

Eliminate the three primary deadlock scenarios with minimal code changes:

1. Fix callback-under-lock pattern in `setState()`
2. Protect `remoteDevices` with proper synchronization
3. Move callback dispatch to message thread
4. Add defensive timeouts on mutex acquisitions

**Effort Estimate**: 2-4 hours
**Risk Level**: Low
**Rollback Complexity**: Trivial (git revert)

---

### Phase A.1: Fix Callback-Under-Lock

**Objective**: Ensure callbacks are never invoked while holding mutexes.

**Duration**: 30-45 minutes

#### Tasks

1. **Modify `NetworkConnection::setState()`**
   - File: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/mesh/NetworkConnection.cpp`
   - Change callback invocation pattern

2. **Implementation**:

```cpp
// BEFORE (NetworkConnection.cpp:255-271)
void NetworkConnection::setState(State newState)
{
    State oldState = currentState.load();
    // ...
    currentState = newState;
    // ...
    if (onStateChanged) {
        onStateChanged(oldState, newState);  // ⚠️ UNDER LOCK
    }
}

// AFTER
void NetworkConnection::setState(State newState)
{
    State oldState = currentState.load();

    // Update state under lock (if needed)
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        currentState = newState;
        // ... other state updates ...
    }

    // Dispatch callback OUTSIDE lock via message thread
    if (onStateChanged) {
        auto callback = onStateChanged;  // Copy callback
        juce::MessageManager::callAsync([callback, oldState, newState]() {
            callback(oldState, newState);
        });
    }
}
```

#### Acceptance Criteria

- [ ] `setState()` releases all mutexes before callback invocation
- [ ] Callbacks dispatched via `juce::MessageManager::callAsync()`
- [ ] Existing state transition tests still pass
- [ ] No compiler warnings

#### Testing

```cpp
// New test: Ensure callbacks are async
TEST(NetworkConnectionMutexFix, CallbackNotUnderLock) {
    NetworkConnection conn(testNodeInfo);

    std::atomic<bool> callbackExecuted{false};
    std::thread::id callbackThread;

    conn.onStateChanged = [&](auto oldState, auto newState) {
        callbackThread = std::this_thread::get_id();
        callbackExecuted = true;
    };

    conn.connect();

    // Wait for callback
    for (int i = 0; i < 100 && !callbackExecuted; ++i) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }

    EXPECT_TRUE(callbackExecuted);
    EXPECT_NE(callbackThread, std::this_thread::get_id());  // Different thread
}
```

---

### Phase A.2: Protect remoteDevices Access

**Objective**: Eliminate Race #3 (device list read without protection).

**Duration**: 45-60 minutes

#### Tasks

1. **Add mutex protection to device list operations**
   - File: `NetworkConnection.cpp` and `NetworkConnection.h`

2. **Implementation**:

```cpp
// NetworkConnection.h - Add new mutex
private:
    mutable std::mutex devicesMutex;  // Protects remoteDevices

// NetworkConnection.cpp:91-96 - Update getRemoteDevices()
std::vector<DeviceInfo> NetworkConnection::getRemoteDevices() const
{
    std::lock_guard<std::mutex> lock(devicesMutex);
    return remoteDevices;  // ✅ Now safe
}

// NetworkConnection.cpp:213-223 - Update performHandshake()
void NetworkConnection::performHandshake()
{
    // ... HTTP request ...

    if (success) {
        {
            std::lock_guard<std::mutex> lock(devicesMutex);
            remoteDevices = parsedDevices;  // ✅ Protected write
        }

        setState(State::Connected);
    }
}
```

#### Alternative: Use Atomic Shared Pointer (Zero-Copy Reads)

```cpp
// NetworkConnection.h
private:
    std::atomic<std::shared_ptr<const std::vector<DeviceInfo>>> remoteDevicesPtr;

// NetworkConnection.cpp
std::vector<DeviceInfo> NetworkConnection::getRemoteDevices() const
{
    auto devices = remoteDevicesPtr.load(std::memory_order_acquire);
    return devices ? *devices : std::vector<DeviceInfo>{};
}

void NetworkConnection::performHandshake()
{
    // ... parse devices ...

    auto devices = std::make_shared<std::vector<DeviceInfo>>(parsedDevices);
    remoteDevicesPtr.store(devices, std::memory_order_release);
}
```

**Recommendation**: Use atomic shared pointer for better read performance.

#### Acceptance Criteria

- [ ] All `remoteDevices` reads are protected
- [ ] All `remoteDevices` writes are protected
- [ ] No data races detected by ThreadSanitizer
- [ ] Device list tests pass

---

### Phase A.3: Add Mutex Timeouts (Defensive)

**Objective**: Prevent indefinite blocking in case of unforeseen contention.

**Duration**: 30-45 minutes

#### Tasks

1. **Replace `std::mutex` with `std::timed_mutex`**
   - File: `NetworkConnection.h`

2. **Implementation**:

```cpp
// NetworkConnection.h
private:
    std::timed_mutex stateMutex;
    std::timed_mutex heartbeatMutex;
    std::timed_mutex devicesMutex;

// NetworkConnection.cpp - Example with timeout
State NetworkConnection::getState() const
{
    std::unique_lock<std::timed_mutex> lock(stateMutex, std::chrono::milliseconds(100));

    if (!lock.owns_lock()) {
        // Log warning - mutex contention detected
        juce::Logger::writeToLog("WARNING: getState() mutex timeout - possible contention");

        // Return atomic snapshot as fallback
        return currentState.load(std::memory_order_acquire);
    }

    return currentState;
}
```

#### Acceptance Criteria

- [ ] All mutex acquisitions have 100ms timeout
- [ ] Timeouts logged to help identify contention
- [ ] Fallback to atomic reads where possible
- [ ] No false positives in normal operation

---

### Phase A.4: Fix MeshManager Connection Lifecycle

**Objective**: Resolve Race #4 (connection may be deleted between pool query and dereference).

**Duration**: 45-60 minutes

#### Tasks

1. **Modify `MeshManager::getTotalDeviceCount()`**
   - File: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/mesh/MeshManager.cpp`

2. **Implementation**:

```cpp
// BEFORE (MeshManager.cpp:147-166)
int MeshManager::getTotalDeviceCount() const
{
    auto connections = connectionPool.getAllConnections();  // ⚠️ Pool lock released

    for (auto* connection : connections) {
        if (connection && connection->getState() == NetworkConnection::State::Connected) {
            total += (int)connection->getRemoteDevices().size();  // ⚠️ May be deleted!
        }
    }
}

// AFTER - Hold pool lock during iteration
int MeshManager::getTotalDeviceCount() const
{
    return connectionPool.withAllConnections([](const auto& connections) {
        int total = 0;
        for (const auto* connection : connections) {
            if (connection && connection->getState() == NetworkConnection::State::Connected) {
                total += (int)connection->getRemoteDevices().size();
            }
        }
        return total;
    });
}

// ConnectionPool.h - Add withAllConnections helper
template<typename Func>
auto withAllConnections(Func&& func) const {
    std::lock_guard<std::mutex> lock(poolMutex);
    return func(connections);  // Callback executes under lock
}
```

#### Acceptance Criteria

- [ ] Connection pool operations atomic
- [ ] No use-after-free in iteration
- [ ] All mesh manager tests pass
- [ ] No deadlocks when querying pool

---

### Phase A.5: Validation & Testing

**Duration**: 30-45 minutes

#### Validation Checklist

- [ ] Build succeeds with no warnings: `cmake --build build`
- [ ] All existing unit tests pass: `ctest --test-dir build`
- [ ] ThreadSanitizer clean: `cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..`
- [ ] Stress test: 100 threads querying concurrently for 60 seconds
- [ ] Callback ordering test: Verify message thread execution
- [ ] Performance baseline: Measure query latency before/after

#### New Tests Required

```cpp
// Test 1: Concurrent queries with state changes
TEST(NetworkConnectionMutexFix, ConcurrentQueriesDuringHandshake) {
    // 50 reader threads + 1 connection thread
    // Run for 10 seconds, verify no deadlocks
}

// Test 2: Callback reentrancy safety
TEST(NetworkConnectionMutexFix, CallbackReentrancySafe) {
    // Callback calls getRemoteNode() - should not deadlock
}

// Test 3: Device list consistency
TEST(NetworkConnectionMutexFix, DeviceListReadDuringWrite) {
    // Read devices while handshake updates list
    // Verify no crashes, no partial data
}

// Test 4: Pool iteration safety
TEST(MeshManagerFix, PoolIterationDuringDisconnect) {
    // Disconnect connection while getTotalDeviceCount() iterating
    // Verify no use-after-free
}
```

---

## Approach B: Full SEDA Migration

### Overview

Complete architectural transformation to staged event-driven design with zero shared mutexes.

**Effort Estimate**: 8-14 hours
**Risk Level**: Medium
**Rollback Complexity**: Moderate (feature branch required)

**Note**: Only proceed if Approach A proves insufficient OR if production metrics show >5% mutex contention.

---

### Phase B.1: Infrastructure Setup

**Objective**: Create command queue and worker thread infrastructure.

**Duration**: 2-3 hours

#### Tasks

1. **Create command hierarchy**
   - New file: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/mesh/NetworkCommands.h`

```cpp
#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <memory>
#include <vector>

namespace NetworkMidi {
namespace Commands {

// Base command
struct Command {
    enum Type {
        Connect,
        Disconnect,
        CheckHeartbeat,
        NotifyHeartbeat,
        SendMidi,
        GetRemoteNode,
        GetDevices,
        Shutdown
    };

    Type type;
    virtual ~Command() = default;
};

// Simple commands
struct ConnectCommand : Command {
    ConnectCommand() { type = Connect; }
};

struct DisconnectCommand : Command {
    DisconnectCommand() { type = Disconnect; }
};

struct CheckHeartbeatCommand : Command {
    CheckHeartbeatCommand() { type = CheckHeartbeat; }
};

struct NotifyHeartbeatCommand : Command {
    NotifyHeartbeatCommand() { type = NotifyHeartbeat; }
};

// Query commands with synchronous response
struct GetRemoteNodeQuery : Command {
    juce::WaitableEvent responseReady;
    NodeInfo result;

    GetRemoteNodeQuery() { type = GetRemoteNode; }
};

struct GetDevicesQuery : Command {
    juce::WaitableEvent responseReady;
    std::vector<DeviceInfo> result;

    GetDevicesQuery() { type = GetDevices; }
};

struct ShutdownCommand : Command {
    ShutdownCommand() { type = Shutdown; }
};

} // namespace Commands
} // namespace NetworkMidi
```

2. **Create command queue**
   - New file: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/mesh/CommandQueue.h`

```cpp
#pragma once

#include "NetworkCommands.h"
#include <juce_events/juce_events.h>
#include <deque>
#include <mutex>

namespace NetworkMidi {

class CommandQueue {
public:
    void push(std::unique_ptr<Commands::Command> cmd) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            queue.push_back(std::move(cmd));
        }
        queueEvent.signal();
    }

    std::unique_ptr<Commands::Command> pop(int timeoutMs) {
        if (!queueEvent.wait(timeoutMs)) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        if (queue.empty()) {
            return nullptr;
        }

        auto cmd = std::move(queue.front());
        queue.pop_front();

        if (!queue.empty()) {
            queueEvent.signal();
        }

        return cmd;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(queueMutex);
        return queue.size();
    }

private:
    mutable std::mutex queueMutex;
    std::deque<std::unique_ptr<Commands::Command>> queue;
    juce::WaitableEvent queueEvent;
};

} // namespace NetworkMidi
```

3. **Create worker thread**
   - New file: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/mesh/ConnectionWorker.h`

```cpp
#pragma once

#include "CommandQueue.h"
#include "NetworkCommands.h"
#include <juce_core/juce_core.h>
#include <atomic>

namespace NetworkMidi {

class ConnectionWorker : public juce::Thread {
public:
    ConnectionWorker(CommandQueue& queue);
    ~ConnectionWorker() override;

    void run() override;

    // Atomic snapshots for fast queries
    State getStateSnapshot() const {
        return stateSnapshot.load(std::memory_order_acquire);
    }

    int64_t getHeartbeatSnapshot() const {
        return heartbeatSnapshot.load(std::memory_order_acquire);
    }

private:
    CommandQueue& commandQueue;

    // Worker-owned state (no mutexes needed)
    State currentState{State::Disconnected};
    int64_t lastHeartbeatTime{0};
    NodeInfo remoteNodeInfo;
    std::vector<DeviceInfo> remoteDevices;
    std::unique_ptr<httplib::Client> httpClient;

    // Atomic snapshots for queries
    std::atomic<State> stateSnapshot{State::Disconnected};
    std::atomic<int64_t> heartbeatSnapshot{0};

    // Command handlers
    void processCommand(std::unique_ptr<Commands::Command> cmd);
    void handleConnect();
    void handleDisconnect();
    void handleCheckHeartbeat();
    void handleNotifyHeartbeat();
    void handleGetRemoteNodeQuery(Commands::GetRemoteNodeQuery* query);
    void handleGetDevicesQuery(Commands::GetDevicesQuery* query);

    // Helper methods
    void updateSnapshots();
    void dispatchStateChangedCallback(State oldState, State newState);
    bool performHandshake();
};

} // namespace NetworkMidi
```

#### Acceptance Criteria

- [ ] CommandQueue tests pass (push/pop operations)
- [ ] Worker thread starts and stops cleanly
- [ ] Command dispatch works (send Connect, verify handleConnect called)
- [ ] No memory leaks (valgrind clean)

---

### Phase B.2: State Migration

**Objective**: Move state ownership to worker thread, use atomic snapshots for queries.

**Duration**: 2-3 hours

#### Tasks

1. **Modify NetworkConnection to use worker**
   - File: `NetworkConnection.h`

```cpp
class NetworkConnection {
public:
    NetworkConnection(const NodeInfo& remoteNode);
    ~NetworkConnection();

    // Public API - dispatch commands
    void connect();
    void disconnect();

    // Fast queries via atomic snapshots
    State getState() const {
        return worker.getStateSnapshot();
    }

    int64_t getTimeSinceLastHeartbeat() const {
        int64_t last = worker.getHeartbeatSnapshot();
        return juce::Time::getCurrentTime().toMilliseconds() - last;
    }

    // Complex queries via command queue
    NodeInfo getRemoteNode() const;
    std::vector<DeviceInfo> getRemoteDevices() const;

    // Callbacks
    std::function<void(State, State)> onStateChanged;
    std::function<void(const juce::String&)> onError;

private:
    CommandQueue commandQueue;
    ConnectionWorker worker;

    // NO MUTEXES - all state owned by worker
};
```

2. **Implement query methods**
   - File: `NetworkConnection.cpp`

```cpp
NetworkConnection::NetworkConnection(const NodeInfo& remoteNode)
    : worker(commandQueue, remoteNode)
{
    worker.startThread();
}

NetworkConnection::~NetworkConnection()
{
    commandQueue.push(std::make_unique<Commands::ShutdownCommand>());
    worker.stopThread(2000);  // 2 second timeout
}

void NetworkConnection::connect()
{
    commandQueue.push(std::make_unique<Commands::ConnectCommand>());
}

void NetworkConnection::disconnect()
{
    commandQueue.push(std::make_unique<Commands::DisconnectCommand>());
}

NodeInfo NetworkConnection::getRemoteNode() const
{
    auto query = std::make_unique<Commands::GetRemoteNodeQuery>();
    auto* queryPtr = query.get();

    commandQueue.push(std::move(query));
    queryPtr->responseReady.wait();

    return queryPtr->result;
}

std::vector<DeviceInfo> NetworkConnection::getRemoteDevices() const
{
    auto query = std::make_unique<Commands::GetDevicesQuery>();
    auto* queryPtr = query.get();

    commandQueue.push(std::move(query));
    queryPtr->responseReady.wait();

    return queryPtr->result;
}
```

3. **Implement worker command handlers**
   - File: `ConnectionWorker.cpp`

```cpp
void ConnectionWorker::processCommand(std::unique_ptr<Commands::Command> cmd)
{
    switch (cmd->type) {
        case Commands::Command::Connect:
            handleConnect();
            break;

        case Commands::Command::Disconnect:
            handleDisconnect();
            break;

        case Commands::Command::CheckHeartbeat:
            handleCheckHeartbeat();
            break;

        case Commands::Command::GetRemoteNode:
            handleGetRemoteNodeQuery(
                static_cast<Commands::GetRemoteNodeQuery*>(cmd.get()));
            break;

        case Commands::Command::GetDevices:
            handleGetDevicesQuery(
                static_cast<Commands::GetDevicesQuery*>(cmd.get()));
            break;

        case Commands::Command::Shutdown:
            signalThreadShouldExit();
            break;
    }
}

void ConnectionWorker::handleConnect()
{
    State oldState = currentState;
    currentState = State::Connecting;
    updateSnapshots();

    bool success = performHandshake();

    if (success) {
        currentState = State::Connected;
        lastHeartbeatTime = juce::Time::getCurrentTime().toMilliseconds();
        updateSnapshots();

        dispatchStateChangedCallback(oldState, State::Connected);
    } else {
        currentState = State::Failed;
        updateSnapshots();

        dispatchStateChangedCallback(oldState, State::Failed);
    }
}

void ConnectionWorker::handleGetRemoteNodeQuery(Commands::GetRemoteNodeQuery* query)
{
    query->result = remoteNodeInfo;
    query->responseReady.signal();
}

void ConnectionWorker::handleGetDevicesQuery(Commands::GetDevicesQuery* query)
{
    query->result = remoteDevices;
    query->responseReady.signal();
}

void ConnectionWorker::updateSnapshots()
{
    stateSnapshot.store(currentState, std::memory_order_release);
    heartbeatSnapshot.store(lastHeartbeatTime, std::memory_order_release);
}

void ConnectionWorker::dispatchStateChangedCallback(State oldState, State newState)
{
    if (onStateChanged) {
        juce::MessageManager::callAsync([this, oldState, newState]() {
            if (onStateChanged) {
                onStateChanged(oldState, newState);
            }
        });
    }
}
```

#### Acceptance Criteria

- [ ] All state mutations happen on worker thread
- [ ] getState() uses atomic snapshot (no blocking)
- [ ] getRemoteNode() uses query command (blocking)
- [ ] No mutexes in NetworkConnection class
- [ ] ThreadSanitizer reports zero data races

---

### Phase B.3: Mutex Removal

**Objective**: Remove all mutex fields from NetworkConnection.

**Duration**: 1-2 hours

#### Tasks

1. **Remove mutex declarations**
   - File: `NetworkConnection.h`

```cpp
// REMOVE these lines:
// std::mutex stateMutex;
// std::mutex heartbeatMutex;
// std::mutex messageMutex;
// std::mutex devicesMutex;
```

2. **Remove all lock_guard usage**
   - File: `NetworkConnection.cpp`
   - Search for `std::lock_guard` and verify all removed

3. **Update HeartbeatMonitor integration**
   - File: `HeartbeatMonitor.cpp`

```cpp
void HeartbeatMonitor::checkTimeouts()
{
    // Now uses atomic snapshot - no blocking
    for (auto* connection : connections) {
        if (connection->getTimeSinceLastHeartbeat() > timeoutMs) {
            // Post check command instead of direct call
            connection->checkHeartbeat();
        }
    }
}
```

#### Acceptance Criteria

- [ ] Zero `std::mutex` or `std::lock_guard` in NetworkConnection
- [ ] Build succeeds
- [ ] All tests pass
- [ ] No performance regression in benchmarks

---

### Phase B.4: Integration Updates

**Objective**: Update MeshManager and HTTP handlers to work with new API.

**Duration**: 2-3 hours

#### Tasks

1. **Update MeshManager**
   - File: `MeshManager.cpp`
   - No changes needed if only using public API
   - Verify `getTotalDeviceCount()` still works

2. **Update HTTP handlers**
   - File: HTTP server implementation
   - Verify `/network/mesh` endpoint works
   - Ensure no blocking on main thread

3. **Update tests**
   - Modify existing tests to account for async callback dispatch
   - Add delays/polling where needed

```cpp
// OLD TEST
TEST(NetworkConnection, ConnectSetsState) {
    connection.connect();
    EXPECT_EQ(connection.getState(), State::Connecting);  // ❌ May be immediate
}

// NEW TEST
TEST(NetworkConnection, ConnectSetsState) {
    connection.connect();

    // Poll for state change
    for (int i = 0; i < 100; ++i) {
        if (connection.getState() == State::Connecting) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(connection.getState(), State::Connecting);
}
```

#### Acceptance Criteria

- [ ] MeshManager compiles and runs
- [ ] HTTP endpoints return correct data
- [ ] All integration tests pass
- [ ] No deadlocks in manual testing

---

### Phase B.5: Performance Benchmarking

**Objective**: Verify performance improvements match design estimates.

**Duration**: 1-2 hours

#### Tasks

1. **Create benchmark suite**
   - New file: `tests/performance/NetworkConnectionBenchmark.cpp`

```cpp
#include <benchmark/benchmark.h>

static void BM_GetState(benchmark::State& state) {
    NetworkConnection conn(testNode);

    for (auto _ : state) {
        auto s = conn.getState();  // Should be ~10ns
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_GetState);

static void BM_GetDevices(benchmark::State& state) {
    NetworkConnection conn(testNode);
    conn.connect();

    for (auto _ : state) {
        auto devices = conn.getRemoteDevices();  // Should be <1ms
        benchmark::DoNotOptimize(devices);
    }
}
BENCHMARK(BM_GetDevices);

static void BM_ConcurrentQueries(benchmark::State& state) {
    NetworkConnection conn(testNode);

    std::vector<std::thread> threads;
    for (int i = 0; i < state.range(0); ++i) {
        threads.emplace_back([&conn] {
            for (int j = 0; j < 1000; ++j) {
                conn.getState();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}
BENCHMARK(BM_ConcurrentQueries)->Range(1, 128);
```

2. **Run benchmarks**

```bash
cmake --build build --target NetworkConnectionBenchmark
./build/tests/performance/NetworkConnectionBenchmark --benchmark_format=json > results.json
```

3. **Compare metrics**

| Operation | Current | Target | Actual | Pass/Fail |
|-----------|---------|--------|--------|-----------|
| getState() | 100ns | <20ns | ??? | ??? |
| getRemoteDevices() | 100ns | <1ms | ??? | ??? |
| 100 threads querying | 10ms | <1ms | ??? | ??? |

#### Acceptance Criteria

- [ ] getState() <20ns (atomic read)
- [ ] getRemoteDevices() <1ms (command latency)
- [ ] 100 concurrent threads <1ms total
- [ ] No deadlocks in stress tests

---

### Phase B.6: TSAN & Stress Testing

**Objective**: Prove zero race conditions and deadlocks.

**Duration**: 1-2 hours

#### Tasks

1. **Build with ThreadSanitizer**

```bash
cmake -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake --build build-tsan
```

2. **Run stress tests**

```bash
./build-tsan/tests/unit/NetworkConnectionTest --gtest_filter="*Stress*"
```

3. **Create dedicated stress test**

```cpp
TEST(NetworkConnectionSEDA, StressTest_100Threads_60Seconds) {
    NetworkConnection conn(testNode);
    conn.connect();

    std::atomic<bool> stop{false};
    std::atomic<int> totalQueries{0};

    std::vector<std::thread> readers;
    for (int i = 0; i < 100; ++i) {
        readers.emplace_back([&] {
            while (!stop) {
                conn.getState();
                conn.getRemoteNode();
                conn.getRemoteDevices();
                ++totalQueries;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(60));
    stop = true;

    for (auto& t : readers) {
        t.join();
    }

    std::cout << "Total queries: " << totalQueries << std::endl;
    // Should be >1M queries in 60 seconds
    EXPECT_GT(totalQueries, 1000000);
}
```

#### Acceptance Criteria

- [ ] ThreadSanitizer reports zero warnings
- [ ] Stress test runs for 60 seconds without crash
- [ ] >1M queries/minute throughput
- [ ] Memory usage stable (no leaks)

---

## Decision Criteria

### When to Use Each Approach

#### Choose Approach A (Mutex Fixes) if:

- ✅ No confirmed deadlocks in production
- ✅ Mesh size expected <20 connections
- ✅ Query latency <1ms acceptable
- ✅ Development time limited
- ✅ Lower risk tolerance

#### Choose Approach B (Full SEDA) if:

- ✅ Deadlocks occurring in production
- ✅ Metrics show >5% mutex contention
- ✅ Mesh size expected >50 connections
- ✅ Query latency <100µs required
- ✅ Long-term scalability critical

#### Metrics to Collect After Approach A

```cpp
// Add instrumentation to NetworkConnection
class NetworkConnection {
private:
    // Contention metrics
    std::atomic<uint64_t> mutexContentionCount{0};
    std::atomic<uint64_t> totalMutexAcquisitions{0};

    State getState() const {
        totalMutexAcquisitions.fetch_add(1);

        std::unique_lock<std::timed_mutex> lock(stateMutex, std::chrono::milliseconds(10));
        if (!lock.owns_lock()) {
            mutexContentionCount.fetch_add(1);
            // ... fallback ...
        }
        // ...
    }

public:
    double getContentionPercentage() const {
        uint64_t total = totalMutexAcquisitions.load();
        return total > 0 ? (100.0 * mutexContentionCount / total) : 0.0;
    }
};
```

**Decision Point**: If contention >5% after 1 week of monitoring → Proceed to Approach B.

---

## Integration with Current Phase 6

### Phase 6 Context

According to the project workplan, Phase 6 focuses on:
- Network mesh topology management
- Heartbeat monitoring for connection health
- HTTP endpoints for mesh queries

### Integration Points

#### 1. MeshManager Integration

**Current State**: MeshManager uses ConnectionPool to manage NetworkConnection instances.

**Approach A Impact**:
- No API changes needed
- MeshManager continues to call `getState()`, `getRemoteDevices()` as before
- Callbacks dispatch via message thread (may need test updates)

**Approach B Impact**:
- No API changes needed (same public interface)
- Query performance improves significantly
- May need to update tests for async behavior

#### 2. HeartbeatMonitor Integration

**Current State**: HeartbeatMonitor timer calls `getTimeSinceLastHeartbeat()` every 1 second.

**Approach A Impact**:
- Minor: Callback dispatch becomes async
- Test update needed to poll for state changes

**Approach B Impact**:
- Significant performance improvement (atomic read vs mutex)
- Must post CheckHeartbeatCommand instead of direct setState()

**Required Change**:

```cpp
// HeartbeatMonitor.cpp
void HeartbeatMonitor::checkTimeouts()
{
    for (auto* connection : connections) {
        if (connection->getTimeSinceLastHeartbeat() > timeoutMs) {
            // APPROACH A: Direct call works (async callback)
            connection->setState(State::Failed);

            // APPROACH B: Post command
            connection->checkHeartbeat();  // Posts CheckHeartbeatCommand
        }
    }
}
```

#### 3. HTTP Endpoint Integration

**Current State**: HTTP handlers call `MeshManager::getStatistics()` which queries all connections.

**Approach A Impact**:
- None - queries work identically
- Potential latency improvement from reduced contention

**Approach B Impact**:
- Complex queries (getRemoteDevices) may add <1ms latency
- Simple queries (getState) become 100x faster

**No changes needed** - public API remains identical.

---

## Testing & Validation Strategy

### Test Matrix

| Test Category | Approach A | Approach B | Purpose |
|--------------|------------|------------|---------|
| Unit Tests | Update callbacks | Update callbacks + async | Verify correctness |
| Integration Tests | Minor updates | Moderate updates | Verify subsystem interaction |
| Concurrency Tests | New: 100 threads | New: stress test | Verify no deadlocks |
| Performance Tests | Baseline only | Full benchmarks | Verify improvements |
| Regression Tests | Full suite | Full suite | Verify no breakage |
| TSAN Tests | Required | Required | Verify no races |

### Validation Plan

#### Pre-Implementation Baseline

1. **Capture current metrics**

```bash
# Build baseline
cmake -B build-baseline
cmake --build build-baseline

# Run baseline tests
./build-baseline/tests/unit/NetworkConnectionTest

# Run baseline benchmarks
./build-baseline/tests/performance/NetworkConnectionBenchmark > baseline.json
```

2. **Create test checklist**

```markdown
- [ ] All 47 existing unit tests pass
- [ ] Integration test suite passes (mesh topology)
- [ ] Manual test: Connect 5 nodes, query mesh 100 times
- [ ] No crashes in 5-minute stress test
- [ ] ThreadSanitizer clean
```

#### Post-Implementation Validation

1. **Regression testing**

```bash
# Build with fixes
cmake -B build-fixed
cmake --build build-fixed

# Run all tests
ctest --test-dir build-fixed --output-on-failure

# Compare results
diff baseline-results.txt fixed-results.txt
```

2. **Performance validation**

```bash
# Run benchmarks
./build-fixed/tests/performance/NetworkConnectionBenchmark > fixed.json

# Compare
python3 scripts/compare_benchmarks.py baseline.json fixed.json
```

Expected output:
```
Metric              | Baseline | Fixed   | Change
--------------------|----------|---------|--------
getState() latency  | 120ns    | 115ns   | -4.2%  ✅
getDevices() latency| 150ns    | 145ns   | -3.3%  ✅
Contention rate     | 0.8%     | 0.1%    | -87.5% ✅
```

3. **Deadlock validation**

```bash
# ThreadSanitizer build
cmake -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread"
cmake --build build-tsan

# Run concurrency tests
./build-tsan/tests/unit/NetworkConnectionTest --gtest_filter="*Concurrent*"

# Expected: No TSAN warnings
```

### Load Testing Strategy

#### Scenario 1: Single Node Under Load

```cpp
TEST(NetworkConnection, LoadTest_SingleNode_1000QPS) {
    NetworkConnection conn(testNode);
    conn.connect();

    auto start = std::chrono::steady_clock::now();
    int queries = 0;

    while (queries < 10000) {
        conn.getState();
        conn.getRemoteDevices();
        ++queries;
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto qps = queries / std::chrono::duration<double>(elapsed).count();

    EXPECT_GT(qps, 1000);  // Should achieve >1000 QPS
}
```

#### Scenario 2: Mesh Under Load

```cpp
TEST(MeshManager, LoadTest_10Nodes_100QPS_Each) {
    MeshManager mesh;

    // Add 10 nodes
    for (int i = 0; i < 10; ++i) {
        mesh.addPeer(createTestNode(i));
    }

    // Query all nodes 1000 times (100 QPS × 10 nodes)
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < 1000; ++i) {
        auto stats = mesh.getStatistics();  // Queries all 10 nodes
        EXPECT_EQ(stats.totalConnections, 10);
    }

    auto elapsed = std::chrono::steady_clock::now() - start;

    // Should complete in <10 seconds (1000 queries / 100 QPS)
    EXPECT_LT(elapsed, std::chrono::seconds(10));
}
```

#### Scenario 3: Chaos Testing

```cpp
TEST(NetworkConnection, ChaosTest_RandomOperations) {
    NetworkConnection conn(testNode);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 5);

    for (int i = 0; i < 10000; ++i) {
        switch (dis(gen)) {
            case 0: conn.connect(); break;
            case 1: conn.disconnect(); break;
            case 2: conn.getState(); break;
            case 3: conn.getRemoteNode(); break;
            case 4: conn.getRemoteDevices(); break;
            case 5: conn.checkHeartbeat(); break;
        }

        // Random delay 0-10ms
        std::this_thread::sleep_for(
            std::chrono::milliseconds(dis(gen) * 2));
    }

    // Should survive chaos without crash
}
```

---

## Timeline & Resources

### Approach A: Mutex Fixes Timeline

| Phase | Duration | Developer Hours | Dependencies |
|-------|----------|----------------|--------------|
| A.1: Fix callback-under-lock | 30-45 min | 0.75h | None |
| A.2: Protect remoteDevices | 45-60 min | 1.0h | None |
| A.3: Add mutex timeouts | 30-45 min | 0.75h | None |
| A.4: Fix MeshManager lifecycle | 45-60 min | 1.0h | None |
| A.5: Validation & testing | 30-45 min | 0.75h | All previous |
| **Total** | **3-4 hours** | **4.25h** | - |

**Critical Path**: Linear (each phase builds on previous)
**Parallelization**: Minimal (single developer)
**Calendar Time**: 1 day (with buffer for testing)

### Approach B: Full SEDA Timeline

| Phase | Duration | Developer Hours | Dependencies |
|-------|----------|----------------|--------------|
| B.1: Infrastructure setup | 2-3 hours | 2.5h | None |
| B.2: State migration | 2-3 hours | 2.5h | B.1 |
| B.3: Mutex removal | 1-2 hours | 1.5h | B.2 |
| B.4: Integration updates | 2-3 hours | 2.5h | B.3 |
| B.5: Performance benchmarking | 1-2 hours | 1.5h | B.4 |
| B.6: TSAN & stress testing | 1-2 hours | 1.5h | B.4 |
| **Total** | **9-15 hours** | **12h** | - |

**Critical Path**: B.1 → B.2 → B.3 → B.4 → (B.5 + B.6 parallel)
**Parallelization**: B.5 and B.6 can run in parallel
**Calendar Time**: 2-3 days (with buffer for debugging)

### Resource Requirements

#### Approach A

- **Developers**: 1 senior C++ developer
- **Skills Required**:
  - C++17 threading (std::mutex, std::atomic)
  - JUCE framework (MessageManager)
  - Unit testing (GTest)
- **Infrastructure**:
  - Development machine with TSAN support
  - CI/CD pipeline with sanitizer builds

#### Approach B

- **Developers**: 1 senior C++ developer + 1 reviewer
- **Skills Required**:
  - Advanced C++ concurrency patterns
  - JUCE threading model (juce::Thread, juce::WaitableEvent)
  - Event-driven architecture design
  - Performance profiling tools
- **Infrastructure**:
  - Development machine with TSAN support
  - Benchmarking environment (isolated CPU cores)
  - CI/CD with performance regression detection

### External Dependencies

- **JUCE Framework**: Version 6.0+ (for modern threading primitives)
- **httplib**: Version 0.14.3 (already integrated)
- **GTest**: For unit testing
- **Google Benchmark**: For performance testing (optional for Approach A, required for B)
- **ThreadSanitizer**: Part of Clang/GCC toolchain

---

## Risk Assessment

### Approach A Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Callback timing breaks tests | Medium | Low | Update tests to poll for callbacks |
| Atomic shared_ptr not available in C++17 | Low | Medium | Use C++20 or mutex fallback |
| Mutex timeouts cause false alarms | Low | Low | Tune timeout values (100-500ms) |
| Contention persists after fixes | Medium | Medium | Be prepared to escalate to Approach B |
| Performance regression | Low | Low | Run baseline benchmarks first |

**Overall Risk**: Low

### Approach B Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Command latency breaks real-time requirements | Low | High | Use atomic snapshots for hot paths |
| Worker thread doesn't shut down cleanly | Medium | Medium | Implement robust shutdown with timeout |
| Query commands deadlock | Low | Critical | Use WaitableEvent with timeout |
| Increased memory usage (command queue) | Low | Low | Monitor queue depth, add bounds |
| Complexity increases bug surface area | Medium | Medium | Comprehensive testing, code review |
| Migration breaks existing tests | High | Medium | Update tests incrementally, maintain backward compat |
| Performance doesn't meet targets | Low | High | Benchmark early (Phase B.1), pivot if needed |

**Overall Risk**: Medium

### Mitigation Strategies

#### For Both Approaches

1. **Feature Branch Development**
   - Create `feature/mutex-fixes` or `feature/seda-migration` branch
   - No direct commits to main
   - Require PR review before merge

2. **Incremental Testing**
   - Run tests after each phase
   - Use CI/CD to catch regressions early
   - Maintain passing tests at all times

3. **Performance Monitoring**
   - Establish baseline metrics before changes
   - Monitor metrics after each phase
   - Rollback if regressions >10%

4. **Canary Deployment**
   - Deploy to test environment first
   - Monitor for 24-48 hours
   - Gradual rollout to production

---

## Rollback Procedures

### Approach A Rollback

**Simplicity**: Trivial (single commit revert)

```bash
# If issues detected after merge
git revert <commit-hash>
git push origin main

# Rebuild
cmake --build build
ctest --test-dir build
```

**Recovery Time**: <10 minutes

**Data Loss**: None (no state migration)

### Approach B Rollback

**Complexity**: Moderate (may span multiple commits)

#### Option 1: Clean Revert (If Single Merge Commit)

```bash
git revert -m 1 <merge-commit>
git push origin main
```

#### Option 2: Branch Revert (If Multiple Commits)

```bash
# Create rollback branch
git checkout -b rollback/seda-migration

# Revert range of commits
git revert <first-commit>..<last-commit>

# Test rollback
cmake --build build
ctest --test-dir build

# Push rollback
git push origin rollback/seda-migration
```

#### Option 3: Forward Fix (If Partial Issues)

If only specific components broken:

```bash
# Fix individual issues without full rollback
git checkout main
git cherry-pick <working-commit>
```

**Recovery Time**: 30-60 minutes

**Data Loss**: None (runtime state only)

### Rollback Decision Criteria

| Severity | Symptom | Action | Timeline |
|----------|---------|--------|----------|
| P0 | Production deadlocks | Immediate rollback | <15 min |
| P1 | Crashes in <1% of requests | Rollback within 1 hour | <1 hour |
| P2 | Performance regression >20% | Rollback within 4 hours | <4 hours |
| P3 | Minor test failures | Forward fix | <1 day |

### Rollback Testing

Before deploying either approach, verify rollback procedure:

```bash
# Simulate rollback in test environment
git checkout test-environment
git merge feature/mutex-fixes
# ... deploy, verify working ...
git revert HEAD
# ... verify rollback successful ...
```

---

## Appendix A: Command Reference

### Build Commands

```bash
# Clean build
rm -rf build && cmake -B build && cmake --build build

# Build with sanitizers
cmake -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build-tsan

# Build with optimizations
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

# Run tests
ctest --test-dir build --output-on-failure

# Run specific test
./build/tests/unit/NetworkConnectionTest --gtest_filter="*Concurrent*"
```

### Monitoring Commands

```bash
# Check for deadlocks (requires gdb)
gdb -batch -ex "thread apply all bt" ./build/MidiHttpServer

# Monitor queue depth (add to ConnectionWorker)
watch -n 1 "grep 'Queue depth' /var/log/midi-server.log"

# Check contention percentage
curl http://localhost:8080/debug/metrics | jq '.contention_percent'
```

---

## Appendix B: Checklist Summary

### Approach A Checklist

- [ ] Phase A.1: Callback-under-lock fixed
  - [ ] setState() releases locks before callback
  - [ ] Callbacks dispatched via MessageManager
  - [ ] Tests updated for async callbacks
- [ ] Phase A.2: remoteDevices protected
  - [ ] Mutex or atomic shared_ptr protecting reads/writes
  - [ ] No TSAN warnings
- [ ] Phase A.3: Mutex timeouts added
  - [ ] All mutexes use std::timed_mutex
  - [ ] Timeouts logged
  - [ ] Fallback to atomics where possible
- [ ] Phase A.4: MeshManager fixed
  - [ ] Pool iteration protected from use-after-free
  - [ ] withAllConnections helper implemented
- [ ] Phase A.5: Validation complete
  - [ ] All tests pass
  - [ ] Benchmarks show no regression
  - [ ] TSAN clean
  - [ ] Contention metrics <1%

### Approach B Checklist

- [ ] Phase B.1: Infrastructure
  - [ ] NetworkCommands.h created
  - [ ] CommandQueue.h created and tested
  - [ ] ConnectionWorker.h created
  - [ ] Worker thread lifecycle tested
- [ ] Phase B.2: State migration
  - [ ] NetworkConnection uses worker
  - [ ] Atomic snapshots for getState(), getHeartbeat()
  - [ ] Query commands for complex data
  - [ ] All handlers implemented
- [ ] Phase B.3: Mutex removal
  - [ ] All mutex fields removed
  - [ ] All lock_guard removed
  - [ ] TSAN clean
- [ ] Phase B.4: Integration
  - [ ] MeshManager works with new API
  - [ ] HeartbeatMonitor works with new API
  - [ ] HTTP handlers work with new API
  - [ ] All integration tests pass
- [ ] Phase B.5: Performance
  - [ ] Benchmarks created
  - [ ] getState() <20ns
  - [ ] getRemoteDevices() <1ms
  - [ ] 100 threads <1ms total
- [ ] Phase B.6: Stress testing
  - [ ] TSAN build passes
  - [ ] 60-second stress test passes
  - [ ] >1M queries/minute achieved
  - [ ] No memory leaks

---

## Appendix C: Metrics Dashboard

### Recommended Metrics to Track

```cpp
// Add to NetworkConnection (both approaches)
struct ConnectionMetrics {
    std::atomic<uint64_t> totalQueries{0};
    std::atomic<uint64_t> mutexContentions{0};
    std::atomic<uint64_t> callbacksDispatched{0};
    std::atomic<uint64_t> commandsProcessed{0};  // SEDA only

    // Latency histogram (requires external library)
    // histogram<int64_t> queryLatencyNs;

    void recordQuery(int64_t latencyNs) {
        totalQueries.fetch_add(1);
        // queryLatencyNs.record(latencyNs);
    }

    nlohmann::json toJson() const {
        return {
            {"total_queries", totalQueries.load()},
            {"mutex_contentions", mutexContentions.load()},
            {"contention_percent", getContentionPercent()},
            {"callbacks_dispatched", callbacksDispatched.load()},
            {"commands_processed", commandsProcessed.load()}
        };
    }

    double getContentionPercent() const {
        uint64_t total = totalQueries.load();
        return total > 0 ? (100.0 * mutexContentions / total) : 0.0;
    }
};
```

### HTTP Debug Endpoint

```cpp
// Add to HTTP server
svr.Get("/debug/connection/:uuid/metrics", [](const auto& req, auto& res) {
    auto uuid = req.path_params.at("uuid");
    auto* conn = meshManager.getConnection(uuid);

    if (!conn) {
        res.status = 404;
        return;
    }

    res.set_content(conn->getMetrics().toJson().dump(), "application/json");
});
```

---

**Document Status**: Ready for Execution
**Recommended Start**: Approach A (Mutex Fixes)
**Decision Point**: Re-evaluate after 1 week of production monitoring
**Next Steps**: Create feature branch and begin Phase A.1
