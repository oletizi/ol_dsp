#!/bin/bash
# Step 2: Verify pre-built dependencies
# This script checks that all pre-built dependencies are in place

set -e

echo "📁 Step 2: Verifying pre-built dependencies..."

cd /workspace

# Check for pre-built JUCE
echo "Checking for JUCE..."
if [ -d "libs/JUCE/build" ]; then
    echo "✅ JUCE build directory found at libs/JUCE/build"
    echo "  Contents:"
    ls -la libs/JUCE/build/ | head -10
    
    # Check for juceaide specifically
    if find libs/JUCE/build -name "juceaide" -type f | grep -q .; then
        echo "✅ juceaide binary found:"
        find libs/JUCE/build -name "juceaide" -type f -exec ls -la {} \;
    else
        echo "❌ juceaide binary NOT found in JUCE build directory"
    fi
else
    echo "❌ JUCE build directory NOT found at libs/JUCE/build"
fi

# Check for other pre-built dependencies
echo ""
echo "Checking for other pre-built dependencies..."

for dep in "libs/stk/build" "libs/rtmidi/build" "test/googletest/build" "test/FakeIt/build"; do
    if [ -d "$dep" ]; then
        echo "✅ Found $dep"
    else
        echo "⚠️  Not found: $dep (may not be pre-built)"
    fi
done

# Check directory structure
echo ""
echo "libs/ directory structure:"
ls -la libs/ | head -15

echo ""
echo "test/ directory structure:"
ls -la test/ | head -10

echo ""
echo "✅ Dependency verification complete"