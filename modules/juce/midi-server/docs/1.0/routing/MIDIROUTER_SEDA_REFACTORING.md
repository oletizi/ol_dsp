# MidiRouter SEDA Refactoring Plan

**Status**: PLANNED
**Priority**: HIGH - Architectural consistency required
**Estimated Effort**: 8-12 hours
**Created**: 2025-10-06

---

## Problem Statement

### Current Architecture Inconsistency

**MidiRouter** (Phase 3) uses traditional mutex-based concurrency:
- 3 mutexes with complex ordering rules (`portMutex` → `messageMutex` → `statsMutex`)
- Risk of deadlocks if ordering violated
- Potential contention on hot paths (statistics updates)
- Does not match system architecture

**NetworkConnection** (Phase B) uses SEDA (Staged Event-Driven Architecture):
- `NetworkConnectionQueue` with command pattern
- Lock-free message passing
- Worker thread processes commands sequentially
- Zero deadlock risk

### Why This Matters

1. **Safety**: Mutex ordering is fragile - one mistake = deadlock
2. **Performance**: Lock-free queues outperform mutexes
3. **Consistency**: System should use one concurrency model
4. **Maintainability**: SEDA is simpler to reason about

### Decision

**Refactor MidiRouter to use SEDA** matching the NetworkConnection pattern.

---

## Current State Analysis

### MidiRouter Mutex Usage

```cpp
class MidiRouter {
private:
    // 3 mutexes with ordering rules
    mutable std::mutex portMutex;        // 1. Protects localPorts
    mutable std::mutex messageMutex;     // 2. Protects messageQueues
    mutable std::mutex statsMutex;       // 3. Protects statistics

    std::map<uint16_t, std::unique_ptr<MidiPortInterface>> localPorts;
    std::map<uint16_t, std::queue<std::vector<uint8_t>>> messageQueues;
    Statistics stats;
};
```

### Critical Methods (Require Locking)

1. **forwardMessage()** - Hot path, needs RouteManager lookup + forwarding
2. **registerLocalPort()** - Infrequent, modifies localPorts
3. **unregisterLocalPort()** - Infrequent, modifies localPorts
4. **queueReceivedMessage()** - Receive path, modifies messageQueues
5. **getStatistics()** - Read-only, needs consistent snapshot
6. **resetStatistics()** - Infrequent, modifies stats

### Performance Hotspots

- `forwardMessage()`: Called for every MIDI message (high frequency)
- `updateRuleStatistics()`: Acquires `statsMutex` inside forwarding loop
- String conversion: `rule.ruleId.toStdString()` allocates on hot path

---

## Target State (SEDA Architecture)

### Command Queue Pattern

```cpp
class MidiRouter {
private:
    NetworkConnectionQueue commandQueue;
    juce::Thread workerThread;
    std::atomic<bool> shouldStop{false};

    // No mutexes needed - all state accessed from worker thread only
    std::map<uint16_t, std::unique_ptr<MidiPortInterface>> localPorts;
    std::map<uint16_t, std::queue<std::vector<uint8_t>>> messageQueues;
    Statistics stats;

    void workerThreadRun();
};
```

### Command Definitions

```cpp
namespace MidiRouterCommands {

// 1. Hot path - message forwarding (async, no response)
struct ForwardMessageCommand : public NetworkConnectionQueue::Command {
    juce::Uuid sourceNode;
    uint16_t sourceDevice;
    std::vector<uint8_t> midiData;

    void execute(MidiRouter* router) override;
};

// 2. Port management (async, no response)
struct RegisterPortCommand : public NetworkConnectionQueue::Command {
    uint16_t deviceId;
    std::unique_ptr<MidiPortInterface> port;

    void execute(MidiRouter* router) override;
};

struct UnregisterPortCommand : public NetworkConnectionQueue::Command {
    uint16_t deviceId;

    void execute(MidiRouter* router) override;
};

// 3. Message queueing (async, no response)
struct QueueMessageCommand : public NetworkConnectionQueue::Command {
    uint16_t deviceId;
    std::vector<uint8_t> midiData;

    void execute(MidiRouter* router) override;
};

// 4. Statistics (sync, with response via callback)
struct GetStatisticsCommand : public NetworkConnectionQueue::QueryCommand<Statistics> {
    void execute(MidiRouter* router) override {
        setResponse(router->getStatisticsInternal());
    }
};

struct ResetStatisticsCommand : public NetworkConnectionQueue::Command {
    void execute(MidiRouter* router) override;
};

// 5. Configuration (async, no response)
struct SetRouteManagerCommand : public NetworkConnectionQueue::Command {
    RouteManager* manager;

    void execute(MidiRouter* router) override;
};

struct SetNetworkTransportCommand : public NetworkConnectionQueue::Command {
    NetworkTransport* transport;

    void execute(MidiRouter* router) override;
};

} // namespace MidiRouterCommands
```

---

## Implementation Plan

### Phase 3.5: SEDA Refactoring (8-12 hours)

#### Step 1: Command Infrastructure (2 hours)

**Task 1.1**: Create command classes
- File: `network/routing/MidiRouterCommands.h` (new)
- Implement all 8 command types
- Inherit from `NetworkConnectionQueue::Command` or `QueryCommand<T>`

**Task 1.2**: Add worker thread to MidiRouter
- Add `NetworkConnectionQueue commandQueue`
- Add worker thread start/stop in constructor/destructor
- Implement `workerThreadRun()` loop

#### Step 2: Public API Refactoring (3 hours)

**Task 2.1**: Convert public methods to command dispatch

**Before** (mutex-based):
```cpp
void MidiRouter::forwardMessage(const juce::Uuid& sourceNode,
                                uint16_t sourceDevice,
                                const std::vector<uint8_t>& midiData)
{
    std::lock_guard<std::mutex> lock(messageMutex);
    // ... forwarding logic
}
```

**After** (SEDA):
```cpp
void MidiRouter::forwardMessage(const juce::Uuid& sourceNode,
                                uint16_t sourceDevice,
                                const std::vector<uint8_t>& midiData)
{
    auto cmd = std::make_unique<ForwardMessageCommand>();
    cmd->sourceNode = sourceNode;
    cmd->sourceDevice = sourceDevice;
    cmd->midiData = midiData;

    commandQueue.push(std::move(cmd));
}
```

**Task 2.2**: Handle synchronous methods with QueryCommand

```cpp
Statistics MidiRouter::getStatistics() const
{
    auto cmd = std::make_unique<GetStatisticsCommand>();
    auto future = cmd->getResponseFuture();

    commandQueue.push(std::move(cmd));

    return future.get(); // Block until worker processes command
}
```

#### Step 3: Remove Mutexes (1 hour)

**Task 3.1**: Delete mutex declarations
- Remove `portMutex`, `messageMutex`, `statsMutex`
- Remove all `std::lock_guard` usage
- Remove mutex ordering documentation (no longer needed!)

**Task 3.2**: Make methods private/internal
- Rename `forwardMessageInternal()` to just internal methods
- Only worker thread calls internal methods
- Public methods just dispatch commands

#### Step 4: Testing (2-3 hours)

**Task 4.1**: Update existing tests
- Tests should still work (same public API)
- May need to add short delays for async commands to process

**Task 4.2**: Add SEDA-specific tests
- Test command queue overflow handling
- Test worker thread shutdown
- Test query command timeout

**Task 4.3**: Verify all 37 tests still pass

#### Step 5: Performance Optimization (2 hours)

**Task 5.1**: Eliminate hot-path allocations
- Pre-allocate command objects (object pool)
- Remove `toStdString()` calls (use juce::String directly)

**Task 5.2**: Tune queue size
- Set appropriate max queue size (1000? 10000?)
- Add queue overflow statistics

#### Step 6: Documentation (1 hour)

**Task 6.1**: Update MidiRouter.h documentation
- Replace mutex ordering docs with SEDA explanation
- Update usage examples to note async behavior
- Document command queue semantics

**Task 6.2**: Update architectural docs
- Note consistency with NetworkConnection
- Explain SEDA benefits

---

## Migration Strategy

### Backward Compatibility

**Public API remains unchanged**:
- All public methods have same signatures
- Callers don't need to change
- Async execution is transparent

**Behavioral changes**:
- Methods return immediately (async execution)
- `getStatistics()` blocks until response (same as before, but via future)
- Small latency increase (~microseconds for queue operation)

### Testing Strategy

1. **Existing tests must pass**: All 37 MidiMessageForwardingTest tests
2. **Add async delay helpers**: `waitForCommandProcessing()` in tests if needed
3. **Stress test**: High message rate forwarding
4. **Regression test**: Ensure no deadlocks (run with TSan)

### Rollout Plan

1. **Development**: Implement on feature branch
2. **Testing**: Verify all tests pass, run stress tests
3. **Integration**: Merge after Phase 3 quality improvements
4. **Monitoring**: Track queue depths, processing latency

---

## Benefits

### Safety
- ✅ **Zero deadlock risk** - no mutexes
- ✅ **Sequential execution** - no race conditions
- ✅ **Simple reasoning** - one thread owns all state

### Performance
- ✅ **Lock-free queues** - better throughput than mutexes
- ✅ **No contention** - single worker thread
- ✅ **Async execution** - callers don't block
- ✅ **Object pooling** - can pre-allocate commands

### Architecture
- ✅ **Consistency** - matches NetworkConnection pattern
- ✅ **Scalability** - easy to add commands
- ✅ **Testability** - commands are independently testable
- ✅ **Maintainability** - simpler concurrency model

---

## Risks & Mitigations

### Risk 1: Async Latency
**Risk**: Command queue adds latency (enqueue + dequeue + execution)
**Mitigation**:
- Lock-free queue is very fast (<1μs)
- Still faster than mutex contention
- MIDI has ~1ms tolerance

### Risk 2: Queue Overflow
**Risk**: High message rate could overflow queue
**Mitigation**:
- Set appropriate queue size (10000 messages)
- Track overflow in statistics
- Apply backpressure if needed

### Risk 3: Query Command Blocking
**Risk**: `getStatistics()` blocks caller until worker processes
**Mitigation**:
- Worker thread is fast (no I/O, no locks)
- Typical latency <10μs
- Same behavior as mutex-based version

### Risk 4: Test Flakiness
**Risk**: Async execution could cause test timing issues
**Mitigation**:
- Add `processAllPendingCommands()` test helper
- Use command completion callbacks in tests
- Increase timeouts if needed

---

## Success Criteria

1. ✅ All 37 MidiMessageForwardingTest tests pass
2. ✅ Zero mutexes in MidiRouter
3. ✅ All public methods dispatch to command queue
4. ✅ Worker thread processes commands sequentially
5. ✅ No performance regression (measure with benchmarks)
6. ✅ ThreadSanitizer (TSan) reports no issues
7. ✅ Code review approval
8. ✅ Documentation updated

---

## Implementation Checklist

- [ ] Create `network/routing/MidiRouterCommands.h`
- [ ] Implement 8 command classes
- [ ] Add `NetworkConnectionQueue` to MidiRouter
- [ ] Add worker thread start/stop
- [ ] Implement `workerThreadRun()` loop
- [ ] Convert `forwardMessage()` to command dispatch
- [ ] Convert `registerLocalPort()` to command dispatch
- [ ] Convert `unregisterLocalPort()` to command dispatch
- [ ] Convert `queueReceivedMessage()` to command dispatch
- [ ] Convert `getStatistics()` to query command
- [ ] Convert `resetStatistics()` to command dispatch
- [ ] Convert `setRouteManager()` to command dispatch
- [ ] Convert `setNetworkTransport()` to command dispatch
- [ ] Remove all 3 mutexes
- [ ] Remove all `std::lock_guard` usage
- [ ] Update MidiRouter.h documentation
- [ ] Add test helpers for async commands
- [ ] Run all 37 tests - verify pass
- [ ] Run stress test (10k messages/sec)
- [ ] Run ThreadSanitizer - verify no issues
- [ ] Measure performance (latency, throughput)
- [ ] Update architectural documentation
- [ ] Code review
- [ ] Merge to main branch

---

## Timeline

**Estimated**: 8-12 hours
- Step 1 (Infrastructure): 2 hours
- Step 2 (API Refactoring): 3 hours
- Step 3 (Remove Mutexes): 1 hour
- Step 4 (Testing): 2-3 hours
- Step 5 (Optimization): 2 hours
- Step 6 (Documentation): 1 hour

**Target Completion**: Before Phase 4 implementation

---

## References

**Existing SEDA Implementation**:
- `network/mesh/NetworkConnectionQueue.h` - Command queue infrastructure
- `network/mesh/NetworkConnectionQueue.cpp` - Implementation
- `network/mesh/tests/NetworkConnectionQueueTest.cpp` - Test patterns

**Related Documentation**:
- Phase B.1 SEDA infrastructure workplan
- NetworkConnection SEDA integration docs

---

## Next Steps

1. **Review this plan** with architecture team
2. **Assign implementation** to backend engineer
3. **Create feature branch**: `feat/midirouter-seda-refactoring`
4. **Implement** following checklist above
5. **Test** thoroughly
6. **Merge** before Phase 4

---

**Author**: Claude Code
**Reviewers**: [To be assigned]
**Status**: PLANNED - Ready for implementation
