#!/bin/bash
# Step 3: Run CMake configuration
# This script runs cmake configuration and shows what it's doing with JUCE

set -e

echo "📁 Step 3: Running CMake configuration..."

cd /workspace

# Create build directory
echo "Creating cmake-build directory..."
mkdir -p cmake-build
cd cmake-build

# Run cmake with verbose output for JUCE
echo "Running cmake..."
echo "This should use pre-built JUCE if available..."
echo "----------------------------------------"

# Time the cmake configuration
start_time=$(date +%s)
cmake ../ 2>&1 | tee cmake_output.log
end_time=$(date +%s)

echo "----------------------------------------"
echo "CMake configuration took $((end_time - start_time)) seconds"

# Check if juceaide was actually rebuilt
echo ""
echo "Checking if juceaide was rebuilt..."
if grep -q "Building juceaide" cmake_output.log; then
    echo "⚠️  CMake says 'Building juceaide' - but let's check if it actually rebuilt..."
    
    # Check modification time of juceaide
    if find /workspace/libs/JUCE/build -name "juceaide" -type f -mmin -2 2>/dev/null | grep -q .; then
        echo "❌ juceaide was modified in the last 2 minutes - it was REBUILT!"
    else
        echo "✅ juceaide was NOT modified recently - cache was used!"
    fi
else
    echo "✅ CMake did not mention building juceaide"
fi

# Show generated files
echo ""
echo "Generated build files:"
ls -la | head -10

echo ""
echo "✅ CMake configuration complete"