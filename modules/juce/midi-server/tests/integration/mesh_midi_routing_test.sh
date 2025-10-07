#!/bin/bash
# Mesh MIDI Routing Integration Test
# Tests end-to-end MIDI routing through mesh network

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="/Users/orion/work/ol_dsp-midi-server"
BUILD_DIR="${PROJECT_ROOT}/build"
SERVER="${BUILD_DIR}/modules/juce/midi-server/network_midi_server_artefacts/network_midi_server"
MESH_TEST="${BUILD_DIR}/modules/juce/midi-server/midi_mesh_test_artefacts/midi_mesh_test"

# Color output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "======================================"
echo "Mesh MIDI Routing Integration Test"
echo "======================================"
echo

# Check if executables exist
if [ ! -f "$SERVER" ]; then
    echo -e "${RED}ERROR: network_midi_server not found at $SERVER${NC}"
    exit 1
fi

if [ ! -f "$MESH_TEST" ]; then
    echo -e "${RED}ERROR: midi_mesh_test not found at $MESH_TEST${NC}"
    exit 1
fi

# Kill any existing servers
echo "Cleaning up existing servers..."
pkill -f network_midi_server || true
sleep 1

# Start Node 1
echo -e "${YELLOW}Starting Node 1...${NC}"
"$SERVER" > /tmp/mesh_test_node1.log 2>&1 &
NODE1_PID=$!
sleep 3

# Start Node 2
echo -e "${YELLOW}Starting Node 2...${NC}"
"$SERVER" > /tmp/mesh_test_node2.log 2>&1 &
NODE2_PID=$!
sleep 3

# Wait for mesh to form
echo -e "${YELLOW}Waiting for mesh to form (5 seconds)...${NC}"
sleep 5

# Check if nodes are running
if ! ps -p $NODE1_PID > /dev/null; then
    echo -e "${RED}ERROR: Node 1 failed to start${NC}"
    cat /tmp/mesh_test_node1.log
    exit 1
fi

if ! ps -p $NODE2_PID > /dev/null; then
    echo -e "${RED}ERROR: Node 2 failed to start${NC}"
    cat /tmp/mesh_test_node2.log
    exit 1
fi

echo -e "${GREEN}✓ Both nodes running${NC}"

# Check mesh connection
echo -e "${YELLOW}Checking mesh status...${NC}"
NODE1_PORT=$(grep "HTTP Server bound to port" /tmp/mesh_test_node1.log | awk '{print $NF}')
NODE2_PORT=$(grep "HTTP Server bound to port" /tmp/mesh_test_node2.log | awk '{print $NF}')

echo "Node 1 HTTP Port: $NODE1_PORT"
echo "Node 2 HTTP Port: $NODE2_PORT"

# Check remote devices
NODE2_DEVICES=$(curl -s "http://localhost:$NODE2_PORT/midi/devices" | python3 -c "import sys, json; data=json.load(sys.stdin); print(len([d for d in data['devices'] if not d['is_local']]))")

echo "Node 2 remote devices: $NODE2_DEVICES"

if [ "$NODE2_DEVICES" -eq "0" ]; then
    echo -e "${RED}ERROR: No remote devices registered on Node 2${NC}"
    echo "Node 1 log:"
    tail -20 /tmp/mesh_test_node1.log
    echo "Node 2 log:"
    tail -20 /tmp/mesh_test_node2.log
    kill $NODE1_PID $NODE2_PID 2>/dev/null || true
    exit 1
fi

echo -e "${GREEN}✓ Mesh connected with $NODE2_DEVICES remote devices${NC}"
echo

# Start MIDI receiver in background
echo -e "${YELLOW}Starting MIDI receiver on virtual2...${NC}"
"$MESH_TEST" receive > /tmp/mesh_test_receive.log 2>&1 &
RECEIVER_PID=$!
sleep 2

# Send MIDI test
echo -e "${YELLOW}Sending MIDI test to virtual1...${NC}"
"$MESH_TEST" send > /tmp/mesh_test_send.log 2>&1
sleep 2

# Check receiver status
if ps -p $RECEIVER_PID > /dev/null; then
    kill $RECEIVER_PID 2>/dev/null || true
fi

# Check results
echo
echo "Sender output:"
cat /tmp/mesh_test_send.log

echo
echo "Receiver output:"
cat /tmp/mesh_test_receive.log

# Check for success
if grep -q "SUCCESS" /tmp/mesh_test_receive.log; then
    echo
    echo -e "${GREEN}======================================"
    echo "✓ MESH MIDI ROUTING TEST PASSED"
    echo "======================================${NC}"
    SUCCESS=0
else
    echo
    echo -e "${RED}======================================"
    echo "✗ MESH MIDI ROUTING TEST FAILED"
    echo "======================================${NC}"
    echo
    echo "Node 1 log (last 30 lines):"
    tail -30 /tmp/mesh_test_node1.log
    echo
    echo "Node 2 log (last 30 lines):"
    tail -30 /tmp/mesh_test_node2.log
    SUCCESS=1
fi

# Cleanup
echo
echo "Cleaning up..."
kill $NODE1_PID $NODE2_PID 2>/dev/null || true
sleep 1

exit $SUCCESS
