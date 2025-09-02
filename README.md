# OL_DSP: Digital Signal Processing Toolkit

[![CI](https://github.com/oletizi/ol_dsp/workflows/CI/badge.svg)](https://github.com/oletizi/ol_dsp/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C++-11%2B-blue.svg)](https://isocpp.org/)
[![TypeScript](https://img.shields.io/badge/TypeScript-5.6+-3178c6.svg)](https://www.typescriptlang.org/)
[![Arduino](https://img.shields.io/badge/Arduino-Compatible-00979D.svg)](https://www.arduino.cc/)
[![JUCE](https://img.shields.io/badge/JUCE-Framework-orange.svg)](https://juce.com/)

An eclectic collection of digital signal processing tools, audio effects, and hardware sampler utilities for Arduino microcontrollers and desktop applications.

## Overview

OL_DSP is an experimental DSP project that bridges the gap between embedded audio processing and professional audio hardware. It combines:

- **Embedded DSP**: Real-time audio processing algorithms for Arduino and other microcontrollers
- **Hardware Sampler Support**: Tools for working with classic Akai samplers (S1000, S5000/S6000)
- **Audio Effects**: Various effect implementations including chorus, pitch shifting, and more
- **Plugin Development**: JUCE-based plugin host and RNBO effect integration
- **MIDI Integration**: Comprehensive MIDI communication for hardware control

## Features

- 🎛️ Real-time audio effects (chorus, pitch shift, reverb, etc.)
- 🎹 MIDI-controlled parameter automation
- 💾 Akai sampler format support (S1000, S5000/S6000)
- 🔌 JUCE plugin hosting capabilities
- 🎵 Max/MSP gen~ patch integration via RNBO
- ⚡ Optimized for embedded systems with limited resources
- 🔊 Support for multiple sample rates and bit depths

## Project Structure

```
ol_dsp/
├── modules/           # Core modules and libraries
│   ├── audio-tools/   # JavaScript/TypeScript audio tools and sampler utilities
│   ├── corelib/       # Core DSP library functions
│   ├── ctllib/        # Control library
│   ├── fxlib/         # Audio effects library
│   ├── iolib/         # I/O library
│   ├── juce/          # JUCE-based plugin host
│   └── rnbo/          # RNBO patches and exports
├── workouts/          # Test programs and experiments
├── test/              # Unit tests
├── build/             # Build output directory
└── libs/              # External libraries (submodules)
```

## Getting Started

### Prerequisites

- **For Arduino/Embedded Development**:
  - Arduino IDE or Arduino CLI
  - Compatible Arduino board (Teensy recommended for audio applications)
  
- **For Desktop Development**:
  - C++ compiler (GCC 9+ or Clang 10+)
  - CMake 3.15+
  - JUCE framework (for plugin development)
  - Node.js 16+ (for TypeScript/JavaScript components)

- **Optional**:
  - Max/MSP 8+ with RNBO (for gen~ patches)
  - MIDI hardware for testing

### Installation

1. **Clone the repository**:
```bash
git clone https://github.com/yourusername/ol_dsp.git
cd ol_dsp
```

2. **Install dependencies**:

Install all JavaScript/TypeScript dependencies (including audio-tools workspace):
```bash
npm install  # This will install dependencies for all workspaces
```

3. **Build the project**:

Build C++ components:
```bash
make        # Builds everything via CMake
make test   # Build and run tests
make clean  # Clean build artifacts
```

For other components, check individual directories for build instructions.

### Working with Audio-Tools (JavaScript/TypeScript)

The `modules/audio-tools` workspace contains JavaScript/TypeScript tools for working with hardware samplers, MIDI devices, and audio file formats.

```bash
# Run the audio-tools development server
npm run dev

# Run tests for audio-tools
npm test

# Build audio-tools
npm run build:audio-tools
```

### Quick Start Examples

#### Running a Simple Effect on Arduino

```cpp
// Example: Basic chorus effect on Arduino
#include "effects/chorus.h"

ChorusEffect chorus;

void setup() {
  chorus.init(44100);  // Initialize at 44.1kHz
  chorus.setDepth(0.5);
  chorus.setRate(2.0);
}

void loop() {
  float sample = readAudioInput();
  float processed = chorus.process(sample);
  writeAudioOutput(processed);
}
```

#### Working with Akai Sampler Files

```javascript
// Example: Reading Akai S1000 sample data
const { AkaiS1000 } = require('./samplers/akai');

const sampler = new AkaiS1000();
const sampleData = await sampler.loadSample('path/to/sample.akai');
console.log(`Loaded ${sampleData.name}: ${sampleData.sampleRate}Hz, ${sampleData.bitDepth}-bit`);
```

#### Using the JUCE Plugin Host

```bash
# List available plugins
./cmake-build/modules/juce/host/plughost_artefacts/plughost --list

# Load and test a plugin (after building with make plughost)
./cmake-build/modules/juce/host/plughost_artefacts/plughost --load path/to/plugin.vst3
```

## Development

### Running Tests

```bash
# Run all tests
make test
npm test

# Run specific test suites
make test-dsp
npm run test:samplers
```

### Code Style

- C/C++: Follow the existing code style, use clang-format
- TypeScript: Use ESLint and Prettier configurations provided
- Keep functions focused and under 50 lines when possible
- Document complex DSP algorithms with comments

### Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-effect`)
3. Commit your changes (`git commit -m 'Add amazing effect'`)
4. Push to the branch (`git push origin feature/amazing-effect`)
5. Open a Pull Request

## Performance Considerations

- **Memory**: Designed for constrained environments (as low as 2KB RAM for basic effects)
- **Latency**: Optimized for real-time processing (<10ms round-trip on supported hardware)
- **Sample Rates**: Supports 22.05kHz to 96kHz
- **Bit Depths**: 16-bit, 24-bit, and 32-bit float processing

## Hardware Compatibility

### Tested Microcontrollers
- Arduino Uno (limited features)
- Arduino Due
- Teensy 3.x/4.x (recommended)
- ESP32 (with I2S audio)
- STM32F4 series

### Supported Samplers
- Akai S1000
- Akai S5000
- Akai S6000
- Generic MIDI samplers (via MIDI implementation)

## Resources

- [DSP Theory and Implementation Guide](docs/dsp-theory.md)
- [MIDI Implementation Chart](docs/midi-implementation.md)
- [Akai Format Specifications](docs/akai-formats.md)
- [Performance Tuning Guide](docs/performance.md)

## Dependencies

- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) - CMake package manager
- JUCE Framework - Audio plugin development
- RNBO - Max/MSP code export
- Arduino Audio Library - For Teensy audio processing

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Max/MSP community for gen~ patches and DSP knowledge
- JUCE framework developers
- Arduino audio library contributors
- Akai sampler reverse-engineering community

## Status

🚧 **Experimental Project** - This is an ongoing experimental project. APIs may change, and some features are still in development.

## Contact

For questions, bug reports, or collaboration opportunities, please open an issue on GitHub.