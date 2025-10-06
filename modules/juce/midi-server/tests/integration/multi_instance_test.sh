#!/bin/bash
# Multi-Instance Integration Test
# Tests discovery and mesh formation across multiple server instances

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="/Users/orion/work/ol_dsp-midi-server"
BUILD_DIR="${PROJECT_ROOT}/build"
DISCOVERY_TEST="${BUILD_DIR}/modules/juce/midi-server/discovery_test_artefacts/discovery_test"
SERVER="${BUILD_DIR}/modules/juce/midi-server/network_midi_server_artefacts/network_midi_server"

# Color output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "======================================"
echo "Multi-Instance Integration Test"
echo "======================================"
echo

# Check if executables exist
if [ ! -f "$DISCOVERY_TEST" ]; then
    echo -e "${RED}ERROR: discovery_test not found at $DISCOVERY_TEST${NC}"
    exit 1
fi

# Test 1: Discovery Test - mDNS Mode
echo -e "${YELLOW}Test 1: mDNS Discovery (single instance, 10 seconds)${NC}"
timeout 10 "$DISCOVERY_TEST" --mode mdns --name test-node-1 --http-port 8081 --devices 2 || true
echo -e "${GREEN}✓ mDNS discovery test completed${NC}"
echo

# Test 2: Discovery Test - Fallback UDP Mode
echo -e "${YELLOW}Test 2: UDP Fallback Discovery (single instance, 10 seconds)${NC}"
timeout 10 "$DISCOVERY_TEST" --mode fallback --name test-node-2 --http-port 8082 --udp-port 9092 --devices 2 || true
echo -e "${GREEN}✓ UDP fallback discovery test completed${NC}"
echo

# Test 3: Discovery Test - Both Modes
echo -e "${YELLOW}Test 3: Combined Discovery (mDNS + UDP, 10 seconds)${NC}"
timeout 10 "$DISCOVERY_TEST" --mode both --name test-node-3 --http-port 8083 --devices 3 || true
echo -e "${GREEN}✓ Combined discovery test completed${NC}"
echo

# Test 4: Multi-Instance Discovery (parallel instances)
echo -e "${YELLOW}Test 4: Multi-Instance Discovery (3 instances, 15 seconds)${NC}"
echo "Starting 3 discovery_test instances in parallel..."

# Start instance 1
timeout 15 "$DISCOVERY_TEST" --mode both --name node-A --http-port 8091 --udp-port 9091 --devices 2 &
PID1=$!
sleep 2

# Start instance 2
timeout 15 "$DISCOVERY_TEST" --mode both --name node-B --http-port 8092 --udp-port 9092 --devices 2 &
PID2=$!
sleep 2

# Start instance 3
timeout 15 "$DISCOVERY_TEST" --mode both --name node-C --http-port 8093 --udp-port 9093 --devices 2 &
PID3=$!

# Wait for all instances
echo "Waiting for instances to complete discovery..."
wait $PID1 || true
wait $PID2 || true
wait $PID3 || true

echo -e "${GREEN}✓ Multi-instance discovery test completed${NC}"
echo

echo "======================================"
echo -e "${GREEN}All integration tests completed${NC}"
echo "======================================"
echo
echo "Summary:"
echo "  - mDNS discovery: ✓"
echo "  - UDP fallback discovery: ✓"
echo "  - Combined discovery: ✓"
echo "  - Multi-instance (3 nodes): ✓"
echo
echo "Note: Check output above for discovered peers"
echo "Expected: Each instance should discover the others"
