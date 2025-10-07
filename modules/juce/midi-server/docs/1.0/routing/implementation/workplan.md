# Cross-Node MIDI Routing - Implementation Workplan

**Version:** 1.0
**Status:** Planning
**Last Updated:** 2025-10-06

## Executive Summary

The Network MIDI Server mesh infrastructure is operational with SEDA architecture and dual-transport MIDI. Nodes successfully connect, exchange device lists, and register remote devices. However, **cross-node MIDI routing is not yet configured**, preventing MIDI messages from flowing between nodes.

This workplan defines the implementation of cross-node routing configuration and message forwarding.

---

## Current State Assessment

### ✅ What's Working (Verified by Integration Test)

1. **Mesh Formation**
   - Two nodes start independently
   - mDNS discovery finds peers
   - HTTP handshake exchanges device lists
   - Mesh connection established successfully

2. **Device Registration**
   - Local devices registered in DeviceRegistry (6 per node)
   - Remote devices received via handshake (6 devices)
   - Remote devices registered with correct owner node UUID
   - Device endpoint returns all devices (local + remote)

3. **SEDA Architecture**
   - ConnectionWorker threads process commands
   - Command queues functional
   - Callbacks properly invoked
   - State management working

4. **Local MIDI I/O**
   - MIDI can be sent to local virtual ports
   - MIDI can be received from local virtual ports
   - JUCE MIDI subsystem operational

### ❌ What's Missing

1. **Cross-Node Routing Configuration**
   - No API to configure forwarding rules
   - No automatic route creation
   - No route persistence
   - No route priority/filtering

2. **MIDI Message Forwarding**
   - Messages arrive at local input but aren't forwarded
   - No logic to determine which remote outputs should receive messages
   - No broadcast/multicast support

3. **Route Management**
   - No route validation
   - No conflict detection
   - No route updates when topology changes

---

## Gap Analysis

### Core Issue

When MIDI arrives at Node 1's `virtual1` input:
1. ✅ MidiInputCallback receives the message
2. ✅ Message converted to bytes
3. ✅ Device ID looked up (deviceId=2 for virtual1)
4. ✅ MidiRouter.sendMessage() called
5. ✅ RoutingTable.getRoute() finds local route
6. ❌ **No forwarding rule exists to send to Node 2**

### Current Routing Table Entries

**Node 1:**
```
Device ID | Name          | Type   | Owner Node | Is Local
----------|---------------|--------|------------|----------
1         | Network m4    | input  | (local)    | true
2         | virtual1      | input  | (local)    | true
3         | virtual2      | input  | (local)    | true
4         | Network m4    | output | (local)    | true
5         | virtual1      | output | (local)    | true
6         | virtual2      | output | (local)    | true
```

**Node 2:**
```
Device ID | Name          | Type   | Owner Node           | Is Local
----------|---------------|--------|----------------------|----------
1         | Network m4    | input  | node-1-uuid          | false
2         | virtual1      | input  | node-1-uuid          | false
3         | virtual2      | input  | node-1-uuid          | false
4         | Network m4    | output | node-1-uuid          | false
5         | virtual1      | output | node-1-uuid          | false
6         | virtual2      | output | node-1-uuid          | false
1         | Network m4    | input  | (local)              | true
2         | virtual1      | input  | (local)              | true
3         | virtual2      | input  | (local)              | true
4         | Network m4    | output | (local)              | true
5         | virtual1      | output | (local)              | true
6         | virtual2      | output | (local)              | true
```

**Problem:** Device IDs conflict between local and remote devices on Node 2!

---

## Implementation Phases

### Phase 1: Device ID Namespacing (Critical Fix)

**Problem:** Remote devices use same IDs as local devices, causing routing conflicts.

**Solution:** Use namespaced device IDs.

#### Option A: UUID-based Global IDs
```cpp
struct GlobalDeviceId {
    juce::Uuid nodeId;      // Owner node
    uint16_t localId;       // Local device ID

    std::string toString() const {
        return nodeId.toString() + ":" + std::to_string(localId);
    }
};
```

**Pros:** Globally unique, no conflicts
**Cons:** Complex, breaks existing code using uint16_t

#### Option B: ID Prefix Ranges (RECOMMENDED)
```cpp
// Reserve ID ranges per node in mesh
// Node 0: 1-999
// Node 1: 1000-1999
// Node 2: 2000-2999
// etc.

uint16_t getGlobalDeviceId(uint16_t localId, uint8_t nodeIndex) {
    return (nodeIndex * 1000) + localId;
}
```

**Pros:** Simple, backward compatible with uint16_t
**Cons:** Limited to ~65 nodes with 1000 devices each

#### Option C: Composite Key in RoutingTable (SELECTED)
Keep local IDs but use (nodeId, deviceId) as composite key internally.

```cpp
class RoutingTable {
    struct RouteKey {
        juce::Uuid nodeId;  // Null for local devices
        uint16_t deviceId;

        bool operator<(const RouteKey& other) const;
    };

    std::map<RouteKey, Route> routes;
public:
    std::optional<Route> getRoute(const juce::Uuid& nodeId, uint16_t deviceId);
    std::optional<Route> getLocalRoute(uint16_t deviceId);  // Helper
};
```

**Pros:** No breaking changes, clear ownership
**Cons:** API changes to routing methods

**Decision:** Use Option C for internal storage, maintain simple uint16_t API for local devices.

#### Implementation Tasks
- [ ] Modify RoutingTable to use composite keys
- [ ] Update DeviceRegistry to track (nodeId, deviceId) pairs
- [ ] Add API methods for local vs remote route lookups
- [ ] Update MidiRouter to use new routing APIs
- [ ] Write unit tests for namespacing

**Estimated Time:** 2-3 days

---

### Phase 2: Routing Configuration API

Add HTTP endpoints to configure cross-node routes.

#### Route Configuration Structure

```json
{
  "routes": [
    {
      "id": "route-1",
      "source": {
        "node_id": "ffe077ad-9cab-480a-a799-bf4593a72d5f",
        "device_id": 2,
        "device_name": "virtual1",
        "type": "input"
      },
      "destinations": [
        {
          "node_id": "36222d7d-c84e-4dd5-b0f5-01159de9b5da",
          "device_id": 6,
          "device_name": "virtual2",
          "type": "output"
        }
      ],
      "enabled": true,
      "priority": 100
    }
  ]
}
```

#### HTTP API Endpoints

##### 1. List All Routes
```
GET /routing/routes
```

**Response:**
```json
{
  "routes": [ /* array of route objects */ ],
  "total": 5,
  "local_routes": 3,
  "remote_routes": 2
}
```

##### 2. Create Route
```
POST /routing/routes

{
  "source_node_id": "local",  // or UUID
  "source_device_id": 2,
  "destination_node_id": "36222d7d-c84e-4dd5-b0f5-01159de9b5da",
  "destination_device_id": 6,
  "enabled": true
}
```

**Response:**
```json
{
  "route_id": "route-1234",
  "status": "created"
}
```

##### 3. Delete Route
```
DELETE /routing/routes/{route_id}
```

##### 4. Update Route
```
PUT /routing/routes/{route_id}

{
  "enabled": false
}
```

##### 5. Get Routing Table
```
GET /routing/table
```

Returns current routing table state (for debugging).

#### Implementation Tasks
- [ ] Define Route data structure
- [ ] Implement RouteManager class
- [ ] Add HTTP endpoints to NetworkMidiServer
- [ ] Implement route validation logic
- [ ] Add route persistence (JSON file)
- [ ] Write integration tests

**Estimated Time:** 3-4 days

---

### Phase 3: Message Forwarding Logic

Implement the actual MIDI forwarding based on configured routes.

#### Current Flow (Local Only)
```
MIDI Input → handleIncomingMidiMessage()
           → getDeviceIdForInput()
           → midiRouter->sendMessage(deviceId, data)
           → routingTable.getRoute(deviceId)
           → routeLocalMessage() OR routeNetworkMessage()
```

#### New Flow (With Forwarding)
```
MIDI Input → handleIncomingMidiMessage()
           → getDeviceIdForInput()
           → midiRouter->sendMessage(localNodeId, deviceId, data)
           → routeManager->getForwardingRules(localNodeId, deviceId)
           → FOR EACH destination:
               - If local: routeLocalMessage()
               - If remote: routeNetworkMessage()
```

#### ForwardingEngine Design

```cpp
class ForwardingEngine {
public:
    struct ForwardingRule {
        juce::Uuid sourceNodeId;
        uint16_t sourceDeviceId;
        juce::Uuid destNodeId;
        uint16_t destDeviceId;
        bool enabled;
        int priority;
    };

    // Get all destinations for a source
    std::vector<ForwardingRule> getDestinations(
        const juce::Uuid& sourceNodeId,
        uint16_t sourceDeviceId
    );

    // Add/remove rules
    void addRule(const ForwardingRule& rule);
    void removeRule(const std::string& ruleId);

    // Process incoming MIDI
    void forwardMessage(
        const juce::Uuid& sourceNodeId,
        uint16_t sourceDeviceId,
        const std::vector<uint8_t>& midiData
    );

private:
    std::map<std::string, ForwardingRule> rules;
    MidiRouter* router;
    MeshManager* meshManager;
};
```

#### Integration Points

**In NetworkMidiServer:**
```cpp
void handleIncomingMidiMessage(juce::MidiInput* source,
                               const juce::MidiMessage& message) override {
    auto deviceId = getDeviceIdForInput(source);
    std::vector<uint8_t> data(message.getRawData(),
                              message.getRawData() + message.getRawDataSize());

    // NEW: Forward through forwarding engine
    forwardingEngine->forwardMessage(identity.getNodeId(), deviceId, data);
}
```

**In MidiRouter:**
```cpp
void routeNetworkMessage(const juce::Uuid& nodeId,
                         uint16_t deviceId,
                         const std::vector<uint8_t>& midiData) {
    // Look up mesh connection
    auto* connection = meshManager->getConnection(nodeId);
    if (!connection) {
        reportError("No connection to node " + nodeId.toString());
        return;
    }

    // Send via network (UDP for realtime, TCP for sysex)
    if (isRealtimeMessage(midiData)) {
        connection->sendRealtimeMidi(deviceId, midiData);
    } else {
        connection->sendNonRealtimeMidi(deviceId, midiData);
    }
}
```

#### Implementation Tasks
- [ ] Implement ForwardingEngine class
- [ ] Add getDestinations() lookup logic
- [ ] Integrate with MidiRouter
- [ ] Add forwarding statistics/metrics
- [ ] Implement message filtering (channel, type)
- [ ] Write unit tests for forwarding logic

**Estimated Time:** 4-5 days

---

### Phase 4: Network Transport Integration

Connect forwarding engine to existing dual-transport MIDI infrastructure.

#### ConnectionWorker Integration

**Add to NetworkConnection:**
```cpp
class NetworkConnection {
public:
    // NEW: Send MIDI to remote node
    void sendMidi(uint16_t deviceId, const std::vector<uint8_t>& midiData);

    // Existing callback for receiving MIDI
    std::function<void(const MidiMessage&)> onMidiMessageReceived;
};
```

**Implement in ConnectionWorker:**
```cpp
void ConnectionWorker::handleSendMidiCommand(Commands::SendMidiCommand* cmd) {
    // Classify message
    if (classifier.isRealtime(cmd->midiData)) {
        // Send via UDP (already implemented)
        if (realtimeTransport) {
            realtimeBuffer->write(cmd->deviceId, cmd->midiData);
        }
    } else {
        // Send via TCP (already implemented)
        if (nonRealtimeTransport) {
            nonRealtimeTransport->sendMessage(cmd->deviceId, cmd->midiData);
        }
    }
}
```

#### MeshManager Routing Bridge

```cpp
// In MeshManager or new NetworkTransportAdapter class
void sendMidiToNode(const juce::Uuid& nodeId,
                   uint16_t deviceId,
                   const std::vector<uint8_t>& midiData) {
    auto* connection = connectionPool.getConnection(nodeId);
    if (connection && connection->getState() == NetworkConnection::State::Connected) {
        connection->sendMidi(deviceId, midiData);
    } else {
        juce::Logger::writeToLog("Cannot send MIDI - node not connected: " +
                                nodeId.toString());
    }
}
```

#### Implementation Tasks
- [ ] Add sendMidi() method to NetworkConnection
- [ ] Create SendMidiCommand in Commands.h
- [ ] Implement SEDA command handling
- [ ] Add connection state validation
- [ ] Implement retry logic for failed sends
- [ ] Add transport statistics
- [ ] Write integration tests

**Estimated Time:** 3-4 days

---

### Phase 5: Auto-Configuration Modes

Implement common routing patterns for ease of use.

#### Mode 1: Mirror Mode
Every local input forwards to same device on all remote nodes.
```
Node 1 virtual1 (input) → Node 2 virtual1 (output)
                        → Node 3 virtual1 (output)
```

#### Mode 2: Broadcast Mode
Local input forwards to ALL remote outputs.
```
Node 1 virtual1 (input) → Node 2 virtual1 (output)
                        → Node 2 virtual2 (output)
                        → Node 3 virtual1 (output)
                        → Node 3 virtual2 (output)
```

#### Mode 3: Ring Mode
Form a ring topology for MIDI forwarding.
```
Node 1 virtual1 → Node 2 virtual1
Node 2 virtual1 → Node 3 virtual1
Node 3 virtual1 → Node 1 virtual1
```

#### API Design
```
POST /routing/auto-configure

{
  "mode": "mirror",
  "source_devices": ["virtual1"],
  "target_nodes": ["all"]  // or specific UUIDs
}
```

#### Implementation Tasks
- [ ] Implement auto-configuration algorithms
- [ ] Add mode selection API
- [ ] Validate topology constraints
- [ ] Add configuration presets
- [ ] Document each mode with examples
- [ ] Write integration tests

**Estimated Time:** 2-3 days

---

### Phase 6: Testing & Validation

#### Unit Tests
- [ ] RouteKey comparison operators
- [ ] RoutingTable composite key lookups
- [ ] ForwardingEngine rule matching
- [ ] Route validation logic
- [ ] Auto-configuration algorithms

#### Integration Tests
- [ ] Two-node MIDI forwarding (virtual1 → virtual2)
- [ ] Multi-destination forwarding (1 input → 3 outputs)
- [ ] Mesh topology changes (node disconnect/reconnect)
- [ ] Route priority resolution
- [ ] Duplicate message filtering
- [ ] Message ordering guarantees

#### End-to-End Tests
```bash
# Test 1: Basic forwarding
./mesh_midi_routing_test.sh

# Test 2: Multi-node mesh (3+ nodes)
./multi_node_mesh_test.sh

# Test 3: Auto-configuration
./auto_config_test.sh
```

#### Performance Tests
- [ ] Latency measurement (input → network → output)
- [ ] Throughput testing (messages/second)
- [ ] Mesh scaling (10+ nodes)
- [ ] Resource usage under load

**Estimated Time:** 5-6 days

---

## Technical Design Details

### RoutingTable Schema (Updated)

```cpp
class RoutingTable {
public:
    struct RouteKey {
        juce::Uuid nodeId;  // Null UUID for local devices
        uint16_t deviceId;

        bool operator<(const RouteKey& other) const {
            if (nodeId != other.nodeId)
                return nodeId < other.nodeId;
            return deviceId < other.deviceId;
        }
    };

    struct Route {
        RouteKey key;
        juce::String deviceName;
        juce::String deviceType;  // "input" or "output"
        bool isLocal() const { return key.nodeId.isNull(); }
    };

    // Core API
    void addRoute(const juce::Uuid& nodeId, uint16_t deviceId,
                  const juce::String& name, const juce::String& type);
    std::optional<Route> getRoute(const juce::Uuid& nodeId, uint16_t deviceId);
    std::optional<Route> getLocalRoute(uint16_t deviceId);
    void removeRoute(const juce::Uuid& nodeId, uint16_t deviceId);
    void removeAllRoutesForNode(const juce::Uuid& nodeId);

    // Query API
    std::vector<Route> getAllRoutes() const;
    std::vector<Route> getRoutesForNode(const juce::Uuid& nodeId) const;
    std::vector<Route> getLocalRoutes() const;

private:
    std::map<RouteKey, Route> routes;
    mutable std::mutex tableMutex;
};
```

### ForwardingRule Schema

```cpp
struct ForwardingRule {
    std::string ruleId;  // UUID for rule

    // Source
    juce::Uuid sourceNodeId;
    uint16_t sourceDeviceId;

    // Destination
    juce::Uuid destNodeId;
    uint16_t destDeviceId;

    // Configuration
    bool enabled = true;
    int priority = 100;  // Higher = higher priority

    // Filters
    std::optional<uint8_t> channelFilter;  // Only forward specific channel
    std::vector<uint8_t> messageTypeFilter;  // e.g., only Note On/Off

    // Statistics
    uint64_t messagesForwarded = 0;
    uint64_t messagesDropped = 0;
    juce::Time lastForwarded;
};
```

### Configuration Persistence

**File:** `~/.network_midi_server/routing_config.json`

```json
{
  "version": "1.0",
  "node_id": "ffe077ad-9cab-480a-a799-bf4593a72d5f",
  "auto_load": true,
  "rules": [
    {
      "rule_id": "a1b2c3d4",
      "source": {
        "node_id": "local",
        "device_id": 2
      },
      "destination": {
        "node_id": "36222d7d-c84e-4dd5-b0f5-01159de9b5da",
        "device_id": 6
      },
      "enabled": true,
      "priority": 100
    }
  ]
}
```

---

## Migration Strategy

### Backward Compatibility

1. **Existing Local Routing:** Must continue to work without changes
2. **Device Registry:** No breaking changes to public API
3. **HTTP Endpoints:** New endpoints only, no modifications to existing
4. **Configuration:** Optional - server works without routing config

### Rollout Plan

1. **Phase 1 (Device ID Namespacing):** Internal changes only, no visible impact
2. **Phase 2 (API):** New endpoints available but optional
3. **Phase 3-4 (Forwarding):** Opt-in via configuration
4. **Phase 5 (Auto-config):** Convenience feature for common patterns
5. **Phase 6 (Testing):** Validation before release

### Feature Flags

```cpp
// In NetworkMidiServer startup
bool enableCrossNodeRouting = config.get("enable_cross_node_routing", false);
if (enableCrossNodeRouting) {
    forwardingEngine = std::make_unique<ForwardingEngine>(midiRouter, meshManager);
}
```

---

## Risk Assessment

### High Risk
- **Device ID conflicts:** Addressed in Phase 1 with composite keys
- **Message loops:** Need loop detection in forwarding engine
- **Performance impact:** Forwarding adds latency - needs benchmarking

### Medium Risk
- **Configuration complexity:** Mitigated by auto-configuration modes
- **Mesh topology changes:** Routes must update when nodes disconnect
- **State synchronization:** Route config not shared across nodes (by design)

### Low Risk
- **API changes:** New endpoints only, no breaking changes
- **Testing complexity:** Standard integration test patterns

---

## Success Criteria

### Must Have (MVP)
1. ✅ Two nodes can forward MIDI between each other
2. ✅ Configuration API for creating routes
3. ✅ Integration test passes (virtual1 → virtual2 across nodes)
4. ✅ Device ID conflicts resolved
5. ✅ Basic error handling and logging

### Should Have
1. Multi-destination forwarding (1 → N)
2. Route persistence across restarts
3. Auto-configuration modes (mirror/broadcast)
4. Performance metrics and monitoring

### Nice to Have
1. WebUI for route configuration
2. Route templates/presets
3. Advanced filtering (channel, message type)
4. Dynamic route updates without restart

---

## Timeline Estimate

| Phase | Description | Duration | Dependencies |
|-------|-------------|----------|--------------|
| 1 | Device ID Namespacing | 2-3 days | None |
| 2 | Routing Configuration API | 3-4 days | Phase 1 |
| 3 | Message Forwarding Logic | 4-5 days | Phase 2 |
| 4 | Network Transport Integration | 3-4 days | Phase 3 |
| 5 | Auto-Configuration Modes | 2-3 days | Phase 4 |
| 6 | Testing & Validation | 5-6 days | Phase 5 |

**Total Estimated Time:** 19-25 days (approximately 4-5 weeks)

---

## Open Questions

1. **Route Synchronization:** Should routing configuration be shared across mesh?
   - **Current Answer:** No, each node configures its own forwarding rules
   - **Rationale:** Simpler, more flexible, avoids consensus problems

2. **Loop Detection:** How to prevent forwarding loops?
   - **Proposed:** TTL field in MIDI packets, max hop count
   - **Alternative:** Route validation at configuration time

3. **Priority Handling:** What happens when multiple rules match?
   - **Proposed:** Highest priority wins, ties use first-configured
   - **Alternative:** Forward to all matches (multicast)

4. **Latency Budget:** What's acceptable for cross-node forwarding?
   - **Target:** < 10ms for realtime messages (Note On/Off)
   - **Measurement:** Add timestamps to test infrastructure

5. **Configuration UI:** Command-line tool or web interface?
   - **Phase 1:** HTTP API only (use curl for config)
   - **Phase 2:** CLI tool for convenience
   - **Future:** Web UI (React/Vue)

---

## References

- [MidiRouter Implementation](/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/routing/MidiRouter.cpp)
- [RoutingTable Implementation](/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/network/routing/RoutingTable.cpp)
- [Mesh Integration Test](/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/tests/integration/mesh_midi_routing_test.sh)
- [SEDA Architecture Docs](/Users/orion/work/ol_dsp-midi-server/modules/juce/midi-server/docs/1.0/mesh/implementation/workplan.md)

---

## Next Steps

1. **Review this workplan** with team/stakeholders
2. **Prioritize phases** - can Phase 5 be deferred?
3. **Assign ownership** - who implements each phase?
4. **Set milestones** - weekly check-ins on progress
5. **Create tickets** - break down each phase into tasks

**Recommended Start:** Phase 1 (Device ID Namespacing) - foundational fix that unblocks everything else.
