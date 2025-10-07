# Phase 4 Architecture Diagrams

## 1. Context Flow Through Network

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Multi-Hop Message Flow                           │
└─────────────────────────────────────────────────────────────────────────┘

   Node A                    Node B                    Node C
┌──────────┐              ┌──────────┐              ┌──────────┐
│          │              │          │              │          │
│ Device 1 │──MIDI──►     │          │              │          │
│  (Input) │              │          │              │          │
│          │              │          │              │          │
└────┬─────┘              │          │              │          │
     │                    │          │              │          │
     │ Forward            │          │              │          │
     ▼                    │          │              │          │
┌──────────┐              │          │              │          │
│ Router   │              │          │              │          │
│          │              │          │              │          │
│ Context: │              │          │              │          │
│  visited={}            │          │              │          │
│  hops=0  │              │          │              │          │
│          │              │          │              │          │
│ Update:  │              │          │              │          │
│  visited={A:1}         │          │              │          │
│  hops=1  │              │          │              │          │
└────┬─────┘              │          │              │          │
     │                    │          │              │          │
     │ Embed Context      │          │              │          │
     ▼                    │          │              │          │
┌──────────┐              │          │              │          │
│ Packet   │              │          │              │          │
│ Builder  │              │          │              │          │
│          │              │          │              │          │
│ [Header] │              │          │              │          │
│ [MIDI]   │              │          │              │          │
│ [Context]│              │          │              │          │
│  hops=1  │              │          │              │          │
│  devices │              │          │              │          │
│   A:1    │              │          │              │          │
└────┬─────┘              │          │              │          │
     │                    │          │              │          │
     │ UDP Transmit       │          │              │          │
     └────────────────────┼─────────►│              │          │
                          │          │              │          │
                          │ Receive  │              │          │
                          ▼          │              │          │
                     ┌──────────┐    │              │          │
                     │ Packet   │    │              │          │
                     │ Parser   │    │              │          │
                     │          │    │              │          │
                     │ Extract: │    │              │          │
                     │  context │    │              │          │
                     └────┬─────┘    │              │          │
                          │          │              │          │
                          ▼          │              │          │
                     ┌──────────┐    │              │          │
                     │ Router   │    │              │          │
                     │          │    │              │          │
                     │ Context: │    │              │          │
                     │  visited={A:1}│              │          │
                     │  hops=1  │    │              │          │
                     │          │    │              │          │
                     │ Update:  │    │              │          │
                     │  visited={A:1, B:2}          │          │
                     │  hops=2  │    │              │          │
                     └────┬─────┘    │              │          │
                          │          │              │          │
                          │ Embed    │              │          │
                          ▼          │              │          │
                     ┌──────────┐    │              │          │
                     │ Packet   │    │              │          │
                     │          │    │              │          │
                     │ [Header] │    │              │          │
                     │ [MIDI]   │    │              │          │
                     │ [Context]│    │              │          │
                     │  hops=2  │    │              │          │
                     │  devices │    │              │          │
                     │   A:1    │    │              │          │
                     │   B:2    │    │              │          │
                     └────┬─────┘    │              │          │
                          │          │              │          │
                          │ UDP      │              │          │
                          └──────────┼──────────────┼─────────►│
                                     │              │          │
                                     │              │ Receive  │
                                     │              ▼          │
                                     │         ┌──────────┐    │
                                     │         │ Router   │    │
                                     │         │          │    │
                                     │         │ Context: │    │
                                     │         │  visited={A:1, B:2}
                                     │         │  hops=2  │    │
                                     │         │          │    │
                                     │         │ Check:   │    │
                                     │         │  C:3 not │    │
                                     │         │  visited │    │
                                     │         │  ✓ OK    │    │
                                     │         └────┬─────┘    │
                                     │              │          │
                                     │              ▼          │
                                     │         ┌──────────┐    │
                                     │         │ Device 3 │    │
                                     │         │ (Output) │    │
                                     │         │  MIDI►   │    │
                                     │         └──────────┘    │
                                     │                         │
                                     └─────────────────────────┘
```

---

## 2. Loop Detection Example

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Circular Route: A → B → A                           │
└─────────────────────────────────────────────────────────────────────────┘

Step 1: Message originates at Node A
┌──────────┐
│ Node A   │
│          │
│ Context: │
│  visited={A:1}
│  hops=1  │
└────┬─────┘
     │ Forward to B
     ▼

Step 2: Node B receives and forwards back to A
┌──────────┐
│ Node B   │
│          │
│ Context: │
│  visited={A:1, B:2}
│  hops=2  │
└────┬─────┘
     │ Forward to A
     ▼

Step 3: Node A receives - LOOP DETECTED!
┌──────────┐
│ Node A   │
│          │
│ Context: │
│  visited={A:1, B:2}
│  hops=2  │
│          │
│ Check:   │
│  A:1 in visitedDevices?
│  YES! Loop detected
│          │
│ Action:  │
│  Drop message
│  stats.loopsDetected++
└──────────┘
```

---

## 3. Packet Format Layout

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      MidiPacket with Context                            │
└─────────────────────────────────────────────────────────────────────────┘

Offset | Size | Field                    | Example Value
-------|------|--------------------------|------------------
  0    |  2   | Magic                    | 0x4D49 ("MI")
  2    |  1   | Version                  | 0x01
  3    |  1   | Flags                    | 0x08 (HasContext)
  4    |  4   | Source Node Hash         | 0x1A2B3C4D
  8    |  4   | Dest Node Hash           | 0x5E6F7A8B
 12    |  2   | Sequence                 | 0x0042
 14    |  4   | Timestamp                | 0x00123456
 18    |  2   | Device ID                | 0x0005
 ──────────────────────────────────────────────────────────────────
 20    |  N   | MIDI Data                | [0x90, 0x3C, 0x64]
 ──────────────────────────────────────────────────────────────────
20+N   |  1   | Extension Type           | 0x01 (Context)
21+N   |  1   | Extension Length         | 0x16 (22 bytes)
22+N   |  1   | Hop Count                | 0x02
23+N   |  1   | Device Count             | 0x02
24+N   |  6   | Visited Device 1         |
       |  4   |   Node Hash              | 0x1A2B3C4D
       |  2   |   Device ID              | 0x0001
30+N   |  6   | Visited Device 2         |
       |  4   |   Node Hash              | 0x5E6F7A8B
       |  2   |   Device ID              | 0x0002
 ──────────────────────────────────────────────────────────────────

Total Size: 20 + N + (4 + M*6) bytes
Example: 20 + 3 + (4 + 2*6) = 39 bytes
```

---

## 4. Component Integration

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Phase 4 Component Architecture                       │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                           MeshManager                                   │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐     │
│  │ Connection Pool  │  │  UuidRegistry    │  │  Node Discovery  │     │
│  │                  │  │                  │  │                  │     │
│  │ Node A: ────────►│  │ register(uuid)   │  │  mDNS Scanner    │     │
│  │ Node B: ────────►│  │ lookup(hash)     │  │                  │     │
│  │ Node C: ────────►│  │ unregister(uuid) │  │                  │     │
│  └──────────────────┘  └─────────┬────────┘  └──────────────────┘     │
│                                  │                                     │
└──────────────────────────────────┼─────────────────────────────────────┘
                                   │
                                   │ inject
                                   ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                            MidiRouter                                   │
│  ┌──────────────────────────────────────────────────────────────┐     │
│  │                     forwardMessage()                         │     │
│  │                                                              │     │
│  │  1. Extract context from incoming packet (if present)       │     │
│  │     ├─► packet.getForwardingContext(*uuidRegistry)          │     │
│  │     └─► deserialize visited devices                         │     │
│  │                                                              │     │
│  │  2. Check loop prevention                                   │     │
│  │     ├─► shouldForward(sourceDevice)?                        │     │
│  │     └─► if loop: drop, stats.loopsDetected++               │     │
│  │                                                              │     │
│  │  3. Update context                                          │     │
│  │     ├─► recordVisit(sourceDevice)                           │     │
│  │     └─► hopCount++                                          │     │
│  │                                                              │     │
│  │  4. Embed context in outgoing packet                        │     │
│  │     ├─► packet.setForwardingContext(context)                │     │
│  │     └─► serialize visited devices                           │     │
│  └──────────────────────────────────────────────────────────────┘     │
│                                                                         │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
                             │ sendPacket()
                             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      NetworkConnection                                  │
│  ┌──────────────────────────────────────────────────────────────┐     │
│  │                    ConnectionWorker                          │     │
│  │                                                              │     │
│  │  SEDA Command: SendPacketCommand                            │     │
│  │  ├─► classify message (realtime/non-realtime)               │     │
│  │  │                                                           │     │
│  │  ├─► if realtime:                                           │     │
│  │  │   └─► UdpMidiTransport.sendPacket()                      │     │
│  │  │                                                           │     │
│  │  └─► if non-realtime:                                       │     │
│  │      └─► TcpMidiTransport.sendPacket()                      │     │
│  └──────────────────────────────────────────────────────────────┘     │
│                                                                         │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
                             │ UDP/TCP
                             ▼
                    ┌────────────────┐
                    │   Network      │
                    └────────────────┘
```

---

## 5. Backward Compatibility Matrix

```
┌─────────────────────────────────────────────────────────────────────────┐
│              Phase 3 ↔ Phase 4 Compatibility Matrix                    │
└─────────────────────────────────────────────────────────────────────────┘

Sender    Receiver   Context      Result
────────  ─────────  ───────────  ──────────────────────────────────────
Phase 3 → Phase 3    None         ✓ Works (original behavior)
Phase 3 → Phase 4    None         ✓ Works (P4 creates fresh context)
Phase 4 → Phase 3    Embedded     ✓ Works (P3 ignores context)
Phase 4 → Phase 4    Embedded     ✓ Works (context preserved)

┌─────────────────────────────────────────────────────────────────────────┐
│                      Mixed Mesh Example                                 │
└─────────────────────────────────────────────────────────────────────────┘

   P4 Node A        P3 Node B        P4 Node C
   ─────────        ─────────        ─────────
   Context:         Context:         Context:
   {A:1}            RESET            {C:3}
   hops=1    ────►  (ignored)  ────► (fresh)

   Result: Loop prevention only works within Phase 4 nodes
   Migration: Upgrade all nodes to Phase 4 for full coverage
```

---

## 6. Performance Visualization

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     Per-Hop Latency Breakdown                           │
└─────────────────────────────────────────────────────────────────────────┘

Operation                         Time
─────────────────────────────────────────────────────────────────
Extract context from packet       ████ 200ns
Update visited devices            ██ 100ns
Increment hop count               ▌ 10ns
Serialize context                 ██████ 300ns
Embed in outgoing packet          ███ 150ns
─────────────────────────────────────────────────────────────────
Total per hop                     ███████████████ 760ns

Multi-hop:
1 hop:  ███████████████ 760ns
2 hops: ██████████████████████████████ 1.5μs
3 hops: █████████████████████████████████████████████ 2.3μs
8 hops: ████████████████████████████████████████████████████████████ 6.1μs

Target: <10μs ✓ Met

┌─────────────────────────────────────────────────────────────────────────┐
│                       Packet Size Growth                                │
└─────────────────────────────────────────────────────────────────────────┘

Hops | Packet Size (3-byte MIDI)
─────┼────────────────────────────────────────────────────
  0  | ███████████████████████ 23 bytes
  1  | █████████████████████████████████ 33 bytes
  2  | ███████████████████████████████████████ 39 bytes
  3  | █████████████████████████████████████████████ 45 bytes
  8  | ███████████████████████████████████████████████████████████████████████ 75 bytes

Target: <100 bytes ✓ Met
```

---

## 7. State Transition Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                Context State Through Forwarding Chain                   │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────┐
│ Initial     │
│ Context:    │
│  visited={} │
│  hops=0     │
└──────┬──────┘
       │
       │ Record visit: A:1
       │ Increment hops
       ▼
┌─────────────┐
│ After A     │
│ Context:    │
│  visited=   │
│   {A:1}     │
│  hops=1     │
└──────┬──────┘
       │
       │ Serialize → Packet
       │ Send to B
       │ Receive at B
       │ Deserialize → Context
       │ Record visit: B:2
       │ Increment hops
       ▼
┌─────────────┐
│ After B     │
│ Context:    │
│  visited=   │
│   {A:1,     │
│    B:2}     │
│  hops=2     │
└──────┬──────┘
       │
       │ Serialize → Packet
       │ Send to C
       │ Receive at C
       │ Deserialize → Context
       │ Record visit: C:3
       │ Increment hops
       ▼
┌─────────────┐
│ After C     │
│ Context:    │
│  visited=   │
│   {A:1,     │
│    B:2,     │
│    C:3}     │
│  hops=3     │
└──────┬──────┘
       │
       │ shouldForward(D:4)?
       │ ✓ D:4 not in visited
       │ ✓ hops < MAX_HOPS (8)
       │
       │ shouldForward(A:1)?
       │ ✗ A:1 in visited
       │ → Loop detected!
       ▼
┌─────────────┐
│ Drop        │
│ Message     │
│             │
│ stats.      │
│  loops      │
│  Detected++ │
└─────────────┘
```

---

## 8. Implementation Dependencies

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     Task Dependency Graph                               │
└─────────────────────────────────────────────────────────────────────────┘

                    ┌────────────┐
                    │   Task 4.1 │
                    │ MidiPacket │
                    │  Context   │
                    └──────┬─────┘
                           │
                           │
          ┌────────────────┴────────────────┐
          │                                 │
          ▼                                 ▼
    ┌────────────┐                    ┌────────────┐
    │   Task 4.2 │                    │   Task 4.4 │
    │ UuidRegistry│                    │  Network   │
    │            │                    │ Transport  │
    └──────┬─────┘                    └──────┬─────┘
          │                                 │
          │                                 │
          └────────────┬────────────────────┘
                       │
                       ▼
                 ┌────────────┐
                 │   Task 4.3 │
                 │ MidiRouter │
                 │  Context   │
                 │ Handling   │
                 └──────┬─────┘
                        │
                        │
         ┌──────────────┴──────────────┐
         │                             │
         ▼                             ▼
   ┌────────────┐              ┌────────────┐
   │   Task 4.5 │              │   Task 4.6 │
   │MeshManager │              │    E2E     │
   │Integration │              │  Testing   │
   └────────────┘              └────────────┘

Critical Path: 4.1 → 4.3 → 4.6 (10 days)
Parallel Work: 4.2, 4.4, 4.5 can start earlier
Total Duration: 16 days (3 weeks)
```

---

**For detailed implementation specifications, see:**
- [Full Phase 4 Design Document](./phase4_multihop_context.md)
- [Executive Summary](./phase4_executive_summary.md)
