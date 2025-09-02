#!/bin/bash

# build-images-locally.sh  
# Quick script to build Docker images locally for testing
# Useful for testing Docker changes without full CI simulation

set -e

# Configuration - can be overridden with environment variables
PLATFORM="${DOCKER_PLATFORM:-linux/arm64}"  # Use ARM64 for local macOS testing
BASE_IMAGE="${BASE_IMAGE:-ol_dsp/base:local}"
CPP_IMAGE="${CPP_IMAGE:-ol_dsp/cpp-builder:local}"
NODE_IMAGE="${NODE_IMAGE:-ol_dsp/node-builder:local}"
FORCE_REBUILD="${FORCE_REBUILD:-false}"

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "🐳 Building Docker images locally..."
echo "Platform: $PLATFORM"
echo "Base image: $BASE_IMAGE" 
echo "C++ image: $CPP_IMAGE"
echo "Node image: $NODE_IMAGE"
echo "Force rebuild: $FORCE_REBUILD"
echo

cd "$PROJECT_ROOT"

# Function to check if image exists locally
image_exists() {
    docker images --format "table {{.Repository}}:{{.Tag}}" | grep -q "$1" 2>/dev/null
}

# Build base image
if [ "$FORCE_REBUILD" = "true" ] || ! image_exists "$BASE_IMAGE"; then
    echo "🔨 Building base image ($BASE_IMAGE)..."
    docker build --platform "$PLATFORM" -f .docker/base.Dockerfile -t "$BASE_IMAGE" .
    echo "✅ Base image built: $BASE_IMAGE"
else
    echo "⏩ Base image $BASE_IMAGE already exists, skipping"
fi

# Build C++ image (rebuild if base was rebuilt or doesn't exist)
if [ "$FORCE_REBUILD" = "true" ] || ! image_exists "$CPP_IMAGE"; then
    echo "🔨 Building C++ image ($CPP_IMAGE)..."
    # Create temporary Dockerfile that uses our local base image
    sed "s|ghcr.io/\${GITHUB_REPOSITORY}/base:latest|$BASE_IMAGE|g" .docker/cpp-builder.Dockerfile > /tmp/cpp-builder-local.Dockerfile
    docker build --platform "$PLATFORM" -f /tmp/cpp-builder-local.Dockerfile -t "$CPP_IMAGE" .
    rm /tmp/cpp-builder-local.Dockerfile
    echo "✅ C++ image built: $CPP_IMAGE"
else
    echo "⏩ C++ image $CPP_IMAGE already exists, skipping" 
fi

# Build Node image
if [ "$FORCE_REBUILD" = "true" ] || ! image_exists "$NODE_IMAGE"; then
    echo "🔨 Building Node image ($NODE_IMAGE)..."
    docker build --platform "$PLATFORM" -f .docker/node-builder.Dockerfile -t "$NODE_IMAGE" .
    echo "✅ Node image built: $NODE_IMAGE"
else
    echo "⏩ Node image $NODE_IMAGE already exists, skipping"
fi

echo
echo "📊 Image Summary:"
docker images | grep "ol_dsp" | head -10

echo
echo "🎉 All images built successfully!"
echo
echo "💡 Usage:"
echo "  • Test C++ build: docker run --rm -v \$(pwd):/workspace -w /workspace $CPP_IMAGE make"
echo "  • Test Node build: docker run --rm -v \$(pwd):/workspace -w /workspace $NODE_IMAGE npm test"  
echo "  • Run full local CI: ./scripts/run-ci-locally.sh"
echo "  • Force rebuild all: FORCE_REBUILD=true ./scripts/build-images-locally.sh"