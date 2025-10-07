# Route Persistence

This document describes the persistence behavior for MIDI routing rules in the Network MIDI Server.

## Overview

Routing rules are automatically persisted to disk and reloaded on server startup, ensuring that routing configurations survive server restarts. This eliminates the need to manually reconfigure routes after every restart.

## File Location

Routes are stored in the instance-specific state directory:

```
/tmp/midi-network-{node-uuid}/routes.json
```

Where `{node-uuid}` is the unique identifier for this server instance.

### Example Path

```
/tmp/midi-network-a1b2c3d4-e5f6-7890-abcd-ef1234567890/routes.json
```

## File Format

Routes are stored as a JSON array of rule objects. Each rule contains:

- **ruleId**: Unique identifier (UUID)
- **enabled**: Boolean flag indicating if rule is active
- **priority**: Integer priority (higher = higher priority, default: 100)
- **source**: Source device specification
  - **nodeId**: UUID of the node owning the source device
  - **deviceId**: Integer device ID on that node
- **destination**: Destination device specification
  - **nodeId**: UUID of the node owning the destination device
  - **deviceId**: Integer device ID on that node
- **channelFilter**: (optional) MIDI channel filter (0 = all, 1-16 = specific channel)
- **messageTypeFilter**: Integer bitmask for message type filtering
- **statistics**: Usage statistics
  - **messagesForwarded**: Count of forwarded messages
  - **messagesDropped**: Count of dropped messages
  - **lastForwardedTime**: Timestamp of last forwarded message (milliseconds)

### Example File

```json
[
  {
    "ruleId": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
    "enabled": true,
    "priority": 100,
    "source": {
      "nodeId": "00000000-0000-0000-0000-000000000000",
      "deviceId": 2
    },
    "destination": {
      "nodeId": "36222d7d-c84e-4dd5-b0f5-01159de9b5da",
      "deviceId": 6
    },
    "messageTypeFilter": 255,
    "statistics": {
      "messagesForwarded": 1234,
      "messagesDropped": 5,
      "lastForwardedTime": 1728256000000
    }
  },
  {
    "ruleId": "b2c3d4e5-f6a7-8901-bcde-f12345678901",
    "enabled": true,
    "priority": 90,
    "source": {
      "nodeId": "36222d7d-c84e-4dd5-b0f5-01159de9b5da",
      "deviceId": 3
    },
    "destination": {
      "nodeId": "00000000-0000-0000-0000-000000000000",
      "deviceId": 4
    },
    "channelFilter": 1,
    "messageTypeFilter": 3,
    "statistics": {
      "messagesForwarded": 567,
      "messagesDropped": 0,
      "lastForwardedTime": 1728256100000
    }
  }
]
```

## Auto-Load Behavior

Routes are automatically loaded during server startup in the following sequence:

1. **UDP transport** is started
2. **Device registry** is initialized
3. **Routing infrastructure** is created (DeviceRegistry, RoutingTable, RouteManager)
4. **Local MIDI devices** are registered
5. **Routes are loaded** from `routes.json` (if file exists)
6. **Remote devices** are discovered via mesh network
7. **MIDI inputs** are started

This ensures that all devices are registered before attempting to load routes, preventing validation errors due to missing devices.

### Startup Logging

```
Loading routes from /tmp/midi-network-{uuid}/routes.json
Loaded 5 routing rule(s)
```

If no routes file exists:

```
No routes file found, starting with empty routing table
```

If loading fails:

```
Warning: Failed to load routes from file
```

## Auto-Save Behavior

Routes are automatically saved during server shutdown:

1. **Routes are saved** to `routes.json`
2. **MIDI inputs** are stopped
3. **Network components** are shut down
4. **Instance cleanup** occurs

### Shutdown Logging

```
Saving routes to /tmp/midi-network-{uuid}/routes.json
Saved 5 routing rule(s)
```

If saving fails:

```
Warning: Failed to save routes to file
```

## Manual Save/Load via API

Currently, routes are only saved automatically on shutdown. Future API endpoints may support:

- **POST /routing/save** - Manually trigger route save
- **POST /routing/load** - Manually trigger route reload
- **POST /routing/export** - Export routes to custom file
- **POST /routing/import** - Import routes from custom file

These endpoints are not yet implemented.

## Validation

When loading routes from file:

1. **JSON parsing** - File must be valid JSON array
2. **Rule validation** - Each rule is validated:
   - Rule ID must not be empty
   - Source and destination must be different
   - Channel filter must be valid (0-16)
   - Source device must exist in DeviceRegistry
   - Destination device must exist in DeviceRegistry
   - Source must be type "input"
   - Destination must be type "output"

### Invalid Rules

If a rule fails validation during load, it is silently skipped. Only valid rules are loaded.

This can happen when:
- Referenced devices no longer exist
- Device IDs have changed
- Remote nodes are not yet connected

## Remote Device Routes

Routes that reference remote devices (devices on other nodes) are loaded during startup, but may fail validation if:

1. The remote node is not yet discovered
2. The remote node's devices have not been registered
3. The remote node is offline

These routes remain in the file and will be validated again on next startup when the remote node may be available.

## Statistics Persistence

Route statistics (message counts, last forwarded time) are persisted along with the route configuration. This allows tracking long-term routing behavior across server restarts.

To reset statistics, use the API endpoint:
- **POST /routing/statistics/reset** (not yet implemented)

## Instance Isolation

Each server instance has its own routes file, ensuring that:
- Multiple instances can run simultaneously with different routing configurations
- Routes are isolated per instance (identified by node UUID)
- Stopping one instance does not affect another instance's routes

## Cleanup Behavior

Route files are **NOT** automatically deleted when the server shuts down. They persist in the instance directory until:

1. The instance directory is manually deleted
2. The operating system cleans up `/tmp` (varies by OS)
3. The server is restarted with a different node UUID

This ensures routes survive unexpected crashes and restarts.

## Best Practices

1. **Backup routes** - Periodically export routes to a persistent location
2. **Monitor logs** - Check startup/shutdown logs for load/save errors
3. **Test after restart** - Verify routes are active after server restart
4. **Document routes** - Keep external documentation of critical routes
5. **Use descriptive priorities** - Assign meaningful priority values for rule ordering

## Troubleshooting

### Routes not loading on startup

Check:
1. File exists at `/tmp/midi-network-{uuid}/routes.json`
2. File is valid JSON
3. Referenced devices exist and are registered
4. Log messages for validation errors

### Routes not saving on shutdown

Check:
1. Write permissions to instance directory
2. Disk space available
3. Log messages for save errors

### Routes disappear after restart

Check:
1. Server is using the same node UUID (check identity persistence)
2. Instance directory was not cleaned up
3. File path is correct in logs

## Implementation Details

### RouteManager Methods

- `loadFromFile(const juce::File& file)` - Load routes from JSON file
- `saveToFile(const juce::File& file)` - Save routes to JSON file

### NetworkMidiServer Integration

- `loadRoutes()` - Called during `startServer()` after device registration
- `saveRoutes()` - Called during `stopServer()` before cleanup

### File I/O

- Uses JUCE `File` class for cross-platform file operations
- Uses JUCE `JSON` parser for serialization/deserialization
- Thread-safe (RouteManager uses mutex for all operations)

## Future Enhancements

Potential improvements to route persistence:

1. **Auto-save on modification** - Save immediately when routes change (not just on shutdown)
2. **Configurable location** - Allow custom route file path
3. **Backup/versioning** - Keep multiple versions of route files
4. **Export/import** - API endpoints for manual export/import
5. **Hot reload** - Reload routes without server restart
6. **Merge strategy** - Options for merging loaded routes with existing routes

## Related Documentation

- [ForwardingRule.h](../../../network/routing/ForwardingRule.h) - Rule data structure
- [RouteManager.h](../../../network/routing/RouteManager.h) - Route management API
- [InstanceManager.h](../../../network/core/InstanceManager.h) - Instance isolation
