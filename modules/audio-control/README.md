# Audio Control

A monorepo for programmatic MIDI controller mapping and audio plugin parameter management.

## Purpose

Audio Control provides tools and libraries for creating accurate, maintainable MIDI mappings between hardware controllers and audio plugins/DAWs. It solves the fundamental problem of controller mapping: ensuring that physical controls map to the correct plugin parameters with proper ranges and behaviors.

## Components

- **`canonical-midi-maps`** - Device-specific MIDI controller mappings with real plugin parameter indices
- **`ardour-midi-maps`** - TypeScript library for generating Ardour DAW MIDI configuration files
- **Plugin interrogation tools** - Extract accurate parameter information from VST/AU plugins using JUCE

## Key Features

- 🎛️ Accurate plugin parameter extraction via a custom plugin loader
- 🎹 Hardware controller abstraction layers
- 📄 DAW-specific format generation (currently Ardour, more coming)
- 🔧 Programmatic mapping creation with TypeScript
- 📊 Parameter categorization and semantic grouping

## Architecture

See [docs/ARCHITECTURE.md](./docs/ARCHITECTURE.md) for detailed information about how plugin descriptors, canonical MIDI maps, and DAW-specific mappings work together.

## Quick Start

```bash
# Install dependencies
pnpm install

# Run complete workflow (extract → validate → generate)
pnpm workflow:complete

# Or run individual phases:
# Phase 1: Extract plugin parameters
pnpm plugins:extract

# Phase 2: Validate canonical mappings
pnpm maps:validate

# Phase 3: Generate DAW-specific formats
pnpm daw:generate

# Build libraries
pnpm build
```

## Available Scripts

### Workflow Management
```bash
pnpm workflow:complete            # Complete workflow (extract → validate → generate)
pnpm workflow:health              # System health check across all phases
```

### Phase 1: Plugin Interrogation
```bash
pnpm plugins:extract              # Extract plugin parameters
pnpm plugins:extract:force        # Force re-extraction (bypass cache)
pnpm plugins:list                 # List available plugins
pnpm plugins:health               # Validate extracted descriptors
```

### Phase 2: Canonical Mapping
```bash
pnpm maps:validate                # Validate canonical mapping files
pnpm maps:list                    # List available canonical mappings
pnpm maps:check                   # Health check (validate against descriptors)
```

### Phase 3: DAW Generation
```bash
pnpm daw:generate                 # Generate all DAW formats
pnpm daw:generate:ardour          # Generate Ardour only
pnpm daw:generate:ardour:install  # Generate and install to Ardour
pnpm daw:list                     # List generated DAW files
```

## Workflow Process

See [docs/PROCESS.md](./docs/PROCESS.md) for the complete workflow documentation.

## License

MIT