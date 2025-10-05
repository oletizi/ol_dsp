# JUCE MIDI Server - Claude AI Agent Guidelines

This document provides guidelines for AI agents working on the JUCE-based MIDI HTTP server module. This is a C++ JUCE application that provides HTTP endpoints for MIDI device discovery and communication.

## Module Overview

This is a **C++ JUCE application** that:
- Discovers MIDI input/output devices on the system
- Provides HTTP REST API for MIDI device queries
- Uses cpp-httplib for HTTP server functionality
- Built with CMake and JUCE framework
- Part of the larger OL_DSP MIDI server ecosystem

## Project Structure

```
midi-server/
├── CMakeLists.txt           # CMake build configuration
├── MidiServer.cpp           # Core MIDI device discovery logic
├── MidiHttpServer.cpp       # HTTP server implementation v1
├── MidiHttpServer2.cpp      # HTTP server implementation v2
├── httplib.h                # cpp-httplib single-header HTTP library
└── cpp-httplib-0.14.3/      # cpp-httplib source distribution
```

## Technology Stack

- **Language**: C++17 or later
- **Framework**: JUCE (audio/MIDI framework)
- **HTTP Library**: cpp-httplib 0.14.3
- **Build System**: CMake
- **Platform**: Cross-platform (macOS, Windows, Linux)

## Core Requirements

### JUCE & MIDI Specifics

- **JUCE Application Lifecycle**: Follow JUCE's application initialization patterns
- **MIDI Device Management**: Use JUCE's MidiInput/MidiOutput classes
- **Thread Safety**: JUCE MIDI callbacks run on separate threads - use proper synchronization
- **Memory Management**: Follow JUCE's ownership patterns (ReferenceCountedObject, OwnedArray, etc.)
- **Real-time Safety**: MIDI callbacks must be real-time safe (no allocation, no locks if possible)

### HTTP Server Guidelines

- **RESTful Design**: Follow REST principles for endpoint design
- **JSON Responses**: Use structured JSON for device information
- **Error Handling**: Return appropriate HTTP status codes (200, 404, 500, etc.)
- **CORS**: Consider cross-origin requests if accessed from web clients
- **Port Configuration**: Use configurable port numbers (default: 8080 or environment variable)

### Code Quality Standards

- **C++ Modern Practices**: Use C++17 features appropriately
- **RAII**: Resource Acquisition Is Initialization for all resources
- **Const Correctness**: Mark methods and parameters const where appropriate
- **Smart Pointers**: Prefer std::unique_ptr and std::shared_ptr over raw pointers
- **File Size**: Keep implementation files under 300-500 lines - refactor if larger
- **Header Guards**: Use `#pragma once` or include guards

### Build & Testing

- **CMake Best Practices**:
  - Use modern CMake (3.15+)
  - Proper target-based configuration
  - Find JUCE via find_package or add_subdirectory
- **Testing**: Write unit tests for MIDI discovery logic
- **CI/CD**: Ensure builds work on all target platforms

## API Endpoints

The HTTP server should provide endpoints like:

```
GET /midi/inputs          # List all MIDI input devices
GET /midi/outputs         # List all MIDI output devices
GET /midi/devices         # List all MIDI devices (inputs + outputs)
GET /health              # Health check endpoint
```

**Response Format**:
```json
{
  "inputs": [
    {"id": 0, "name": "USB MIDI Device", "manufacturer": "Vendor"},
    ...
  ],
  "outputs": [
    {"id": 0, "name": "USB MIDI Device", "manufacturer": "Vendor"},
    ...
  ]
}
```

## Development Workflow for AI Agents

### Before Making Changes

1. **Understand JUCE patterns** - Review existing JUCE initialization code
2. **Check CMakeLists.txt** - Understand build configuration and dependencies
3. **Review MIDI device enumeration** - Understand how JUCE discovers devices
4. **Test HTTP endpoints** - Verify current API behavior

### When Writing Code

1. **Follow JUCE conventions** - Use JUCE naming (camelCase for methods)
2. **Thread safety** - Be aware of JUCE's threading model
3. **Resource cleanup** - Ensure proper MIDI device cleanup on shutdown
4. **Error handling** - Handle MIDI device errors gracefully
5. **HTTP error responses** - Return meaningful error messages and status codes
6. **Keep files modular** - Separate MIDI logic from HTTP server logic
7. **Document endpoints** - Add comments describing API endpoints

### Building & Testing

```bash
# Configure CMake build
cmake -B build -S .

# Build the project
cmake --build build

# Run the server
./build/MidiHttpServer

# Test endpoints
curl http://localhost:8080/midi/devices
curl http://localhost:8080/health
```

## MIDI-Specific Best Practices

### Device Discovery
- **Refresh on Hotplug**: Detect when MIDI devices are connected/disconnected
- **Device IDs**: Use stable device identifiers when possible
- **Device Info**: Provide manufacturer, model, and connection status

### Threading
- **MIDI Callbacks**: Never block in MIDI input callbacks
- **HTTP Handlers**: Can use blocking I/O for HTTP responses
- **Synchronization**: Use mutexes to protect shared device lists

### Error Handling

```cpp
// ✅ GOOD: Proper error handling with HTTP status codes
httplib::Server svr;
svr.Get("/midi/inputs", [](const httplib::Request& req, httplib::Response& res) {
    try {
        auto devices = MidiDeviceManager::getInputDevices();
        res.set_content(devices.toJson(), "application/json");
        res.status = 200;
    } catch (const std::exception& e) {
        res.set_content(juce::String("Error: ") + e.what(), "text/plain");
        res.status = 500;
    }
});

// ❌ BAD: Ignoring errors
svr.Get("/midi/inputs", [](const httplib::Request& req, httplib::Response& res) {
    auto devices = MidiDeviceManager::getInputDevices(); // May throw
    res.set_content(devices.toJson(), "application/json");
});
```

## Critical Don'ts for JUCE/MIDI Development

❌ **NEVER allocate memory in MIDI callbacks** - Real-time constraint violation
❌ **NEVER use blocking calls in MIDI callbacks** - Will cause audio glitches
❌ **NEVER forget to close MIDI devices** - Resource leak
❌ **NEVER bypass CMake build system** - Use CMake targets, not manual compilation
❌ **NEVER hardcode platform-specific paths** - Use CMake or JUCE platform abstractions
❌ **NEVER commit build artifacts** - Build directory should be in .gitignore
❌ **NEVER create files larger than 500 lines** - Refactor for modularity
❌ **NEVER ignore JUCE module dependencies** - Ensure all required JUCE modules are linked

## Success Criteria

An AI agent has successfully completed work when:

- ✅ CMake builds successfully on all platforms
- ✅ JUCE modules are properly configured
- ✅ MIDI devices are discovered correctly
- ✅ HTTP endpoints return valid JSON
- ✅ Error handling is comprehensive
- ✅ Code follows JUCE conventions
- ✅ Thread safety is maintained
- ✅ Memory management is correct (no leaks)
- ✅ Files are appropriately sized (under 500 lines)
- ✅ API is documented

## Parent Project Guidelines

This module inherits guidelines from the parent OL_DSP project:
- See `/Users/orion/work/ol_dsp-midi-server/CLAUDE.md` for:
  - DSP-specific guidelines
  - Memory management for real-time audio
  - MIDI timing constraints
  - General project structure and patterns

## When in Doubt

- Review existing JUCE applications in the project
- Check JUCE documentation for API usage
- Follow JUCE tutorials for MIDI and HTTP patterns
- Test on actual MIDI hardware when possible
- Prioritize thread safety and resource cleanup
- Always document make/CMake targets for self-documenting build process
