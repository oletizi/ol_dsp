#!/bin/bash
# Step 4: Build the project
# This script runs the actual build

set -e

echo "📁 Step 4: Building project..."

cd /workspace

if [ ! -d "cmake-build" ]; then
    echo "❌ cmake-build directory not found. Run step 3 first!"
    exit 1
fi

cd cmake-build

# Run make with timing
echo "Running make..."
echo "----------------------------------------"

start_time=$(date +%s)
make -j$(nproc)
end_time=$(date +%s)

echo "----------------------------------------"
echo "Build took $((end_time - start_time)) seconds"

# Check what was built
echo ""
echo "Build artifacts:"
find . -type f -executable -name "*.so" -o -name "*.a" 2>/dev/null | head -20

echo ""
echo "✅ Project build complete"