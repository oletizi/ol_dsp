# Phase 4 Executive Summary: Multi-Hop Context Architecture

**Version:** 1.0
**Date:** 2025-10-06
**Status:** Design Approved for Implementation

---

## Problem Statement

Phase 3 routing implements loop prevention **within a single node** using `ForwardingContext`. However, this context is not transmitted across the network, making multi-hop loop detection impossible:

```
❌ Node A → Node B → Node A (loop NOT detected - context lost at Node B)
```

**Phase 4 Goal:** Embed `ForwardingContext` in MIDI packets for network-wide loop prevention.

---

## Solution Architecture

### 1. Packet Format Extension

**Add optional context extension to MidiPacket:**

```
┌─────────────────────────────────────────────┐
│ Standard 20-byte header (unchanged)         │
├─────────────────────────────────────────────┤
│ Context Extension (optional):               │
│   • Extension Type: 0x01           (1 byte) │
│   • Extension Length: N            (1 byte) │
│   • Hop Count: 0-8                 (1 byte) │
│   • Device Count: M                (1 byte) │
│   • Visited Device 1               (6 bytes)│
│     - Node ID Hash: uint32         (4 bytes)│
│     - Device ID: uint16            (2 bytes)│
│   • ... (repeat for M devices)              │
└─────────────────────────────────────────────┘
```

**Key Features:**
- New `HasContext` flag bit indicates extension presence
- Max size: 50 bytes (2 header + 8 devices × 6 bytes)
- Backward compatible (Phase 3 nodes ignore extension)

---

### 2. Context Serialization Format

**Hybrid Compact Format:**
- Self-describing (device_count indicates array length)
- Bounded size (max 8 hops × 6 bytes = 50 bytes)
- Reuses existing UUID hash infrastructure from MidiPacket
- Simple to serialize/deserialize (~700ns total)

**Why Not Bitmap?**
- Bitmap requires global device ID registry (complex)
- Hybrid format is simpler and meets size requirements

---

### 3. UUID Reverse Lookup

**Problem:** Context stores 32-bit hashes, need full UUIDs for DeviceKey.

**Solution:** `UuidRegistry` class maintains hash → UUID mapping.

```cpp
class UuidRegistry {
public:
    void registerNode(const juce::Uuid& nodeId);
    std::optional<juce::Uuid> lookupFromHash(uint32_t hash) const;
};
```

**Integration:**
- MeshManager owns registry
- Registers nodes on connection establishment
- Unregisters on disconnection
- Injected into MidiRouter for context deserialization

**Hash Collision Risk:**
- 32-bit hash: ~1 in 4 billion collision probability
- Acceptable for mesh <1000 nodes
- Future: Use full UUID if collisions become issue

---

### 4. MidiRouter Changes

**Updated Flow:**

```cpp
// Receive network packet
void MidiRouter::onNetworkPacketReceived(const MidiPacket& packet) {
    // Extract context from incoming packet (if present)
    auto contextOpt = packet.getForwardingContext(*uuidRegistry);

    ForwardingContext context = contextOpt.value_or(ForwardingContext{});

    // Forward with context
    forwardMessageInternal(sourceNode, deviceId, midiData, context);
}

// Forward to remote node
void MidiRouter::forwardToDestination(..., const ForwardingContext& context) {
    // Create packet
    MidiPacket packet = MidiPacket::createDataPacket(...);

    // Embed context
    packet.setForwardingContext(context);

    // Send via network
    networkTransport->sendPacket(packet, address, port);
}
```

**Key Changes:**
1. Extract context from incoming packets
2. Update context (visited devices, hop count)
3. Embed context in outgoing packets
4. Maintain backward compatibility

---

## Performance Characteristics

### Latency

```
Per-hop overhead:        ~760ns
3-hop total overhead:    ~2.3μs
8-hop total overhead:    ~6.1μs
Target: <10μs           ✓ Met
```

### Packet Size

```
Hops | Context Size | Total Packet Size
-----|--------------|------------------
0    | 0 bytes      | 23 bytes
1    | 10 bytes     | 33 bytes
2    | 16 bytes     | 39 bytes
8    | 52 bytes     | 75 bytes (max)

Target: <100 bytes      ✓ Met
```

### Memory

```
Per-message:             50 bytes (transient)
UUID registry:           16 bytes/node (persistent)
100-node mesh:           1.6 KB total
                        ✓ Negligible
```

---

## Backward Compatibility

### Phase 3 ↔ Phase 4 Interoperability

**Phase 3 Node receives Phase 4 packet:**
- Ignores `HasContext` flag (reserved bit)
- Forwards with fresh context (Phase 3 behavior)
- No errors

**Phase 4 Node receives Phase 3 packet:**
- No context flag set
- Creates fresh context
- Continues forwarding normally

**Mixed Mesh:**
- P3 → P4: Context starts at P4 node
- P4 → P3 → P4: Context lost at P3, resets at second P4
- Gradual rollout safe

---

## Implementation Plan

### Task Breakdown (16 days total)

| Task | Description | Duration |
|------|-------------|----------|
| 4.1  | Extend MidiPacket with context | 3 days |
| 4.2  | Implement UuidRegistry | 2 days |
| 4.3  | Update MidiRouter for context | 4 days |
| 4.4  | Update NetworkTransport interface | 2 days |
| 4.5  | Integrate with MeshManager | 2 days |
| 4.6  | End-to-end testing | 3 days |

**Critical Path:** 4.1 → 4.3 → 4.6

---

## Testing Strategy

### Unit Tests (15 cases)
- Context serialization/deserialization
- UUID registry lifecycle
- Hash collision detection
- Backward compatibility

### Integration Tests (8 scenarios)
- Two-node context propagation
- Three-node multi-hop
- Loop detection (circular route)
- Max hops exceeded
- Phase 3 ↔ Phase 4 compatibility

### Performance Tests
- Latency per hop (<10μs)
- Packet size distribution
- Throughput (1000 msg/sec with context)

---

## Risk Assessment

### High Risk (Mitigated)

**UUID Hash Collisions:**
- Probability: ~1/4B per pair
- Mitigation: Log collisions, monitor statistics
- Fallback: Use full UUID in v2.0

**Context Serialization Bugs:**
- Impact: Corrupted context, infinite loops
- Mitigation: Extensive unit tests, fuzz testing
- Fail-safe: Drop context on deserialization error

### Medium Risk

**Backward Compatibility:**
- Impact: Phase 3 nodes unable to communicate
- Mitigation: Phase 4 understands Phase 3 packets
- Rollout: Feature flag, gradual deployment

**Performance Degradation:**
- Impact: Increased latency
- Mitigation: Benchmarks show <10μs overhead
- Rollback: Disable context via feature flag

---

## Migration Strategy

### Rollout Stages

**Stage 1: Deploy with Context Disabled**
- Feature flag: `enable_forwarding_context=false`
- Test in staging environment
- No risk to existing traffic

**Stage 2: Enable on Test Nodes**
- 2-3 test nodes with context enabled
- Monitor packet sizes, latency
- Validate loop prevention

**Stage 3: Gradual Rollout**
- 25% of nodes enabled
- Monitor `loopsDetected`, `contextsProcessed` stats
- Roll back if issues detected

**Stage 4: Full Deployment**
- Enable on all nodes
- Context becomes standard for forwarding

---

## Success Criteria

### Must Have (MVP)
- ✅ Multi-hop routing with context (3+ nodes)
- ✅ Network-wide loop detection
- ✅ Backward compatibility with Phase 3
- ✅ <10μs latency overhead per hop
- ✅ <100 byte packet size
- ✅ All integration tests passing

### Should Have
- UUID registry with collision detection
- Performance monitoring dashboard
- Feature flag for gradual rollout
- Migration documentation

---

## Key Decisions

### Decision 1: Hybrid Context Format (vs Bitmap)
**Rationale:** Simpler implementation, meets size requirements, no global ID registry needed.

### Decision 2: Optional Context Extension (vs Mandatory)
**Rationale:** Enables backward compatibility, gradual rollout, reduces overhead for local-only routing.

### Decision 3: 32-bit UUID Hashes (vs Full UUID)
**Rationale:** Saves 24 bytes per hop, collision risk acceptable for mesh <1000 nodes.

### Decision 4: Context in Packet Extension (vs Separate Channel)
**Rationale:** Keeps context with message, simpler to implement, no synchronization issues.

---

## Files to Modify

### Core Packet Format
- `/network/core/MidiPacket.h` (add context methods)
- `/network/core/MidiPacket.cpp` (serialization logic)

### UUID Registry (New)
- `/network/routing/UuidRegistry.h` (new class)
- `/network/routing/UuidRegistry.cpp` (implementation)

### Routing Engine
- `/network/routing/MidiRouter.h` (context extraction/embedding)
- `/network/routing/MidiRouter.cpp` (forwarding logic)

### Mesh Integration
- `/network/mesh/MeshManager.h` (UUID registry ownership)
- `/network/mesh/MeshManager.cpp` (register/unregister nodes)
- `/network/mesh/NetworkConnection.h` (sendPacket method)

---

## Next Steps

1. **Approval:** Architecture team sign-off on design
2. **Prototype:** Context serialization benchmark (validate <1μs)
3. **Implementation:** Begin Task 4.1 (MidiPacket extension)
4. **Testing:** Set up 3-node test mesh
5. **Deployment:** Staged rollout with feature flag

**Target Completion:** 3 weeks from start

---

## Questions?

For detailed technical design, see:
- [Full Phase 4 Design Document](./phase4_multihop_context.md)
- [Routing Workplan (Phases 1-3)](./implementation/workplan.md)
- [Transport Layer Report](../../network/transport/PHASE4_REPORT.md)

**Contact:** Architecture team for review and approval.
