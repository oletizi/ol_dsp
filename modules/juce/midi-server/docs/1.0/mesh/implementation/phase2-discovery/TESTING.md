# Testing the Network MIDI Service Discovery System

This document provides comprehensive instructions for testing the service discovery implementation.

## Prerequisites

- macOS system (for full mDNS/Bonjour support)
- CMake and build tools installed
- Network access (for multi-machine testing)

## Building the Test Program

```bash
cd /Users/orion/work/ol_dsp-midi-server
cmake -B build -S .
cmake --build build --target discovery_test
```

The binary will be located at:
```
build/modules/juce/midi-server/discovery_test_artefacts/discovery_test
```

## Basic Testing

### Test 1: Single Node Discovery (Both Methods)

Run a single instance with both mDNS and fallback discovery:

```bash
./build/modules/juce/midi-server/discovery_test_artefacts/discovery_test \
    --mode both \
    --name test-node-1 \
    --http-port 8080 \
    --udp-port 9090 \
    --devices 3
```

Expected output:
```
=== Network MIDI Discovery Test ===
Mode:         both
Node ID:      xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
Node Name:    test-node-1
HTTP Port:    8080
UDP Port:     9090
Device Count: 3
===================================

Starting mDNS discovery...
✓ mDNS advertising started
✓ mDNS browsing started

Starting fallback discovery...
✓ Fallback broadcasting started
✓ Fallback listening started

Discovery active. Press Ctrl+C to stop...
```

### Test 2: Multi-Instance Discovery on Same Machine

**Terminal 1:**
```bash
./build/modules/juce/midi-server/discovery_test_artefacts/discovery_test \
    --mode both --name node1 --http-port 8080 --udp-port 9090
```

**Terminal 2 (wait 2 seconds, then run):**
```bash
./build/modules/juce/midi-server/discovery_test_artefacts/discovery_test \
    --mode both --name node2 --http-port 8081 --udp-port 9091
```

**Terminal 3 (wait 2 seconds, then run):**
```bash
./build/modules/juce/midi-server/discovery_test_artefacts/discovery_test \
    --mode both --name node3 --http-port 8082 --udp-port 9092
```

Expected behavior:
- Terminal 1 should show "Discovered: node2" within 2-5 seconds of starting node2
- Terminal 1 should show "Discovered: node3" within 2-5 seconds of starting node3
- Terminal 2 should discover both node1 and node3
- Terminal 3 should discover both node1 and node2

Example output in Terminal 1:
```
[DISCOVERED] Node: node2
  UUID:      yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy
  IP:        127.0.0.1
  HTTP Port: 8081
  UDP Port:  9091
  Devices:   3
  Version:   1.0

[DISCOVERED] Node: node3
  UUID:      zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz
  IP:        127.0.0.1
  HTTP Port: 8082
  UDP Port:  9092
  Devices:   3
  Version:   1.0
```

### Test 3: mDNS Only

Test mDNS/Bonjour discovery in isolation:

**Terminal 1:**
```bash
./build/modules/juce/midi-server/discovery_test_artefacts/discovery_test \
    --mode mdns --name mdns-node-1
```

**Terminal 2:**
```bash
./build/modules/juce/midi-server/discovery_test_artefacts/discovery_test \
    --mode mdns --name mdns-node-2
```

Expected:
- Faster discovery (100-500ms typically)
- Immediate removal detection when a node stops

### Test 4: Fallback (UDP Multicast) Only

Test UDP multicast fallback in isolation:

**Terminal 1:**
```bash
./build/modules/juce/midi-server/discovery_test_artefacts/discovery_test \
    --mode fallback --name fallback-node-1
```

**Terminal 2:**
```bash
./build/modules/juce/midi-server/discovery_test_artefacts/discovery_test \
    --mode fallback --name fallback-node-2
```

Expected:
- Discovery within 0-5 seconds (depends on broadcast timing)
- Removal detection after 15 seconds of timeout

### Test 5: Node Removal Detection

1. Start 3 nodes (as in Test 2)
2. Wait until all nodes have discovered each other
3. Stop node2 (Ctrl+C in Terminal 2)
4. Observe removal notifications in Terminals 1 and 3

Expected output in Terminal 1:
```
[REMOVED] Node UUID: yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy
```

**mDNS timing**: Immediate (< 1 second)
**Fallback timing**: 15 seconds (after timeout)

### Test 6: Device Count Updates

The test program automatically updates device count every 30 seconds:

1. Start a node
2. Wait 30 seconds
3. Observe the update message:
```
[UPDATE] Changing device count to 4
```

Note:
- For mDNS, this requires re-registration (brief service interruption)
- For fallback, next broadcast will include new count

### Test 7: Cross-Network Discovery

Test discovery across different machines on the same LAN:

**Machine A:**
```bash
./discovery_test --mode both --name machine-a --http-port 8080
```

**Machine B:**
```bash
./discovery_test --mode both --name machine-b --http-port 8080
```

Expected:
- Both machines discover each other
- IP addresses are correctly resolved (not 127.0.0.1)

## Advanced Testing

### Test 8: Stress Test (Many Nodes)

Test scalability with many nodes:

```bash
# Run this script to start 10 nodes
for i in {1..10}; do
    port_http=$((8080 + i))
    port_udp=$((9090 + i))
    ./discovery_test --mode both --name "stress-node-$i" \
        --http-port $port_http --udp-port $port_udp &
done
```

Expected:
- All 10 nodes discover each other
- Total of 9 discoveries per node
- Reasonable CPU/memory usage

Clean up:
```bash
killall discovery_test
```

### Test 9: Network Partition Simulation

Test recovery from network disruptions:

1. Start 2 nodes on different machines
2. Verify they discover each other
3. Disconnect network cable or disable WiFi on one machine
4. Wait 15 seconds (fallback timeout)
5. Reconnect network
6. Verify nodes re-discover each other

### Test 10: Firewall Testing

Test with firewall enabled:

**macOS:**
```bash
# Enable firewall
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate on

# Run test
./discovery_test --mode both --name firewall-test

# Check if mDNS works (port 5353 should be allowed by default)
# Check if fallback works (UDP multicast should work)
```

If fallback fails, allow the binary through firewall:
```bash
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add discovery_test
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblockapp discovery_test
```

## Debugging

### Verify mDNS Service Registration

Use macOS built-in DNS-SD tools:

**Browse for services:**
```bash
dns-sd -B _midi-network._tcp
```

Expected output:
```
Browsing for _midi-network._tcp
Timestamp     A/R Flags if Domain    Service Type         Instance Name
12:34:56.789  Add     2  4 local.    _midi-network._tcp.  test-node-1
```

**Resolve a specific service:**
```bash
dns-sd -L "test-node-1" _midi-network._tcp
```

Expected output:
```
Lookup test-node-1._midi-network._tcp.local
12:34:56.789  test-node-1._midi-network._tcp.local. can be reached at
  MacBook-Pro.local.:8080 (interface 4)
  uuid=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
  http_port=8080
  udp_port=9090
  hostname=MacBook-Pro.local
  version=1.0
  devices=3
```

### Verify UDP Multicast

**Check multicast group membership:**
```bash
netstat -g
```

Should show:
```
Group                     Link-layer Address  Netif
239.255.42.99             01:00:5e:7f:2a:63   en0
```

**Capture multicast packets:**
```bash
sudo tcpdump -i any -n 'udp port 5353'
```

Expected output (every 5 seconds):
```
12:34:56.789 IP 192.168.1.10.54321 > 239.255.42.99.5353: UDP, length 156
```

**View packet contents:**
```bash
sudo tcpdump -i any -n -X 'udp port 5353'
```

Should show JSON payloads in hex dump.

### Check for Port Conflicts

Verify ports are available before starting:

```bash
# Check if port 8080 is in use
lsof -i :8080

# Check if port 9090 is in use
lsof -i :9090
```

If ports are in use, choose different ports with `--http-port` and `--udp-port`.

### Monitor Resource Usage

**CPU and Memory:**
```bash
# While test is running
top -pid $(pgrep discovery_test)
```

Expected:
- CPU: < 1% when idle, brief spikes during discovery
- Memory: < 10MB per instance

**Network Traffic:**
```bash
# Monitor network activity
nettop -p discovery_test
```

Expected:
- Fallback: ~100 bytes out every 5 seconds
- mDNS: Minimal (multicast queries only)

## Troubleshooting

### Issue: No mDNS Discovery

**Symptoms:**
- `✗ mDNS advertising failed`
- `✗ mDNS browsing failed`

**Possible causes:**
1. mDNS daemon not running (unlikely on macOS)
2. Firewall blocking port 5353
3. Platform stub implementation (Linux/Windows)

**Solutions:**
```bash
# Check mDNS daemon (macOS)
sudo launchctl list | grep mdns

# Use fallback mode
./discovery_test --mode fallback
```

### Issue: No Fallback Discovery

**Symptoms:**
- `✗ Fallback broadcasting failed`
- `✗ Fallback listening failed`

**Possible causes:**
1. Port 5353 already in use
2. Multicast routing disabled
3. Firewall blocking UDP multicast

**Solutions:**
```bash
# Check port availability
lsof -i :5353

# Kill conflicting process
sudo kill -9 <PID>

# Check multicast routing
netstat -g | grep 239.255.42.99
```

### Issue: Nodes Don't Discover Each Other

**Symptoms:**
- No discovery messages after 30 seconds
- Self-discovery being skipped (expected)

**Diagnostics:**
```bash
# Terminal 1: Start node with verbose output
./discovery_test --mode both --name node1

# Terminal 2: Browse manually
dns-sd -B _midi-network._tcp

# Terminal 3: Capture packets
sudo tcpdump -i any 'udp port 5353' -vv
```

**Check:**
- Are services registered? (dns-sd output)
- Are packets being sent? (tcpdump output)
- Is self-discovery being filtered? (UUID match)

### Issue: High CPU Usage

**Symptoms:**
- discovery_test using > 5% CPU when idle

**Possible causes:**
1. Busy loop in event processing
2. Too-short select() timeout
3. Excessive logging

**Solutions:**
- Check select() timeout in mdns_macos.cpp (should be 1 second)
- Verify broadcast interval (should be 5 seconds, not 5ms)
- Profile with Instruments or sample

### Issue: Memory Leak

**Symptoms:**
- Memory usage growing over time

**Diagnostics:**
```bash
# Run with leak detection
leaks --atExit -- ./discovery_test --mode both --name leak-test

# Or use Valgrind (if installed)
valgrind --leak-check=full ./discovery_test
```

**Common causes:**
- DNSServiceRef not deallocated
- Threads not joined on shutdown
- Discovered nodes not cleaned up

## Performance Benchmarks

### Expected Performance Metrics

**Discovery Latency:**
- mDNS: 100-500ms (local network)
- Fallback: 0-5 seconds (depends on broadcast timing)

**Removal Detection:**
- mDNS: < 1 second (immediate callback)
- Fallback: 15 seconds (timeout-based)

**Resource Usage:**
- CPU (idle): < 1%
- CPU (discovery): < 5% (brief spike)
- Memory: < 10MB per instance
- Network (fallback): ~20 bytes/sec per node

**Scalability:**
- Tested up to 10 nodes on single machine
- Should handle 50+ nodes on LAN
- Fallback broadcasts scale linearly: O(n)

### Benchmark Commands

**Test discovery latency:**
```bash
time ./discovery_test --mode mdns --name latency-test &
# Note the timestamp of first discovery
```

**Test removal detection:**
```bash
# Start 2 nodes, note timestamps
./discovery_test --mode mdns --name node1 &
./discovery_test --mode mdns --name node2 &
# Kill node2, note removal timestamp in node1
```

**Test network overhead:**
```bash
# Capture 60 seconds of traffic
sudo tcpdump -i any 'udp port 5353' -w discovery.pcap &
sleep 60
killall tcpdump

# Analyze
tcpdump -r discovery.pcap | wc -l  # packet count
ls -lh discovery.pcap               # total bytes
```

## Next Steps

After validating the discovery system:

1. **Integrate with NetworkMidiServer** - Use discovery in main server
2. **Implement mesh formation** - Connect to discovered nodes
3. **Add authentication** - Verify node identity before connecting
4. **Optimize performance** - Reduce latency, improve scalability
5. **Complete platform support** - Implement Linux (Avahi) and Windows (Bonjour)

## References

- [Bonjour Overview](https://developer.apple.com/bonjour/)
- [DNS-SD API](https://developer.apple.com/documentation/dnssd)
- [Multicast UDP](https://www.tldp.org/HOWTO/Multicast-HOWTO.html)
- [Avahi Documentation](https://www.avahi.org/doxygen/html/)
