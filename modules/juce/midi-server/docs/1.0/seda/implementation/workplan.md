# SEDA Implementation Workplan

**Document Version**: 1.1
**Date**: 2025-10-06
**Status**: Ready for Execution
**Related Design**: [design.md](../planning/design.md)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Strategic Decision](#strategic-decision)
3. [Approach A: Mutex Fixes (Recommended Start)](#approach-a-mutex-fixes-recommended-start)
4. [Approach B: Full SEDA Migration](#approach-b-full-seda-migration)
5. [Approach C: Dual-Transport MIDI (Independent)](#approach-c-dual-transport-midi-independent)
6. [Decision Criteria](#decision-criteria)
7. [Integration with Current Phase 6](#integration-with-current-phase-6)
8. [Testing & Validation Strategy](#testing--validation-strategy)
9. [Timeline & Resources](#timeline--resources)
10. [Risk Assessment](#risk-assessment)
11. [Rollback Procedures](#rollback-procedures)

---

## Executive Summary

This workplan provides **three implementation strategies** for improving the MIDI server architecture:

- **Approach A (Mutex Fixes)**: Quick, low-risk fixes to eliminate callback-under-lock and race conditions - **2-4 hours**
- **Approach B (Full SEDA)**: Complete architectural migration to event-driven stages - **8-14 hours**
- **Approach C (Dual-Transport MIDI)**: Separate real-time and non-real-time MIDI into independent transports - **8-12 hours** (**NEW**)

**Recommendation**: Start with Approach A to eliminate immediate deadlock risks. **Implement Approach C independently** to address QoS requirements. Evaluate Approach B based on production metrics.

### Key Metrics

| Metric | Current (Mutex) | After Fixes (A) | Full SEDA (B) | Dual-Transport (C) | Target |
|--------|----------------|----------------|---------------|-------------------|--------|
| Deadlock Risk | Medium | Low | Zero | Low | Zero |
| Query Latency | ~100ns-10ms | ~100ns | ~10ns (atomic) | ~100ns | <1ms |
| MIDI RT Latency | N/A | N/A | N/A | <1ms (UDP) | <1ms |
| MIDI Bulk Latency | N/A | N/A | N/A | <100ms (TCP) | <100ms |
| Burst Handling | Poor | Poor | Poor | 2000 msg/sec | 500+ msg/sec |
| Implementation Time | 0h | 2-4h | 8-14h | 8-12h | Minimize |
| Code Complexity | Low | Low | Medium-High | Medium | Balance |
| Maintainability | Good | Good | Medium | Good | High |

**Key Insight:** **Approach C addresses different concerns than A/B** - it solves MIDI QoS requirements regardless of threading architecture.

---

## Strategic Decision

### Decision Framework

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Decision Tree                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│  Are deadlocks occurring in production?                                     │
│           │                                                                   │
│           ├── NO ──> Start with Approach A (Mutex Fixes)                    │
│           │          Monitor for 1-2 weeks                                   │
│           │          Collect metrics                                         │
│           │                                                                   │
│           └── YES ──> Skip to Approach B (Full SEDA)                        │
│                       High priority migration                                │
│                                                                               │
│  After Approach A:                                                           │
│  ├── Metrics show <1% contention ──> DONE (but consider C)                 │
│  ├── Metrics show >5% contention ──> Proceed to B                           │
│  └── Deadlocks still occur ──> Emergency B                                  │
│                                                                               │
│  Do you need real-time MIDI performance?                                    │
│           │                                                                   │
│           ├── YES ──> Implement Approach C (Dual-Transport)                 │
│           │          - Burst handling (>500 msg/sec)                         │
│           │          - SysEx reliability (100%)                              │
│           │          - Latency requirements differ by message type           │
│           │                                                                   │
│           └── NO ──> Single transport sufficient                             │
│                                                                               │
│  Approach C is INDEPENDENT - can run alongside A or B                        │
│                                                                               │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Current Status Assessment

Based on the design document analysis:

- **Deadlock Evidence**: Potential but not confirmed in production
- **Contention Evidence**: Not measured
- **Scale Requirements**: Current mesh typically <10 connections
- **Performance Requirements**: HTTP queries must complete <100ms
- **MIDI Requirements**: Need to support burst traffic and reliable SysEx delivery (**NEW**)

**Recommended Path**:
1. **Approach A first** (eliminate immediate risks)
2. **Approach C in parallel** (address MIDI QoS requirements)
3. **Approach B if needed** (based on production metrics)

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
| getState() latency | 120ns | <20ns | ??? | ??? |
| getRemoteDevices() latency| 150ns | <1ms | ??? | ??? |
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

## Approach C: Dual-Transport MIDI (Independent)

### Overview

**NEW APPROACH**: Separate real-time and non-real-time MIDI into independent transport paths with purpose-built QoS.

**Key Insight**: This approach addresses **different requirements** than Approaches A and B:
- Approaches A/B solve **threading/mutex issues**
- Approach C solves **MIDI quality-of-service requirements**

**Therefore: Approach C can be implemented INDEPENDENTLY** of whether you choose mutex fixes (A) or full SEDA (B).

**Effort Estimate**: 8-12 hours
**Risk Level**: Medium
**Rollback Complexity**: Moderate (feature branch required)
**Prerequisites**: None (can be done alongside A or B)

---

### When to Implement Approach C

**Implement dual-transport if:**

- ✅ Need to support burst MIDI traffic (>500 msg/sec)
- ✅ Need reliable SysEx delivery (100% guaranteed)
- ✅ Latency requirements differ by message type (real-time <1ms, bulk <100ms)
- ✅ Planning for real-time performance scenarios (live music, controllers)
- ✅ Network mesh will handle intensive MIDI traffic

**Skip dual-transport if:**

- ❌ Only sending low-frequency MIDI (<50 msg/sec)
- ❌ No SysEx or bulk transfers needed
- ❌ All MIDI can tolerate 100ms+ latency
- ❌ Single-transport simplicity preferred

---

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          MIDI Message Router                                 │
│                                                                               │
│  Input: juce::MidiMessage                                                    │
│  ┌───────────────────────────────────────────────────────────┐              │
│  │  Message Classifier: classifyMidiMessage()                │              │
│  │  - Route by message type (0x80-0xFF)                     │              │
│  └───────────┬────────────────────────────────┬──────────────┘              │
│              │                                 │                              │
│              │ Real-Time                       │ Non-Real-Time                │
│              │ (Note On/Off, CC, Clock)        │ (SysEx, Bulk Transfer)       │
│              v                                 v                              │
│   ┌──────────────────────┐         ┌──────────────────────┐                 │
│   │  REAL-TIME STAGE     │         │ NON-REAL-TIME STAGE  │                 │
│   │  (Lock-Free)         │         │ (Standard Queue)     │                 │
│   ├──────────────────────┤         ├──────────────────────┤                 │
│   │ juce::AbstractFifo   │         │ std::mutex +         │                 │
│   │ Ring Buffer          │         │ std::deque           │                 │
│   │                      │         │                      │                 │
│   │ Capacity: 2048 msgs  │         │ Unbounded capacity   │                 │
│   │ Policy: Drop oldest  │         │ Policy: Block/Retry  │                 │
│   │                      │         │                      │                 │
│   │ Thread Priority:     │         │ Thread Priority:     │                 │
│   │ realtimeAudio (9)    │         │ normal (5)           │                 │
│   └──────────┬───────────┘         └──────────┬───────────┘                 │
│              │                                 │                              │
│              │ UDP Packets                     │ TCP Stream                   │
│              │ (Best-effort)                   │ (Reliable)                   │
│              v                                 v                              │
│   ┌──────────────────────┐         ┌──────────────────────┐                 │
│   │  UDP I/O THREAD      │         │  TCP I/O THREAD      │                 │
│   │  (Non-blocking)      │         │  (Blocking OK)       │                 │
│   └──────────────────────┘         └──────────────────────┘                 │
│              │                                 │                              │
│              v                                 v                              │
│        UDP Socket (port 5004)           TCP Socket (port 5005)               │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

### Phase C.1: Implement Real-Time Ring Buffer

**Objective**: Create lock-free ring buffer for real-time MIDI messages.

**Duration**: 2-3 hours

#### Tasks

1. **Create RealtimeMidiBuffer class**
   - New file: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/transport/RealtimeMidiBuffer.h`

```cpp
#pragma once

#include <juce_core/juce_core.h>
#include <atomic>

namespace NetworkMidi {

/**
 * Lock-free ring buffer for real-time MIDI messages.
 *
 * Features:
 * - Single producer, single consumer (lock-free)
 * - Fixed capacity with drop-oldest overflow policy
 * - ~50ns write latency, ~200ns batch read latency
 * - Thread-safe without mutexes
 */
class RealtimeMidiBuffer {
public:
    static constexpr int CAPACITY = 2048;  // Power of 2

    struct MidiPacket {
        uint8_t data[4];     // Max 4 bytes for channel voice messages
        uint8_t length;
        uint16_t deviceId;
        uint32_t timestamp;  // Microseconds since epoch
    };

    /**
     * Write message to buffer (called from MIDI input thread).
     *
     * @param packet MIDI message to enqueue
     * @return true if written, false if buffer full (message dropped)
     */
    bool write(const MidiPacket& packet);

    /**
     * Read batch of messages (called from UDP sender thread).
     *
     * @param dest Destination array (caller-owned)
     * @param maxCount Maximum messages to read
     * @return Number of messages actually read
     */
    int readBatch(MidiPacket* dest, int maxCount);

    /**
     * Get buffer statistics (lock-free).
     */
    struct Stats {
        int numReady;        // Messages currently in buffer
        int freeSpace;       // Available capacity
        uint64_t dropped;    // Total messages dropped
        uint64_t written;    // Total messages written
        uint64_t read;       // Total messages read
        float dropRate;      // Percentage of messages dropped
    };

    Stats getStats() const;

private:
    juce::AbstractFifo fifo{CAPACITY};
    MidiPacket buffer[CAPACITY];

    std::atomic<uint64_t> droppedCount{0};
    std::atomic<uint64_t> totalWritten{0};
    std::atomic<uint64_t> totalRead{0};
};

} // namespace NetworkMidi
```

2. **Implement write() with drop-oldest policy**
   - File: `RealtimeMidiBuffer.cpp`

```cpp
bool RealtimeMidiBuffer::write(const MidiPacket& packet) {
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 == 0) {
        // Buffer full - implement drop-oldest policy
        droppedCount.fetch_add(1, std::memory_order_relaxed);

        // Force-advance read pointer to make space
        int readStart, readSize, dummy1, dummy2;
        fifo.prepareToRead(1, readStart, readSize, dummy1, dummy2);
        if (readSize > 0) {
            fifo.finishedRead(1);  // Discard oldest
        }

        // Try again
        fifo.prepareToWrite(1, start1, size1, start2, size2);
        if (size1 == 0) {
            return false;  // Still can't write (shouldn't happen)
        }
    }

    buffer[start1] = packet;
    fifo.finishedWrite(1);
    totalWritten.fetch_add(1, std::memory_order_relaxed);
    return true;
}
```

3. **Implement readBatch() for efficient UDP batching**

```cpp
int RealtimeMidiBuffer::readBatch(MidiPacket* dest, int maxCount) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(maxCount, start1, size1, start2, size2);

    int totalRead = 0;

    // Copy first contiguous block
    for (int i = 0; i < size1; ++i) {
        dest[totalRead++] = buffer[start1 + i];
    }

    // Copy second block if ring wraps around
    for (int i = 0; i < size2; ++i) {
        dest[totalRead++] = buffer[start2 + i];
    }

    if (totalRead > 0) {
        fifo.finishedRead(totalRead);
        this->totalRead.fetch_add(totalRead, std::memory_order_relaxed);
    }

    return totalRead;
}
```

4. **Unit tests**

```cpp
TEST(RealtimeMidiBuffer, WriteAndRead) {
    RealtimeMidiBuffer buffer;
    RealtimeMidiBuffer::MidiPacket packet;
    packet.data[0] = 0x90;  // Note On
    packet.data[1] = 60;    // Middle C
    packet.data[2] = 100;   // Velocity
    packet.length = 3;

    EXPECT_TRUE(buffer.write(packet));

    RealtimeMidiBuffer::MidiPacket readPackets[1];
    int count = buffer.readBatch(readPackets, 1);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(readPackets[0].data[0], 0x90);
}

TEST(RealtimeMidiBuffer, BurstHandling) {
    RealtimeMidiBuffer buffer;

    // Write 2000 messages (burst scenario)
    for (int i = 0; i < 2000; ++i) {
        RealtimeMidiBuffer::MidiPacket packet;
        packet.data[0] = 0x90;
        packet.data[1] = 60 + (i % 12);
        packet.data[2] = 100;
        packet.length = 3;
        buffer.write(packet);
    }

    auto stats = buffer.getStats();
    // Should handle 2000 messages with <1% drops
    EXPECT_LT(stats.dropRate, 1.0f);
}
```

#### Acceptance Criteria

- [ ] RealtimeMidiBuffer compiles successfully
- [ ] write() handles overflow with drop-oldest policy
- [ ] readBatch() efficiently reads multiple messages
- [ ] Burst handling test passes (2000 msg/sec)
- [ ] ThreadSanitizer clean

---

### Phase C.2: Create UDP Real-Time Transport

**Objective**: Implement UDP transport thread for real-time MIDI.

**Duration**: 2-3 hours

#### Tasks

1. **Create RealtimeMidiTransport class**
   - New file: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/transport/UdpMidiTransport.h`

```cpp
#pragma once

#include "RealtimeMidiBuffer.h"
#include <juce_core/juce_core.h>

namespace NetworkMidi {

/**
 * Real-time MIDI UDP transport thread.
 *
 * Priority: realtimeAudio (highest user-space priority)
 * Latency target: <1ms end-to-end
 * Delivery: Best-effort (no retries)
 */
class RealtimeMidiTransport : public juce::Thread {
public:
    RealtimeMidiTransport(RealtimeMidiBuffer& buf, juce::String host, int port);
    ~RealtimeMidiTransport() override;

    void run() override;

    struct Stats {
        std::atomic<uint64_t> packetsSent{0};
        std::atomic<uint64_t> packetsReceived{0};
        std::atomic<uint64_t> sendFailures{0};
    };

    Stats getStats() const { return stats; }

private:
    RealtimeMidiBuffer& buffer;
    juce::DatagramSocket udpSocket;
    juce::String remoteHost;
    int remotePort;
    Stats stats;

    void sendPacket(const RealtimeMidiBuffer::MidiPacket& packet);
    void receivePackets();
};

} // namespace NetworkMidi
```

2. **Implement UDP send/receive loop**
   - File: `UdpMidiTransport.cpp`

```cpp
RealtimeMidiTransport::RealtimeMidiTransport(RealtimeMidiBuffer& buf,
                                              juce::String host, int port)
    : juce::Thread("RealtimeMidiUDP"), buffer(buf),
      remoteHost(host), remotePort(port)
{
    // Set real-time priority
    setPriority(9);  // realtimeAudio priority
}

void RealtimeMidiTransport::run() {
    // Bind socket (non-blocking)
    if (!udpSocket.bindToPort(0)) {  // Let OS assign port
        DBG("Failed to bind UDP socket");
        return;
    }

    RealtimeMidiBuffer::MidiPacket packets[32];  // Batch buffer

    while (!threadShouldExit()) {
        // Read batch from ring buffer (lock-free)
        int count = buffer.readBatch(packets, 32);

        if (count > 0) {
            // Send each packet via UDP (non-blocking)
            for (int i = 0; i < count; ++i) {
                sendPacket(packets[i]);
            }
        } else {
            // No messages - yield CPU briefly
            juce::Thread::sleep(1);  // 1ms sleep (acceptable latency)
        }

        // Also receive incoming UDP messages (non-blocking)
        receivePackets();
    }
}

void RealtimeMidiTransport::sendPacket(const RealtimeMidiBuffer::MidiPacket& packet) {
    // Serialize packet (fixed-size binary format)
    uint8_t wireFormat[16];
    wireFormat[0] = 'M';  // Magic byte
    wireFormat[1] = 'R';  // Real-time marker
    wireFormat[2] = packet.length;
    wireFormat[3] = (packet.deviceId >> 8) & 0xFF;
    wireFormat[4] = packet.deviceId & 0xFF;
    std::memcpy(&wireFormat[5], &packet.timestamp, 4);
    std::memcpy(&wireFormat[9], packet.data, packet.length);

    int totalSize = 9 + packet.length;

    // Send UDP (best-effort, no retry)
    int sent = udpSocket.write(remoteHost, remotePort, wireFormat, totalSize);
    if (sent == totalSize) {
        stats.packetsSent.fetch_add(1, std::memory_order_relaxed);
    } else {
        stats.sendFailures.fetch_add(1, std::memory_order_relaxed);
    }
}
```

3. **Unit tests**

```cpp
TEST(RealtimeMidiTransport, SendReceive) {
    RealtimeMidiBuffer buffer;
    RealtimeMidiTransport transport(buffer, "127.0.0.1", 5004);

    transport.startThread();

    // Write messages to buffer
    for (int i = 0; i < 100; ++i) {
        RealtimeMidiBuffer::MidiPacket packet;
        packet.data[0] = 0x90;
        packet.data[1] = 60;
        packet.data[2] = 100;
        packet.length = 3;
        buffer.write(packet);
    }

    // Wait for transmission
    juce::Thread::sleep(100);

    auto stats = transport.getStats();
    EXPECT_GT(stats.packetsSent, 90);  // At least 90% sent
}
```

#### Acceptance Criteria

- [ ] UDP transport thread starts successfully
- [ ] Real-time priority set (priority 9)
- [ ] Non-blocking send/receive implemented
- [ ] Latency <1ms (measure with timestamps)
- [ ] Unit tests pass

---

### Phase C.3: Create TCP Non-Real-Time Transport

**Objective**: Implement TCP transport for reliable SysEx delivery.

**Duration**: 2-3 hours

#### Tasks

1. **Create NonRealtimeMidiTransport class**
   - New file: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/transport/TcpMidiTransport.h`

```cpp
#pragma once

#include <juce_core/juce_core.h>
#include <deque>
#include <mutex>
#include <map>

namespace NetworkMidi {

/**
 * Non-real-time MIDI transport using TCP for reliable delivery.
 *
 * Features:
 * - Guaranteed delivery (ACK/retry)
 * - SysEx fragmentation (1KB chunks)
 * - Flow control
 * - 100% reliability target
 */
class NonRealtimeMidiTransport : public juce::Thread {
public:
    struct MidiPacket {
        std::vector<uint8_t> data;
        uint16_t deviceId;
        uint32_t sequenceNumber;
        bool requiresAck;
    };

    NonRealtimeMidiTransport(juce::String host, int port);
    ~NonRealtimeMidiTransport() override;

    void sendMessage(const juce::MidiMessage& msg, uint16_t deviceId);
    std::vector<MidiPacket> getReceivedMessages();

    void run() override;

private:
    std::mutex queueMutex;
    std::deque<MidiPacket> sendQueue;
    std::deque<MidiPacket> receiveQueue;
    juce::WaitableEvent dataAvailable;

    juce::StreamingSocket tcpSocket;
    juce::String remoteHost;
    int remotePort;
    bool connected{false};

    uint32_t nextSequenceNumber{0};
    std::map<uint32_t, MidiPacket> pendingAcks;

    void attemptConnection();
    void processSendQueue();
    void sendTcpPacket(const MidiPacket& packet);
    void receiveData();
    void sendAck(uint32_t seqNum);
    void retryUnacknowledged();
};

} // namespace NetworkMidi
```

2. **Implement SysEx fragmentation**
   - File: `TcpMidiTransport.cpp`

```cpp
void NonRealtimeMidiTransport::sendTcpPacket(const MidiPacket& packet) {
    // Fragment if SysEx is too large (>1KB chunks)
    const size_t MAX_FRAGMENT = 1024;
    size_t offset = 0;

    while (offset < packet.data.size()) {
        size_t fragmentSize = std::min(MAX_FRAGMENT, packet.data.size() - offset);

        // Build TCP frame: [header][fragment]
        std::vector<uint8_t> frame;
        frame.push_back('M');  // Magic
        frame.push_back('N');  // Non-real-time
        frame.push_back((packet.sequenceNumber >> 24) & 0xFF);
        frame.push_back((packet.sequenceNumber >> 16) & 0xFF);
        frame.push_back((packet.sequenceNumber >> 8) & 0xFF);
        frame.push_back(packet.sequenceNumber & 0xFF);
        frame.push_back((fragmentSize >> 8) & 0xFF);
        frame.push_back(fragmentSize & 0xFF);
        frame.insert(frame.end(), packet.data.begin() + offset,
                    packet.data.begin() + offset + fragmentSize);

        int sent = tcpSocket.write(frame.data(), (int)frame.size());
        if (sent != (int)frame.size()) {
            connected = false;
            return;
        }

        offset += fragmentSize;
    }
}
```

3. **Implement ACK/retry mechanism**

```cpp
void NonRealtimeMidiTransport::retryUnacknowledged() {
    auto now = juce::Time::getCurrentTime();
    std::vector<uint32_t> toRetry;

    for (auto& [seqNum, pending] : pendingAcks) {
        auto elapsed = now.toMilliseconds() - pending.sentTime.toMilliseconds();

        if (elapsed > 1000 && pending.retryCount < 3) {  // 1s timeout, 3 retries
            toRetry.push_back(seqNum);
        } else if (pending.retryCount >= 3) {
            // Give up - log error
            DBG("Failed to deliver packet " << seqNum << " after 3 retries");
            pendingAcks.erase(seqNum);
        }
    }

    for (uint32_t seqNum : toRetry) {
        auto& pending = pendingAcks[seqNum];
        pending.retryCount++;
        pending.sentTime = juce::Time::getCurrentTime();
        sendTcpPacket(pending.packet);
    }
}
```

4. **Unit tests**

```cpp
TEST(NonRealtimeMidiTransport, SysExReliability) {
    NonRealtimeMidiTransport transport("127.0.0.1", 5005);
    transport.startThread();

    // Send 100 large SysEx messages
    for (int i = 0; i < 100; ++i) {
        std::vector<uint8_t> sysex(5000);
        sysex[0] = 0xF0;  // SysEx start
        for (size_t j = 1; j < sysex.size() - 1; ++j) {
            sysex[j] = rand() % 128;
        }
        sysex.back() = 0xF7;  // SysEx end

        juce::MidiMessage msg(sysex.data(), (int)sysex.size());
        transport.sendMessage(msg, 0);
    }

    // Wait for delivery
    juce::Thread::sleep(10000);

    // Verify 100% delivery
    auto received = transport.getReceivedMessages();
    EXPECT_EQ(received.size(), 100);
}
```

#### Acceptance Criteria

- [ ] TCP transport thread starts successfully
- [ ] SysEx fragmentation works (1KB chunks)
- [ ] ACK/retry mechanism implemented
- [ ] 100% delivery in reliability test
- [ ] Latency <100ms for large SysEx

---

### Phase C.4: Message Classification and Routing

**Objective**: Route MIDI messages to appropriate transport based on type.

**Duration**: 1-2 hours

#### Tasks

1. **Create message classifier**
   - New file: `/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/transport/MidiClassifier.h`

```cpp
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace NetworkMidi {

enum class MidiMessageClass {
    RealTime,      // Needs UDP transport
    NonRealTime    // Needs TCP transport
};

/**
 * Classify MIDI message by QoS requirements.
 *
 * Real-Time (UDP):
 * - Note On/Off, CC, Pitch Bend (0x80-0xEF)
 * - MIDI Clock, Start, Stop (0xF8-0xFF)
 * - Active Sensing
 *
 * Non-Real-Time (TCP):
 * - System Exclusive (0xF0...0xF7)
 * - Bulk transfers
 */
inline MidiMessageClass classifyMidiMessage(const juce::MidiMessage& msg) {
    // System Real-Time messages (0xF8 - 0xFF)
    if (msg.getRawData()[0] >= 0xF8) {
        return MidiMessageClass::RealTime;
    }

    // System Exclusive (0xF0 ... 0xF7)
    if (msg.isSysEx()) {
        return MidiMessageClass::NonRealTime;
    }

    // Channel Voice messages (0x80 - 0xEF)
    if (msg.getRawData()[0] >= 0x80 && msg.getRawData()[0] < 0xF0) {
        return MidiMessageClass::RealTime;
    }

    // Default to non-real-time for safety
    return MidiMessageClass::NonRealTime;
}

} // namespace NetworkMidi
```

2. **Update MIDI input callback to route messages**

```cpp
void MidiInputCallback::handleIncomingMidiMessage(juce::MidiInput* source,
                                                   const juce::MidiMessage& msg) {
    auto messageClass = classifyMidiMessage(msg);

    if (messageClass == MidiMessageClass::RealTime) {
        // Route to real-time buffer (lock-free)
        RealtimeMidiBuffer::MidiPacket packet;
        packet.data[0] = msg.getRawData()[0];
        packet.data[1] = msg.getRawData()[1];
        packet.data[2] = msg.getRawData()[2];
        packet.length = std::min(msg.getRawDataSize(), 4);
        packet.deviceId = 0;
        packet.timestamp = juce::Time::getMillisecondCounterHiRes();

        bool success = realtimeBuffer.write(packet);
        if (!success) {
            inputDrops.fetch_add(1, std::memory_order_relaxed);
        }
    } else {
        // Route to non-real-time transport (TCP)
        nonRealtimeTransport.sendMessage(msg, 0);
    }
}
```

3. **Unit tests**

```cpp
TEST(MidiClassifier, ClassifyNoteOn) {
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 100.0f);
    EXPECT_EQ(classifyMidiMessage(noteOn), MidiMessageClass::RealTime);
}

TEST(MidiClassifier, ClassifySysEx) {
    uint8_t sysex[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
    juce::MidiMessage msg(sysex, 5);
    EXPECT_EQ(classifyMidiMessage(msg), MidiMessageClass::NonRealTime);
}

TEST(MidiClassifier, ClassifyMidiClock) {
    juce::MidiMessage clock(0xF8);
    EXPECT_EQ(classifyMidiMessage(clock), MidiMessageClass::RealTime);
}
```

#### Acceptance Criteria

- [ ] classifyMidiMessage() correctly identifies all message types
- [ ] MIDI routing works (real-time to UDP, non-real-time to TCP)
- [ ] Unit tests pass
- [ ] No classification errors in manual testing

---

### Phase C.5: Testing and Validation

**Objective**: Validate dual-transport performance and reliability.

**Duration**: 2-3 hours

#### Tasks

1. **Burst handling test**

```cpp
TEST(DualTransport, BurstHandling_2000MsgPerSec) {
    RealtimeMidiBuffer buffer;
    RealtimeMidiTransport transport(buffer, "127.0.0.1", 5004);
    transport.startThread();

    // Simulate 2000 msg/sec burst for 1 second
    for (int i = 0; i < 2000; ++i) {
        RealtimeMidiBuffer::MidiPacket packet;
        packet.data[0] = 0x90;
        packet.data[1] = 60 + (i % 12);
        packet.data[2] = 100;
        packet.length = 3;
        buffer.write(packet);

        // 500µs between messages = 2000 msg/sec
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    auto stats = buffer.getStats();
    EXPECT_LT(stats.dropRate, 1.0f);  // <1% drops
}
```

2. **SysEx reliability test**

```cpp
TEST(DualTransport, SysExReliability_100Percent) {
    NonRealtimeMidiTransport transport("127.0.0.1", 5005);
    transport.startThread();

    // Send 100 SysEx messages
    for (int i = 0; i < 100; ++i) {
        std::vector<uint8_t> sysex(1000);
        sysex[0] = 0xF0;
        for (size_t j = 1; j < sysex.size() - 1; ++j) {
            sysex[j] = (i * 10 + j) % 128;
        }
        sysex.back() = 0xF7;

        juce::MidiMessage msg(sysex.data(), (int)sysex.size());
        transport.sendMessage(msg, 0);
    }

    juce::Thread::sleep(5000);

    auto received = transport.getReceivedMessages();
    EXPECT_EQ(received.size(), 100);  // 100% delivery
}
```

3. **Latency measurement test**

```cpp
TEST(DualTransport, LatencyMeasurement) {
    RealtimeMidiBuffer buffer;
    RealtimeMidiTransport transport(buffer, "127.0.0.1", 5004);
    transport.startThread();

    // Measure round-trip latency
    auto start = juce::Time::getMillisecondCounterHiRes();

    RealtimeMidiBuffer::MidiPacket packet;
    packet.data[0] = 0x90;
    packet.data[1] = 60;
    packet.data[2] = 100;
    packet.length = 3;
    packet.timestamp = start;
    buffer.write(packet);

    // Wait for response (implementation-specific)
    juce::Thread::sleep(10);

    auto elapsed = juce::Time::getMillisecondCounterHiRes() - start;
    EXPECT_LT(elapsed, 1.0);  // <1ms latency
}
```

4. **Create metrics endpoint**

```cpp
// Add to HTTP server
svr.Get("/metrics", [](const auto& req, auto& res) {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();

    auto rtStats = realtimeBuffer.getStats();
    obj->setProperty("realtime_messages_sent", (int)rtStats.written);
    obj->setProperty("realtime_messages_dropped", (int)rtStats.dropped);
    obj->setProperty("realtime_drop_rate", (double)rtStats.dropRate);

    auto nrtStats = nonRealtimeTransport.getStats();
    obj->setProperty("nonrealtime_messages_sent", (int)nrtStats.sent);
    obj->setProperty("nonrealtime_retries", (int)nrtStats.retries);

    res.set_content(juce::JSON::toString(juce::var(obj)), "application/json");
});
```

#### Acceptance Criteria

- [ ] Burst handling: <1% drops at 2000 msg/sec
- [ ] SysEx reliability: 100% delivery
- [ ] Real-time latency: <1ms
- [ ] Non-real-time latency: <100ms
- [ ] Metrics endpoint functional

---

### Integration with Approaches A and B

#### Integration with Approach A (Mutex Fixes)

**Scenario**: Dual-transport + mutex fixes

**Compatibility**: ✅ **Fully compatible**

- Dual-transport uses its own lock-free buffer (real-time path)
- TCP path can use mutexes safely (non-real-time)
- No conflicts with mutex fixes in NetworkConnection

**Implementation notes**:
- Real-time buffer is lock-free (no mutex issues)
- TCP queue uses mutex but only for low-frequency SysEx
- Message routing layer sits above NetworkConnection

#### Integration with Approach B (Full SEDA)

**Scenario**: Dual-transport + SEDA architecture

**Compatibility**: ✅ **Natural fit**

- Real-time MIDI becomes Stage 3A (UDP worker)
- Non-real-time MIDI becomes Stage 3B (TCP worker)
- Both integrate cleanly with SEDA command queue pattern

**Implementation notes**:
- Real-time buffer can be part of SEDA Stage 3A
- Non-real-time transport can be SEDA Stage 3B
- Classification layer routes messages to appropriate stage

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

#### Choose Approach C (Dual-Transport) if:

- ✅ Need burst MIDI handling (>500 msg/sec)
- ✅ Need reliable SysEx delivery (100%)
- ✅ Latency requirements differ by message type
- ✅ Planning for real-time performance scenarios
- ✅ Network mesh will carry intensive MIDI traffic

**Key Insight**: Approach C is **orthogonal** to A and B - you can implement dual-transport with either mutex fixes or SEDA.

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
- **UDP transport for MIDI messages** (partially implemented)

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

**Approach C Impact**:
- MeshManager routes MIDI messages through classifier
- Needs to track dual-transport statistics
- May add `/metrics/midi` endpoint

#### 2. HeartbeatMonitor Integration

**Current State**: HeartbeatMonitor timer calls `getTimeSinceLastHeartbeat()` every 1 second.

**Approach A Impact**:
- Minor: Callback dispatch becomes async
- Test update needed to poll for state changes

**Approach B Impact**:
- Significant performance improvement (atomic read vs mutex)
- Must post CheckHeartbeatCommand instead of direct setState()

**Approach C Impact**:
- No direct impact (heartbeats can use existing mechanism)
- Could optionally use UDP real-time path for heartbeat packets

**Required Change for Approach B**:

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

**Approach C Impact**:
- Add `/metrics/midi` endpoint for transport statistics
- Show real-time vs non-real-time message counts
- Display drop rates and latency histograms

**No changes needed for A/B** - public API remains identical.

#### 4. UDP Transport Integration (Phase 6)

**Current State**: UDP transport partially implemented for MIDI messages.

**Approach C Integration**:
- Existing UDP transport becomes **real-time MIDI path**
- Add TCP transport as **non-real-time path**
- Message classifier routes between the two
- Minimal changes to existing UDP code

**Migration path**:
1. Wrap existing UDP code in `RealtimeMidiTransport`
2. Add `RealtimeMidiBuffer` in front of UDP sender
3. Create parallel `NonRealtimeMidiTransport` for TCP
4. Add `classifyMidiMessage()` router

---

## Testing & Validation Strategy

### Test Matrix

| Test Category | Approach A | Approach B | Approach C | Purpose |
|--------------|------------|------------|------------|---------|
| Unit Tests | Update callbacks | Update callbacks + async | MIDI routing | Verify correctness |
| Integration Tests | Minor updates | Moderate updates | Dual-transport integration | Verify subsystem interaction |
| Concurrency Tests | New: 100 threads | New: stress test | Burst handling | Verify no deadlocks / overflow |
| Performance Tests | Baseline only | Full benchmarks | Latency measurement | Verify improvements |
| Regression Tests | Full suite | Full suite | Full suite | Verify no breakage |
| TSAN Tests | Required | Required | Required | Verify no races |

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

#### Scenario 4: MIDI Burst Testing (NEW - Approach C)

```cpp
TEST(DualTransport, MIDIBurstTest_2000MsgPerSec) {
    RealtimeMidiBuffer buffer;
    RealtimeMidiTransport transport(buffer, "127.0.0.1", 5004);
    transport.startThread();

    // Simulate intense performance (2000 msg/sec for 5 seconds)
    for (int second = 0; second < 5; ++second) {
        for (int i = 0; i < 2000; ++i) {
            RealtimeMidiBuffer::MidiPacket packet;
            packet.data[0] = 0x90;  // Note On
            packet.data[1] = 60 + (i % 12);
            packet.data[2] = 100;
            packet.length = 3;
            buffer.write(packet);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    }

    auto stats = buffer.getStats();
    EXPECT_LT(stats.dropRate, 1.0f);  // <1% drops over 5 seconds
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

### Approach C: Dual-Transport Timeline (NEW)

| Phase | Duration | Developer Hours | Dependencies |
|-------|----------|----------------|--------------|
| C.1: Real-time ring buffer | 2-3 hours | 2.5h | None |
| C.2: UDP real-time transport | 2-3 hours | 2.5h | C.1 |
| C.3: TCP non-real-time transport | 2-3 hours | 2.5h | None (parallel) |
| C.4: Message classification | 1-2 hours | 1.5h | C.2, C.3 |
| C.5: Testing & validation | 2-3 hours | 2.5h | All previous |
| **Total** | **9-14 hours** | **11.5h** | - |

**Critical Path**: C.1 → C.2 → C.4 → C.5 (C.3 can be parallel with C.1/C.2)
**Parallelization**: C.3 independent of C.1/C.2
**Calendar Time**: 2 days (with buffer for testing)

### Combined Timeline: A + C (Recommended)

| Phase | Duration | Developer Hours | Dependencies |
|-------|----------|----------------|--------------|
| **Week 1** | | | |
| A.1-A.5: Mutex fixes | 3-4 hours | 4.25h | None |
| **Week 2** | | | |
| C.1-C.2: Real-time path | 4-6 hours | 5h | None |
| C.3: Non-real-time path | 2-3 hours | 2.5h | Parallel |
| **Week 3** | | | |
| C.4-C.5: Integration & testing | 3-5 hours | 4h | C.1-C.3 |
| **Total** | **12-18 hours** | **15.75h** | - |

**Calendar Time**: 3 weeks (relaxed schedule with testing)

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

#### Approach C (NEW)

- **Developers**: 1 senior C++ developer
- **Skills Required**:
  - Lock-free data structures (juce::AbstractFifo)
  - Network programming (UDP/TCP)
  - MIDI protocol knowledge
  - Real-time systems experience
- **Infrastructure**:
  - Development machine with network access
  - MIDI hardware for testing (optional)
  - Packet capture tools (Wireshark)
  - Latency measurement tools

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

### Approach C Risks (NEW)

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Ring buffer sizing inadequate for bursts | Low | Medium | Monitor drop rates, increase capacity if needed |
| UDP packet loss in network | Medium | Low | Expected - monitor drop rates, acceptable for real-time |
| TCP connection failures | Medium | Medium | Retry logic (3 retries, 1s timeout) |
| Message classification errors | Low | High | Comprehensive unit tests, manual validation |
| Real-time priority not available on platform | Low | Medium | Graceful fallback to normal priority |
| Latency targets not met | Medium | High | Benchmark early, optimize UDP path |
| SysEx fragmentation bugs | Medium | Medium | Test with large SysEx messages (10KB+) |

**Overall Risk**: Medium

### Mitigation Strategies

#### For All Approaches

1. **Feature Branch Development**
   - Create `feature/mutex-fixes`, `feature/seda-migration`, or `feature/dual-transport` branch
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

### Approach C Rollback (NEW)

**Complexity**: Moderate (independent feature)

#### Option 1: Feature Flag Disable

```cpp
// Add feature flag in configuration
static constexpr bool ENABLE_DUAL_TRANSPORT = false;

if (ENABLE_DUAL_TRANSPORT) {
    // Use dual-transport
    auto msgClass = classifyMidiMessage(msg);
    // ...
} else {
    // Use original single transport
    sendViaSingleTransport(msg);
}
```

**Recovery Time**: <5 minutes (config change)

#### Option 2: Git Revert

```bash
git revert <dual-transport-merge-commit>
git push origin main
```

**Recovery Time**: 15-30 minutes

**Data Loss**: None (transport layer only)

### Rollback Decision Criteria

| Severity | Symptom | Action | Timeline |
|----------|---------|--------|----------|
| P0 | Production deadlocks | Immediate rollback | <15 min |
| P1 | Crashes in <1% of requests | Rollback within 1 hour | <1 hour |
| P2 | Performance regression >20% | Rollback within 4 hours | <4 hours |
| P3 | Minor test failures | Forward fix | <1 day |

### Rollback Testing

Before deploying any approach, verify rollback procedure:

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

# Monitor MIDI transport metrics (Approach C)
curl http://localhost:8080/metrics/midi | jq '.realtime_drop_rate'
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

### Approach C Checklist (NEW)

- [ ] Phase C.1: Real-time ring buffer
  - [ ] RealtimeMidiBuffer.h created
  - [ ] write() with drop-oldest policy implemented
  - [ ] readBatch() for efficient reading
  - [ ] getStats() for monitoring
  - [ ] Unit tests pass
  - [ ] Burst handling test passes (2000 msg/sec)
- [ ] Phase C.2: UDP real-time transport
  - [ ] RealtimeMidiTransport thread created
  - [ ] Real-time priority set (9)
  - [ ] Non-blocking UDP send/receive
  - [ ] Packet serialization implemented
  - [ ] Latency <1ms verified
- [ ] Phase C.3: TCP non-real-time transport
  - [ ] NonRealtimeMidiTransport thread created
  - [ ] TCP connection management
  - [ ] SysEx fragmentation (1KB chunks)
  - [ ] SysEx reassembly with sequence numbers
  - [ ] ACK/retry mechanism (1s timeout, 3 retries)
  - [ ] 100% delivery verified
- [ ] Phase C.4: Message classification
  - [ ] classifyMidiMessage() implemented
  - [ ] Message routing works correctly
  - [ ] Unit tests for all MIDI message types
- [ ] Phase C.5: Testing & validation
  - [ ] Burst handling test passes (<1% drops)
  - [ ] SysEx reliability test passes (100%)
  - [ ] Latency tests pass (RT <1ms, NRT <100ms)
  - [ ] /metrics/midi endpoint functional
  - [ ] Integration with existing code complete

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

    void recordQuery(int64_t latencyNs) {
        totalQueries.fetch_add(1);
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

### MIDI Transport Metrics (Approach C - NEW)

```cpp
struct MidiTransportMetrics {
    // Real-time path (UDP)
    std::atomic<uint64_t> realtimeMessagesSent{0};
    std::atomic<uint64_t> realtimeMessagesReceived{0};
    std::atomic<uint64_t> realtimeMessagesDropped{0};
    std::atomic<uint64_t> udpSendFailures{0};

    // Non-real-time path (TCP)
    std::atomic<uint64_t> nonRealtimeMessagesSent{0};
    std::atomic<uint64_t> nonRealtimeMessagesReceived{0};
    std::atomic<uint64_t> tcpRetries{0};
    std::atomic<uint64_t> tcpFragmentsSent{0};

    nlohmann::json toJson() const {
        return {
            {"realtime_sent", realtimeMessagesSent.load()},
            {"realtime_received", realtimeMessagesReceived.load()},
            {"realtime_dropped", realtimeMessagesDropped.load()},
            {"realtime_drop_rate", calculateDropRate()},
            {"nonrealtime_sent", nonRealtimeMessagesSent.load()},
            {"nonrealtime_received", nonRealtimeMessagesReceived.load()},
            {"tcp_retries", tcpRetries.load()},
            {"tcp_retry_rate", calculateRetryRate()}
        };
    }

    double calculateDropRate() const {
        uint64_t sent = realtimeMessagesSent.load();
        uint64_t dropped = realtimeMessagesDropped.load();
        return sent > 0 ? (100.0 * dropped / sent) : 0.0;
    }

    double calculateRetryRate() const {
        uint64_t sent = nonRealtimeMessagesSent.load();
        uint64_t retries = tcpRetries.load();
        return sent > 0 ? (100.0 * retries / sent) : 0.0;
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

// Approach C: Add MIDI transport metrics
svr.Get("/metrics/midi", [](const auto& req, auto& res) {
    auto metrics = midiTransportMetrics.toJson();
    res.set_content(metrics.dump(), "application/json");
});
```

---

**Document Status**: Ready for Execution
**Recommended Start**: Approach A (Mutex Fixes) + Approach C (Dual-Transport) in parallel or sequence
**Decision Point**: Re-evaluate Approach B after 1 week of production monitoring
**Next Steps**:
1. Create feature branch for Approach A
2. Create feature branch for Approach C (can be parallel)
3. Begin Phase A.1 and/or Phase C.1
