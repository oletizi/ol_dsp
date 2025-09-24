# Docker Build Environment

This directory contains Docker configurations to speed up CI builds and provide consistent development environments.

## Images

### cpp-builder.Dockerfile
- **Base**: Ubuntu 24.04
- **Purpose**: C++ builds with JUCE dependencies
- **Includes**: CMake, build tools, JUCE system dependencies
- **Size**: ~500MB (vs 2+ minutes of apt installs)

### node-builder.Dockerfile  
- **Base**: Node.js 20 slim
- **Purpose**: npm workspace builds
- **Includes**: Node 20, build tools for native modules
- **Size**: ~300MB (vs 1+ minute of npm setup)

## Usage

### Local Development

```bash
# Start all services
docker-compose up -d

# Build C++ in container
docker-compose exec cpp-builder make

# Run npm commands in container
docker-compose exec node-builder npm test

# Use combined dev environment
docker-compose exec dev bash
```

### CI/CD

The GitHub Actions workflow automatically:
1. Builds and pushes images to GitHub Container Registry
2. Uses cached images for subsequent builds
3. Falls back to macOS native builds for audio testing

### Manual Docker Commands

```bash
# Build images locally
docker build -f .docker/cpp-builder.Dockerfile -t ol_dsp-cpp .
docker build -f .docker/node-builder.Dockerfile -t ol_dsp-node .

# Run builds in containers
docker run --rm -v $(pwd):/workspace ol_dsp-cpp make
docker run --rm -v $(pwd):/workspace ol_dsp-node npm ci
```

## Benefits

- **Speed**: Dependencies pre-installed, ~2-3x faster CI
- **Consistency**: Same environment across development and CI
- **Caching**: Docker layer caching reduces rebuild times
- **Isolation**: Clean, reproducible builds

## Registry

Images are automatically published to:
- `ghcr.io/oletizi/ol_dsp/cpp-builder:latest`
- `ghcr.io/oletizi/ol_dsp/node-builder:latest`