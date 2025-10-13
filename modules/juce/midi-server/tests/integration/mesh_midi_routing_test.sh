#!/bin/bash
#
# Integration Test: End-to-End MIDI Routing Across Mesh
#
# Tests that MIDI messages sent to a local device on one node
# can be routed to and received by another node in the mesh.
#
# Test Flow:
#   1. Start two network_midi_server instances on different ports
#   2. Wait for mesh formation (mDNS discovery + handshake)
#   3. Verify remote devices are registered (primary mesh indicator)
#   4. Send MIDI via local MIDI device on node 1
#   5. Verify MIDI routing by checking reception on node 2
#   6. Verify network statistics show messages routed

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="/Users/orion/work/ol_dsp-midi-server"
BUILD_DIR="${PROJECT_ROOT}/build"
SERVER="${BUILD_DIR}/modules/juce/midi-server/network_midi_server_artefacts/network_midi_server"
MESH_TEST="${BUILD_DIR}/modules/juce/midi-server/midi_mesh_test_artefacts/midi_mesh_test"

# Test parameters
NODE1_PORT=""
NODE2_PORT=""
NODE1_PID=""
NODE2_PID=""
RECEIVER_PID=""
TIMEOUT=20

# Color output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results
TESTS_PASSED=0
TESTS_FAILED=0
OVERALL_RESULT="PASS"

# Cleanup function
cleanup() {
    echo
    echo -e "${YELLOW}Cleaning up...${NC}"

    if [ -n "$RECEIVER_PID" ] && ps -p $RECEIVER_PID > /dev/null 2>&1; then
        kill $RECEIVER_PID 2>/dev/null || true
    fi

    if [ -n "$NODE1_PID" ] && ps -p $NODE1_PID > /dev/null 2>&1; then
        kill $NODE1_PID 2>/dev/null || true
    fi

    if [ -n "$NODE2_PID" ] && ps -p $NODE2_PID > /dev/null 2>&1; then
        kill $NODE2_PID 2>/dev/null || true
    fi

    sleep 1
}

trap cleanup EXIT

# Test result functions
test_pass() {
    echo -e "${GREEN}✓ $1${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

test_fail() {
    echo -e "${RED}✗ $1${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
    OVERALL_RESULT="FAIL"
}

test_info() {
    echo -e "${BLUE}  $1${NC}"
}

# Wait for mesh formation by polling for remote devices
# This is more reliable than /network/mesh which has state query issues
wait_for_remote_devices() {
    local port=$1
    local node_name="$2"

    echo -e "${YELLOW}Waiting for remote devices on $node_name (up to ${TIMEOUT}s)...${NC}"

    for i in $(seq 1 $TIMEOUT); do
        REMOTE=$(curl -s "http://localhost:$port/midi/devices" 2>/dev/null | \
                 python3 -c "import sys, json; data=json.load(sys.stdin); print(len([d for d in data.get('devices', []) if not d.get('is_local', True)]))" 2>/dev/null || echo "0")

        if [ "$REMOTE" -gt "0" ]; then
            test_pass "Mesh formed - remote devices discovered on $node_name ($REMOTE devices)"
            return 0
        fi

        if [ $((i % 4)) -eq 0 ]; then
            echo -e "${BLUE}  Waiting... ($i/${TIMEOUT}s)${NC}"
        fi

        sleep 1
    done

    test_fail "Mesh formation timeout on $node_name (no remote devices after ${TIMEOUT}s)"
    return 1
}

# Test device discovery with details
test_device_discovery() {
    local port=$1
    local node_name="$2"

    echo
    echo -e "${YELLOW}Verifying device discovery on $node_name...${NC}"

    # Get device counts
    local devices_json=$(curl -s "http://localhost:$port/midi/devices" 2>/dev/null)

    TOTAL=$(echo "$devices_json" | python3 -c "import sys, json; data=json.load(sys.stdin); print(len(data.get('devices', [])))" 2>/dev/null || echo "0")

    LOCAL=$(echo "$devices_json" | python3 -c "import sys, json; data=json.load(sys.stdin); print(len([d for d in data.get('devices', []) if d.get('is_local', False)]))" 2>/dev/null || echo "0")

    REMOTE=$(echo "$devices_json" | python3 -c "import sys, json; data=json.load(sys.stdin); print(len([d for d in data.get('devices', []) if not d.get('is_local', True)]))" 2>/dev/null || echo "0")

    test_info "Total devices: $TOTAL"
    test_info "Local devices: $LOCAL"
    test_info "Remote devices: $REMOTE"

    if [ "$TOTAL" -gt "0" ] && [ "$LOCAL" -gt "0" ] && [ "$REMOTE" -gt "0" ]; then
        test_pass "Device registry populated on $node_name"
        return 0
    else
        test_fail "Incomplete device registry on $node_name"
        return 1
    fi
}

# Test MIDI routing via actual MIDI send/receive
test_midi_routing() {
    local direction="$1"

    echo
    echo -e "${YELLOW}Testing MIDI routing: $direction${NC}"

    # Get initial network stats from node 1
    INITIAL_SENT=$(curl -s "http://localhost:$NODE1_PORT/network/stats" 2>/dev/null | \
                   python3 -c "import sys, json; data=json.load(sys.stdin); print(data.get('network_sent', 0))" 2>/dev/null || echo "0")

    test_info "Initial network messages sent: $INITIAL_SENT"

    # Start MIDI receiver in background (listening on virtual MIDI port)
    echo -e "${BLUE}  Starting MIDI receiver...${NC}"
    "$MESH_TEST" receive > /tmp/mesh_test_receive.log 2>&1 &
    RECEIVER_PID=$!
    sleep 2

    # Send MIDI test messages
    echo -e "${BLUE}  Sending MIDI messages...${NC}"
    "$MESH_TEST" send > /tmp/mesh_test_send.log 2>&1
    sleep 2

    # Stop receiver
    if ps -p $RECEIVER_PID > /dev/null 2>&1; then
        kill $RECEIVER_PID 2>/dev/null || true
    fi
    RECEIVER_PID=""

    # Check if receiver got messages
    if grep -q "SUCCESS" /tmp/mesh_test_receive.log 2>/dev/null; then
        RECEIVED_COUNT=$(grep "Received" /tmp/mesh_test_receive.log 2>/dev/null | wc -l | tr -d ' ')
        test_pass "MIDI messages received ($RECEIVED_COUNT messages)"
    else
        test_fail "No MIDI messages received"
        test_info "Sender output:"
        cat /tmp/mesh_test_send.log | head -10 | sed 's/^/    /'
        test_info "Receiver output:"
        cat /tmp/mesh_test_receive.log | head -10 | sed 's/^/    /'
        return 1
    fi

    # Verify network stats show messages were sent
    sleep 1
    FINAL_SENT=$(curl -s "http://localhost:$NODE1_PORT/network/stats" 2>/dev/null | \
                 python3 -c "import sys, json; data=json.load(sys.stdin); print(data.get('network_sent', 0))" 2>/dev/null || echo "0")

    test_info "Final network messages sent: $FINAL_SENT"

    if [ "$FINAL_SENT" -gt "$INITIAL_SENT" ]; then
        DELTA=$((FINAL_SENT - INITIAL_SENT))
        test_pass "Network statistics updated (sent $DELTA messages)"
    else
        test_fail "Network statistics unchanged (may not have routed through mesh)"
    fi

    return 0
}

#==============================================================================
# MAIN TEST EXECUTION
#==============================================================================

echo "============================================================"
echo "Network MIDI Mesh - End-to-End Routing Integration Test"
echo "============================================================"
echo

# Check prerequisites
echo -e "${YELLOW}Checking prerequisites...${NC}"

if [ ! -f "$SERVER" ]; then
    echo -e "${RED}ERROR: network_midi_server not found at:${NC}"
    echo "  $SERVER"
    echo
    echo "Please build the project first:"
    echo "  cmake --build build"
    exit 1
fi

if [ ! -f "$MESH_TEST" ]; then
    echo -e "${RED}ERROR: midi_mesh_test not found at:${NC}"
    echo "  $MESH_TEST"
    echo
    echo "Please build the project first:"
    echo "  cmake --build build"
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo -e "${RED}ERROR: python3 is required for JSON parsing${NC}"
    exit 1
fi

test_pass "All prerequisites found"

# Cleanup any existing servers
echo
echo -e "${YELLOW}Cleaning up existing servers...${NC}"
pkill -f network_midi_server || true
sleep 1

#==============================================================================
# TEST 1: Start server instances
#==============================================================================

echo
echo -e "${YELLOW}Starting test instances...${NC}"

# Start Node 1
echo -e "${BLUE}  Starting Node 1...${NC}"
"$SERVER" > /tmp/mesh_test_node1.log 2>&1 &
NODE1_PID=$!
sleep 3

# Check if Node 1 started
if ! ps -p $NODE1_PID > /dev/null 2>&1; then
    test_fail "Node 1 failed to start"
    echo "Log output:"
    cat /tmp/mesh_test_node1.log
    exit 1
fi

# Extract Node 1 port
NODE1_PORT=$(grep "HTTP Server bound to port" /tmp/mesh_test_node1.log | awk '{print $NF}')
if [ -z "$NODE1_PORT" ]; then
    test_fail "Could not determine Node 1 HTTP port"
    cat /tmp/mesh_test_node1.log
    exit 1
fi

test_info "Node 1: http://localhost:$NODE1_PORT (PID: $NODE1_PID)"

# Start Node 2
echo -e "${BLUE}  Starting Node 2...${NC}"
"$SERVER" > /tmp/mesh_test_node2.log 2>&1 &
NODE2_PID=$!
sleep 3

# Check if Node 2 started
if ! ps -p $NODE2_PID > /dev/null 2>&1; then
    test_fail "Node 2 failed to start"
    echo "Log output:"
    cat /tmp/mesh_test_node2.log
    exit 1
fi

# Extract Node 2 port
NODE2_PORT=$(grep "HTTP Server bound to port" /tmp/mesh_test_node2.log | awk '{print $NF}')
if [ -z "$NODE2_PORT" ]; then
    test_fail "Could not determine Node 2 HTTP port"
    cat /tmp/mesh_test_node2.log
    exit 1
fi

test_info "Node 2: http://localhost:$NODE2_PORT (PID: $NODE2_PID)"

test_pass "Both nodes started successfully"

#==============================================================================
# TEST 2: Wait for mesh formation
#==============================================================================

echo
wait_for_remote_devices $NODE1_PORT "Node 1" || exit 1

echo
wait_for_remote_devices $NODE2_PORT "Node 2" || exit 1

#==============================================================================
# TEST 3: Verify device discovery
#==============================================================================

test_device_discovery $NODE1_PORT "Node 1" || exit 1
test_device_discovery $NODE2_PORT "Node 2" || exit 1

#==============================================================================
# TEST 4: Test MIDI routing
#==============================================================================

test_midi_routing "Node 1 → Node 2"

#==============================================================================
# TEST SUMMARY
#==============================================================================

echo
echo "============================================================"
echo "TEST SUMMARY"
echo "============================================================"
echo
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo

if [ "$OVERALL_RESULT" = "PASS" ]; then
    echo "============================================================"
    echo -e "${GREEN}RESULT: PASS (all tests passed)${NC}"
    echo "============================================================"
    exit 0
else
    echo "============================================================"
    echo -e "${RED}RESULT: FAIL (some tests failed)${NC}"
    echo "============================================================"
    echo
    echo "Debug information:"
    echo
    echo "Node 1 log (last 30 lines):"
    tail -30 /tmp/mesh_test_node1.log | sed 's/^/  /'
    echo
    echo "Node 2 log (last 30 lines):"
    tail -30 /tmp/mesh_test_node2.log | sed 's/^/  /'
    exit 1
fi
