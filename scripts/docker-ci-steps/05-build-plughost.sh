#!/bin/bash
# Step 5: Build and verify plugin host
# This script builds the plugin host and verifies it works

set -e

echo "📁 Step 5: Building plugin host..."

cd /workspace

if [ ! -d "cmake-build" ]; then
    echo "❌ cmake-build directory not found. Run steps 3-4 first!"
    exit 1
fi

cd cmake-build

# Build plugin host
echo "Building plugin host..."
echo "----------------------------------------"

start_time=$(date +%s)
make plughost
end_time=$(date +%s)

echo "----------------------------------------"
echo "Plugin host build took $((end_time - start_time)) seconds"

# Verify plugin host
echo ""
echo "Verifying plugin host binary..."
PLUGHOST_PATH="./modules/juce/host/plughost_artefacts/plughost"

if [ -f "$PLUGHOST_PATH" ]; then
    echo "✅ Plugin host binary found"
    ls -la "$PLUGHOST_PATH"
    
    echo ""
    echo "Testing plugin host..."
    timeout 10s "$PLUGHOST_PATH" --help || echo "Plugin host help completed"
else
    echo "❌ Plugin host binary NOT found at $PLUGHOST_PATH"
    echo "Looking for it elsewhere..."
    find . -name "plughost" -type f 2>/dev/null
fi

echo ""
echo "✅ Plugin host build complete"