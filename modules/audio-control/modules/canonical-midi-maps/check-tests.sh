#!/bin/bash
set -e

echo "=== Running tests for canonical-midi-maps module ==="
cd /Users/orion/work/ol_dsp/modules/audio-control/modules/canonical-midi-maps

# Run tests with verbose output
echo "Running tests..."
pnpm test --run --reporter=verbose 2>&1 || true

echo ""
echo "Test run complete."