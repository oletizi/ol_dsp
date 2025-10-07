#!/bin/bash
# routing_api_test.sh
#
# Integration test for Routing Configuration API (Phase 2)
# Tests the HTTP REST API endpoints for managing routing rules
#
# Usage: ./routing_api_test.sh [server_port]
#
# Prerequisites:
# - network_midi_server must be built
# - curl must be installed
# - jq must be installed (for JSON parsing)

set -e  # Exit on error

# Configuration
SERVER_PORT=${1:-8080}
SERVER_HOST="localhost"
BASE_URL="http://${SERVER_HOST}:${SERVER_PORT}"
SERVER_BIN="./build/network_midi_server"
SERVER_PID=""
TEST_RESULTS=()
FAILED_TESTS=0
PASSED_TESTS=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Utility functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

test_pass() {
    echo -e "${GREEN}✓${NC} $1"
    TEST_RESULTS+=("PASS: $1")
    ((PASSED_TESTS++))
}

test_fail() {
    echo -e "${RED}✗${NC} $1"
    TEST_RESULTS+=("FAIL: $1")
    ((FAILED_TESTS++))
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    if ! command -v curl &> /dev/null; then
        log_error "curl is not installed. Please install curl."
        exit 1
    fi

    if ! command -v jq &> /dev/null; then
        log_warn "jq is not installed. JSON parsing will be limited."
    fi

    if [ ! -f "$SERVER_BIN" ]; then
        log_error "Server binary not found at $SERVER_BIN"
        log_error "Please build the project first: cmake --build build"
        exit 1
    fi
}

# Start the server
start_server() {
    log_info "Starting network_midi_server on port $SERVER_PORT..."

    # Kill any existing server on the port
    lsof -ti:$SERVER_PORT | xargs kill -9 2>/dev/null || true
    sleep 1

    # Start server in background
    $SERVER_BIN --port $SERVER_PORT > /tmp/routing_test_server.log 2>&1 &
    SERVER_PID=$!

    log_info "Server started with PID $SERVER_PID"

    # Wait for server to be ready
    log_info "Waiting for server to be ready..."
    for i in {1..30}; do
        if curl -s "$BASE_URL/health" > /dev/null 2>&1; then
            log_info "Server is ready"
            return 0
        fi
        sleep 1
    done

    log_error "Server failed to start within 30 seconds"
    cat /tmp/routing_test_server.log
    return 1
}

# Stop the server
stop_server() {
    if [ -n "$SERVER_PID" ]; then
        log_info "Stopping server (PID $SERVER_PID)..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
        SERVER_PID=""
    fi
}

# Cleanup on exit
cleanup() {
    stop_server
    log_info "Cleanup complete"
}

trap cleanup EXIT

# Test: Health check
test_health_check() {
    log_info "Test: Health check endpoint"

    response=$(curl -s -w "\n%{http_code}" "$BASE_URL/health")
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)

    if [ "$http_code" = "200" ]; then
        test_pass "Health check returned 200 OK"
    else
        test_fail "Health check returned $http_code (expected 200)"
    fi
}

# Test: Create a route
test_create_route() {
    log_info "Test: Create routing rule via POST /routing/routes"

    # Create a simple route
    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/routing/routes" \
        -H "Content-Type: application/json" \
        -d '{
            "sourceNode": "00000000-0000-0000-0000-000000000000",
            "sourceDeviceId": 1,
            "destNode": "12345678-1234-5678-1234-567812345678",
            "destDeviceId": 10,
            "priority": 100,
            "enabled": true
        }')

    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)

    if [ "$http_code" = "201" ] || [ "$http_code" = "200" ]; then
        # Extract rule ID from response
        if command -v jq &> /dev/null; then
            CREATED_RULE_ID=$(echo "$body" | jq -r '.ruleId')
            test_pass "Route created successfully (ID: $CREATED_RULE_ID)"
        else
            test_pass "Route created successfully (HTTP $http_code)"
        fi
    else
        test_fail "Failed to create route (HTTP $http_code): $body"
    fi
}

# Test: List routes
test_list_routes() {
    log_info "Test: List routing rules via GET /routing/routes"

    response=$(curl -s -w "\n%{http_code}" "$BASE_URL/routing/routes")
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)

    if [ "$http_code" = "200" ]; then
        if command -v jq &> /dev/null; then
            rule_count=$(echo "$body" | jq '.rules | length')
            test_pass "Listed $rule_count routing rules"
        else
            test_pass "Successfully retrieved routing rules"
        fi
    else
        test_fail "Failed to list routes (HTTP $http_code): $body"
    fi
}

# Test: Get specific route
test_get_route() {
    log_info "Test: Get specific route via GET /routing/routes/{id}"

    if [ -z "$CREATED_RULE_ID" ]; then
        log_warn "No rule ID available, skipping get route test"
        return
    fi

    response=$(curl -s -w "\n%{http_code}" "$BASE_URL/routing/routes/$CREATED_RULE_ID")
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)

    if [ "$http_code" = "200" ]; then
        test_pass "Retrieved specific route successfully"
    else
        test_fail "Failed to get route (HTTP $http_code): $body"
    fi
}

# Test: Update route
test_update_route() {
    log_info "Test: Update route via PUT /routing/routes/{id}"

    if [ -z "$CREATED_RULE_ID" ]; then
        log_warn "No rule ID available, skipping update test"
        return
    fi

    response=$(curl -s -w "\n%{http_code}" -X PUT "$BASE_URL/routing/routes/$CREATED_RULE_ID" \
        -H "Content-Type: application/json" \
        -d '{
            "sourceNode": "00000000-0000-0000-0000-000000000000",
            "sourceDeviceId": 1,
            "destNode": "87654321-4321-8765-4321-876543218765",
            "destDeviceId": 20,
            "priority": 200,
            "enabled": false
        }')

    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)

    if [ "$http_code" = "200" ]; then
        test_pass "Route updated successfully"
    else
        test_fail "Failed to update route (HTTP $http_code): $body"
    fi
}

# Test: Create route with filters
test_create_route_with_filters() {
    log_info "Test: Create route with channel and message type filters"

    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/routing/routes" \
        -H "Content-Type: application/json" \
        -d '{
            "sourceNode": "00000000-0000-0000-0000-000000000000",
            "sourceDeviceId": 2,
            "destNode": "12345678-1234-5678-1234-567812345678",
            "destDeviceId": 15,
            "priority": 150,
            "enabled": true,
            "channelFilter": [1, 2, 3],
            "messageTypeFilter": [144, 128]
        }')

    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)

    if [ "$http_code" = "201" ] || [ "$http_code" = "200" ]; then
        if command -v jq &> /dev/null; then
            FILTERED_RULE_ID=$(echo "$body" | jq -r '.ruleId')
            test_pass "Route with filters created successfully (ID: $FILTERED_RULE_ID)"
        else
            test_pass "Route with filters created successfully"
        fi
    else
        test_fail "Failed to create route with filters (HTTP $http_code): $body"
    fi
}

# Test: Get route statistics
test_route_statistics() {
    log_info "Test: Get route statistics via GET /routing/routes/{id}/stats"

    if [ -z "$CREATED_RULE_ID" ]; then
        log_warn "No rule ID available, skipping statistics test"
        return
    fi

    response=$(curl -s -w "\n%{http_code}" "$BASE_URL/routing/routes/$CREATED_RULE_ID/stats")
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)

    if [ "$http_code" = "200" ]; then
        if command -v jq &> /dev/null; then
            messages_routed=$(echo "$body" | jq -r '.messagesRouted')
            test_pass "Retrieved route statistics (messages: $messages_routed)"
        else
            test_pass "Retrieved route statistics"
        fi
    else
        test_fail "Failed to get statistics (HTTP $http_code): $body"
    fi
}

# Test: Enable/disable route
test_enable_disable_route() {
    log_info "Test: Enable/disable route via PATCH /routing/routes/{id}"

    if [ -z "$CREATED_RULE_ID" ]; then
        log_warn "No rule ID available, skipping enable/disable test"
        return
    fi

    # Disable
    response=$(curl -s -w "\n%{http_code}" -X PATCH "$BASE_URL/routing/routes/$CREATED_RULE_ID" \
        -H "Content-Type: application/json" \
        -d '{"enabled": false}')

    http_code=$(echo "$response" | tail -n1)

    if [ "$http_code" = "200" ]; then
        test_pass "Route disabled successfully"
    else
        test_fail "Failed to disable route (HTTP $http_code)"
    fi

    # Enable
    response=$(curl -s -w "\n%{http_code}" -X PATCH "$BASE_URL/routing/routes/$CREATED_RULE_ID" \
        -H "Content-Type: application/json" \
        -d '{"enabled": true}')

    http_code=$(echo "$response" | tail -n1)

    if [ "$http_code" = "200" ]; then
        test_pass "Route enabled successfully"
    else
        test_fail "Failed to enable route (HTTP $http_code)"
    fi
}

# Test: Delete route
test_delete_route() {
    log_info "Test: Delete route via DELETE /routing/routes/{id}"

    if [ -z "$CREATED_RULE_ID" ]; then
        log_warn "No rule ID available, skipping delete test"
        return
    fi

    response=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/routing/routes/$CREATED_RULE_ID")
    http_code=$(echo "$response" | tail -n1)

    if [ "$http_code" = "204" ] || [ "$http_code" = "200" ]; then
        test_pass "Route deleted successfully"
    else
        test_fail "Failed to delete route (HTTP $http_code)"
    fi

    # Verify deletion
    response=$(curl -s -w "\n%{http_code}" "$BASE_URL/routing/routes/$CREATED_RULE_ID")
    http_code=$(echo "$response" | tail -n1)

    if [ "$http_code" = "404" ]; then
        test_pass "Verified route was deleted (404 on GET)"
    else
        test_fail "Route still exists after deletion (HTTP $http_code)"
    fi
}

# Test: Invalid route creation
test_invalid_route_creation() {
    log_info "Test: Reject invalid route (non-existent device)"

    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/routing/routes" \
        -H "Content-Type: application/json" \
        -d '{
            "sourceNode": "00000000-0000-0000-0000-000000000000",
            "sourceDeviceId": 9999,
            "destNode": "12345678-1234-5678-1234-567812345678",
            "destDeviceId": 10,
            "priority": 100,
            "enabled": true
        }')

    http_code=$(echo "$response" | tail -n1)

    if [ "$http_code" = "400" ] || [ "$http_code" = "404" ]; then
        test_pass "Invalid route rejected with appropriate error code ($http_code)"
    else
        test_fail "Invalid route should be rejected (got HTTP $http_code)"
    fi
}

# Test: Malformed JSON
test_malformed_json() {
    log_info "Test: Reject malformed JSON"

    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/routing/routes" \
        -H "Content-Type: application/json" \
        -d '{"invalid json": }')

    http_code=$(echo "$response" | tail -n1)

    if [ "$http_code" = "400" ]; then
        test_pass "Malformed JSON rejected with 400 Bad Request"
    else
        test_fail "Malformed JSON should return 400 (got HTTP $http_code)"
    fi
}

# Print test summary
print_summary() {
    echo ""
    echo "========================================"
    echo "Test Summary"
    echo "========================================"
    echo -e "${GREEN}Passed:${NC} $PASSED_TESTS"
    echo -e "${RED}Failed:${NC} $FAILED_TESTS"
    echo "Total:  $((PASSED_TESTS + FAILED_TESTS))"
    echo ""

    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        exit 0
    else
        echo -e "${RED}Some tests failed!${NC}"
        echo ""
        echo "Failed tests:"
        for result in "${TEST_RESULTS[@]}"; do
            if [[ $result == FAIL:* ]]; then
                echo -e "  ${RED}✗${NC} ${result#FAIL: }"
            fi
        done
        exit 1
    fi
}

# Main test execution
main() {
    log_info "Starting Routing API Integration Tests"
    log_info "Server: $BASE_URL"
    echo ""

    check_prerequisites
    start_server

    echo ""
    log_info "Running tests..."
    echo ""

    # Run all tests
    test_health_check
    test_create_route
    test_list_routes
    test_get_route
    test_update_route
    test_create_route_with_filters
    test_route_statistics
    test_enable_disable_route
    test_delete_route
    test_invalid_route_creation
    test_malformed_json

    echo ""
    print_summary
}

# Run main
main "$@"
