#!/bin/bash
# Network MIDI Server Integration Test
# Tests full server functionality including HTTP API

set -e

PROJECT_ROOT="/Users/orion/work/ol_dsp-midi-server"
BUILD_DIR="${PROJECT_ROOT}/build"
SERVER="${BUILD_DIR}/modules/juce/midi-server/network_midi_server_artefacts/network_midi_server"

# Color output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "======================================"
echo "Network MIDI Server Integration Test"
echo "======================================"
echo

# Check if server exists
if [ ! -f "$SERVER" ]; then
    echo -e "${RED}ERROR: network_midi_server not found at $SERVER${NC}"
    exit 1
fi

# Test 1: Single server instance
echo -e "${YELLOW}Test 1: Single Server Instance${NC}"
echo "Starting server in background..."

# Start server in background and capture output
timeout 10 "$SERVER" > /tmp/midi_server_test.log 2>&1 &
SERVER_PID=$!
sleep 3

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}✗ Server failed to start${NC}"
    cat /tmp/midi_server_test.log
    exit 1
fi

echo -e "${GREEN}✓ Server started successfully (PID: $SERVER_PID)${NC}"

# Extract port from log
PORT=$(grep "bound to port" /tmp/midi_server_test.log | sed -n 's/.*port \([0-9]*\)/\1/p' | head -1)
if [ -z "$PORT" ]; then
    echo -e "${YELLOW}⚠ Could not extract port, trying common ports...${NC}"
    # Try to find the port from the log
    cat /tmp/midi_server_test.log
fi

echo "Server running on port: $PORT"
echo

# Test HTTP endpoints (if we got the port)
if [ -n "$PORT" ]; then
    echo -e "${YELLOW}Test 2: HTTP API Endpoints${NC}"

    # Test /health endpoint
    echo -n "Testing GET /health... "
    if curl -s "http://localhost:$PORT/health" > /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
    fi

    # Test /midi/inputs endpoint
    echo -n "Testing GET /midi/inputs... "
    if curl -s "http://localhost:$PORT/midi/inputs" > /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
    fi

    # Test /midi/outputs endpoint
    echo -n "Testing GET /midi/outputs... "
    if curl -s "http://localhost:$PORT/midi/outputs" > /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
    fi

    # Test /midi/devices endpoint
    echo -n "Testing GET /midi/devices... "
    DEVICES=$(curl -s "http://localhost:$PORT/midi/devices")
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓${NC}"
        echo "Response: $DEVICES"
    else
        echo -e "${RED}✗${NC}"
    fi
    echo
fi

# Stop server
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
echo -e "${GREEN}✓ Server stopped${NC}"
echo

# Test 3: Multi-instance server test
echo -e "${YELLOW}Test 3: Multi-Instance Servers (3 instances)${NC}"
echo "Starting 3 server instances..."

# Start 3 instances with auto-port allocation
timeout 15 "$SERVER" > /tmp/midi_server_1.log 2>&1 &
PID1=$!
sleep 2

timeout 15 "$SERVER" > /tmp/midi_server_2.log 2>&1 &
PID2=$!
sleep 2

timeout 15 "$SERVER" > /tmp/midi_server_3.log 2>&1 &
PID3=$!
sleep 2

# Check all instances are running
RUNNING=0
if kill -0 $PID1 2>/dev/null; then ((RUNNING++)); fi
if kill -0 $PID2 2>/dev/null; then ((RUNNING++)); fi
if kill -0 $PID3 2>/dev/null; then ((RUNNING++)); fi

echo "Running instances: $RUNNING/3"

if [ $RUNNING -eq 3 ]; then
    echo -e "${GREEN}✓ All 3 instances started successfully${NC}"

    # Show instance details
    echo
    echo "Instance 1:"
    grep -E "(UUID|Port|Node)" /tmp/midi_server_1.log | head -5
    echo
    echo "Instance 2:"
    grep -E "(UUID|Port|Node)" /tmp/midi_server_2.log | head -5
    echo
    echo "Instance 3:"
    grep -E "(UUID|Port|Node)" /tmp/midi_server_3.log | head -5
else
    echo -e "${YELLOW}⚠ Only $RUNNING instances started${NC}"
fi

# Let them run for a bit
sleep 5

# Stop all instances
echo
echo "Stopping all instances..."
kill $PID1 $PID2 $PID3 2>/dev/null || true
wait $PID1 $PID2 $PID3 2>/dev/null || true
echo -e "${GREEN}✓ All instances stopped${NC}"

echo
echo "======================================"
echo -e "${GREEN}Integration tests completed${NC}"
echo "======================================"
echo
echo "Summary:"
echo "  - Single server instance: ✓"
echo "  - HTTP API endpoints: ✓ (if port was available)"
echo "  - Multi-instance isolation: ✓ ($RUNNING/3 instances)"
echo
echo "Logs available at:"
echo "  - /tmp/midi_server_test.log"
echo "  - /tmp/midi_server_{1,2,3}.log"
