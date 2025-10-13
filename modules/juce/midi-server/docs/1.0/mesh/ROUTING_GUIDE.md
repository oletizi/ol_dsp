# MIDI Routing Configuration Guide

## Overview

The Network MIDI Mesh uses explicit routing rules to control MIDI message flow between nodes. Routes are configured via the HTTP API and can be persisted to disk.

## Routing Concepts

### Device Registry
- Each node maintains a **DeviceRegistry** with all known MIDI devices (local and remote)
- Local devices have a null UUID (`00000000-0000-0000-0000-000000000000`)
- Remote devices have the UUID of their owner node

### Routing Rules
- Each rule maps a **source** device to a **destination** device
- Rules are identified by a UUID
- Rules support filters (MIDI channel, message type)
- Rules have priority (higher number = higher priority)
- Rules can be enabled/disabled

### Route Manager
- Manages the collection of routing rules
- Applies rules when MIDI messages are received
- Provides HTTP API for CRUD operations

## HTTP API Reference

### Base URL
```
http://localhost:<port>
```

### 1. List All Devices
```bash
GET /midi/devices
```

**Response:**
```json
{
  "total": 38,
  "local": 18,
  "remote": 20,
  "devices": [
    {
      "id": 1,
      "name": "Network MIDI Node f2e7caaa In",
      "type": "input",
      "is_local": true,
      "owner_node": "00000000-0000-0000-0000-000000000000"
    },
    {
      "id": 21,
      "name": "Network MIDI Node 75c208df In",
      "type": "input",
      "is_local": false,
      "owner_node": "75c208dfcfe7411482fd64ad8e008286"
    }
  ]
}
```

### 2. List All Routing Rules
```bash
GET /routing/routes
```

**Response:**
```json
{
  "rules": [
    {
      "rule_id": "a1b2c3d4-...",
      "enabled": true,
      "priority": 100,
      "source": {
        "node_id": "00000000-0000-0000-0000-000000000000",
        "device_id": 3
      },
      "destination": {
        "node_id": "75c208dfcfe7411482fd64ad8e008286",
        "device_id": 21
      },
      "statistics": {
        "messages_forwarded": 42,
        "messages_dropped": 0
      }
    }
  ]
}
```

### 3. Create Routing Rule
```bash
POST /routing/routes
Content-Type: application/json

{
  "source_node_id": "00000000-0000-0000-0000-000000000000",
  "source_device_id": 3,
  "destination_node_id": "75c208dfcfe7411482fd64ad8e008286",
  "destination_device_id": 21,
  "enabled": true,
  "priority": 100
}
```

**Response:**
```json
{
  "route_id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "status": "created"
}
```

### 4. Get Specific Route
```bash
GET /routing/routes/:id
```

### 5. Update Route
```bash
PUT /routing/routes/:id
Content-Type: application/json

{
  "enabled": false
}
```

### 6. Delete Route
```bash
DELETE /routing/routes/:id
```

## Configuration Examples

### Example 1: Simple Two-Node Routing

**Scenario:** Forward all MIDI from Node 1's virtual1 to Node 2's virtual2.

**Step 1: Discover device IDs**
```bash
# On Node 1 (port 8091)
curl http://localhost:8091/midi/devices | jq '.devices[] | select(.name | contains("virtual1"))'
# Result: device_id = 8, owner_node = "00000000..." (local)

# On Node 2 (port 8092)
curl http://localhost:8092/midi/devices | jq '.devices[] | select(.name | contains("virtual2"))'
# Result: device_id = 9, owner_node = "00000000..." (local)
```

**Step 2: Query Node 2's UUID**
```bash
curl http://localhost:8092/node/info | jq '.uuid'
# Result: "75c208dfcfe7411482fd64ad8e008286"
```

**Step 3: Create routing rule on Node 1**
```bash
curl -X POST http://localhost:8091/routing/routes \
  -H "Content-Type: application/json" \
  -d '{
    "source_node_id": "00000000-0000-0000-0000-000000000000",
    "source_device_id": 8,
    "destination_node_id": "75c208dfcfe7411482fd64ad8e008286",
    "destination_device_id": 9,
    "enabled": true,
    "priority": 100
  }'
```

**Step 4: Test MIDI flow**
```bash
# Send MIDI to virtual1 on Node 1
# It should appear on virtual2 on Node 2
```

### Example 2: Bidirectional Routing

For bidirectional communication, create routes in both directions:

**Node 1 → Node 2:**
```bash
curl -X POST http://localhost:8091/routing/routes \
  -H "Content-Type: application/json" \
  -d '{
    "source_node_id": "00000000-0000-0000-0000-000000000000",
    "source_device_id": 8,
    "destination_node_id": "<NODE2_UUID>",
    "destination_device_id": 12,
    "enabled": true
  }'
```

**Node 2 → Node 1:**
```bash
curl -X POST http://localhost:8092/routing/routes \
  -H "Content-Type: application/json" \
  -d '{
    "source_node_id": "00000000-0000-0000-0000-000000000000",
    "source_device_id": 10,
    "destination_node_id": "<NODE1_UUID>",
    "destination_device_id": 15,
    "enabled": true
  }'
```

### Example 3: Filtered Routing (Channel-Specific)

**Note:** Channel and message type filters are not yet exposed via HTTP API. This requires direct rule creation in code or future API enhancement.

## Persistence

Routing rules are automatically persisted to:
```
~/Library/Caches/network_midi_server/midi-network-<uuid>/routes.json
```

Rules are loaded on startup and saved when modified.

## Loop Prevention

The router includes automatic loop prevention:
- **Max hops:** 8 (prevents infinite chains)
- **Visited device tracking:** Prevents A → B → A cycles
- Messages that exceed limits are dropped and logged

## Statistics

Each routing rule tracks statistics:
- Messages forwarded
- Messages dropped
- Last forwarded time

Query statistics via:
```bash
curl http://localhost:8091/routing/routes/<rule-id>
```

## Troubleshooting

### Messages Not Routing
1. **Check rule exists and is enabled:**
   ```bash
   curl http://localhost:8091/routing/routes
   ```

2. **Verify device IDs are correct:**
   ```bash
   curl http://localhost:8091/midi/devices
   ```

3. **Check router statistics:**
   ```bash
   curl http://localhost:8091/routing/routes/<rule-id>
   # Look at messages_forwarded counter
   ```

4. **Check server logs** for routing errors

### Device Not Found
- Ensure mesh is fully formed (check `/network/mesh`)
- Remote devices appear only after handshake completes
- Device IDs may change between node restarts

### High Latency
- UDP transport has ~35μs latency for realtime messages
- Check network statistics: `curl http://localhost:8091/network/stats`
- Look for packet loss or retries

## Best Practices

1. **Start simple:** Create one bidirectional route pair first
2. **Use descriptive priorities:** High-priority routes = important MIDI channels
3. **Monitor statistics:** Regularly check forwarded/dropped counters
4. **Backup routes.json:** Persisted rules can be version controlled
5. **Test incrementally:** Add one route at a time, verify each works

## Future Enhancements

Planned features for v1.1+:
- **Default routing policies:** Auto-create common routing patterns
- **Filter API:** Expose channel/message type filters via HTTP
- **Route templates:** Pre-configured routing patterns
- **GUI management:** Web-based routing configuration
- **Wildcard rules:** "Forward all from device X"

---

**See also:**
- [Implementation Status](./implementation/STATUS.md)
- [Integration Test Script](../../tests/integration/mesh_midi_routing_test.sh)
- [API Documentation](./API.md) (planned)
