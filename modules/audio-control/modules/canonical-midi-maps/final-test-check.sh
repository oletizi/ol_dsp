#!/bin/bash
set -e

echo "=== Final test verification for canonical-midi-maps ==="
cd /Users/orion/work/ol_dsp/modules/audio-control/modules/canonical-midi-maps

echo "1. Type checking..."
pnpm typecheck

echo ""
echo "2. Running all tests..."
pnpm test --run

echo ""
echo "3. Build check..."
pnpm build

echo ""
echo "✅ All checks completed successfully!"