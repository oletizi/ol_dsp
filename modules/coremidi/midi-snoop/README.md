# midisnoop

A CLI tool for **passive monitoring** of all CoreMIDI traffic on macOS - spy on MIDI conversations between devices without rewiring them.

## Features

- ✅ **Passive monitoring of MIDI sources (inputs)** - works out of the box
- ✅ **Passive monitoring of MIDI destinations (outputs)** - built-in SnoizeMIDISpy driver
- ✅ **No rewiring required** - monitors existing MIDI connections transparently
- ✅ **Color-coded output** - green for sources, blue for destinations
- ✅ **Decodes MIDI messages** - shows message types and channel info
- ✅ **Hex dump** - displays raw MIDI bytes

## Quick Start

```bash
make build          # Build midisnoop and SnoizeMIDISpy driver
make install-driver # Install driver to ~/Library/Audio/MIDI Drivers
make run            # Run midisnoop
```

The first time you run `midisnoop`, it will automatically install the SnoizeMIDISpy driver if needed.

## How It Works

### Source Monitoring (Always Available)
midisnoop connects to all MIDI sources using standard CoreMIDI APIs:
- `MIDIInputPortCreate()` creates an input port
- `MIDIPortConnectSource()` connects to each source
- Callbacks receive MIDI packets passively

### Destination Monitoring (Built-in Driver)
Passive destination monitoring requires a CoreMIDI driver plugin that intercepts system-level MIDI traffic:
- The **SpyingMIDIDriver** implements the `MIDIDriverInterface` with a `Monitor` callback
- The driver calls `MIDIDriverEnableMonitoring(true)` to receive ALL outgoing MIDI packets
- CoreMIDI server routes packets to the driver transparently
- The driver broadcasts packets to midisnoop via message ports
- **No rewiring required** - existing MIDI connections work normally

This project includes a build of the open-source [SnoizeMIDISpy driver](https://github.com/krevis/MIDIApps) from MIDI Monitor.

## Output Format

```
[SOURCE] m4-mini : Note On (ch 1) : 90 3C 7F
[SOURCE] Launch Control XL : Control Change (ch 1) : B0 0D 42
[DESTINATION] DAW In : Note Off (ch 1) : 80 3C 00
```

## Build Commands

```bash
make build          # Compile midisnoop and driver
make install-driver # Install driver to ~/Library/Audio/MIDI Drivers
make run            # Build and run
make clean          # Remove build artifacts
make help           # Show all targets
```

## Project Structure

```
midi-snoop/
├── midisnoop.mm              # Main CLI application
├── snoize-spy/               # SnoizeMIDISpy driver and framework
│   ├── driver/               # CoreMIDI driver plugin sources
│   ├── framework/            # Client framework for connecting to driver
│   └── CMakeLists.txt        # Driver build configuration
├── CMakeLists.txt            # Main build configuration
├── Makefile                  # Convenience targets
└── README.md
```

## Requirements

- macOS 10.13+
- CMake 3.15+
- Xcode Command Line Tools

## Dependencies

The SnoizeMIDISpy driver source is automatically fetched from the [MIDIApps](https://github.com/krevis/MIDIApps) repository via the ol_dsp dependency management system.

## License

- **midisnoop CLI**: BSD-style (matches ol_dsp project)
- **SnoizeMIDISpy driver**: BSD-style (from [MIDIApps](https://github.com/krevis/MIDIApps))

## References

- [MIDI Monitor](https://www.snoize.com/MIDIMonitor/) - GUI MIDI monitor with spy driver
- [MIDIApps Source](https://github.com/krevis/MIDIApps) - Open source implementation
- [CoreMIDI Documentation](https://developer.apple.com/documentation/coremidi/)