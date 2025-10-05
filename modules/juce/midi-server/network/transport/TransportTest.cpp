#include "UdpMidiTransport.h"
#include "ReliableTransport.h"
#include "MessageBuffer.h"
#include <iostream>
#include <cassert>

using namespace NetworkMidi;

/**
 * Test suite for Phase 4 Network MIDI Transport.
 *
 * Tests:
 * - MidiPacket serialization/deserialization
 * - UDP transport send/receive
 * - Reliable delivery with ACK/retry
 * - Message buffering and reordering
 */

// Helper function to create test MIDI data
std::vector<uint8_t> createNoteOn(uint8_t note, uint8_t velocity) {
    return {0x90, note, velocity};
}

std::vector<uint8_t> createSysEx() {
    std::vector<uint8_t> sysex = {0xF0, 0x43, 0x12, 0x00};
    for (int i = 0; i < 100; i++) {
        sysex.push_back(i & 0x7F);
    }
    sysex.push_back(0xF7);
    return sysex;
}

// Test 1: MidiPacket Serialization/Deserialization
void testPacketSerialization() {
    std::cout << "Test 1: MidiPacket Serialization/Deserialization..." << std::endl;

    juce::Uuid sourceNode;
    juce::Uuid destNode;
    std::vector<uint8_t> midiData = createNoteOn(60, 100);

    // Create packet
    MidiPacket originalPacket = MidiPacket::createDataPacket(
        sourceNode, destNode, 5, midiData, 1234
    );

    // Serialize
    auto serialized = originalPacket.serialize();
    assert(serialized.size() == MidiPacket::HEADER_SIZE + midiData.size());

    // Deserialize
    MidiPacket deserializedPacket = MidiPacket::deserialize(
        serialized.data(), serialized.size()
    );

    // Verify
    assert(deserializedPacket.isValid());
    assert(deserializedPacket.getSequence() == 1234);
    assert(deserializedPacket.getDeviceId() == 5);
    assert(deserializedPacket.getMidiData() == midiData);

    std::cout << "  ✓ Basic packet serialization works" << std::endl;

    // Test SysEx packet
    std::vector<uint8_t> sysexData = createSysEx();
    MidiPacket sysexPacket = MidiPacket::createDataPacket(
        sourceNode, destNode, 10, sysexData, 5678
    );

    assert(sysexPacket.isSysEx());
    assert(sysexPacket.isReliable());

    auto sysexSerialized = sysexPacket.serialize();
    MidiPacket sysexDeserialized = MidiPacket::deserialize(
        sysexSerialized.data(), sysexSerialized.size()
    );

    assert(sysexDeserialized.getMidiData() == sysexData);

    std::cout << "  ✓ SysEx packet serialization works" << std::endl;
    std::cout << "Test 1: PASSED\n" << std::endl;
}

// Test 2: UDP Transport Basic Send/Receive
void testUdpTransport() {
    std::cout << "Test 2: UDP Transport Basic Send/Receive..." << std::endl;

    // Create two transports (simulating two nodes)
    UdpMidiTransport transport1(0);  // Auto-assign port
    UdpMidiTransport transport2(0);  // Auto-assign port

    juce::Uuid node1 = juce::Uuid();
    juce::Uuid node2 = juce::Uuid();

    transport1.setNodeId(node1);
    transport2.setNodeId(node2);

    // Start both transports
    assert(transport1.start());
    assert(transport2.start());

    std::cout << "  Transport 1 port: " << transport1.getPort() << std::endl;
    std::cout << "  Transport 2 port: " << transport2.getPort() << std::endl;

    // Set up receive callback
    bool receivedPacket = false;
    MidiPacket receivedData;

    transport2.onPacketReceived = [&](const MidiPacket& packet,
                                       const juce::String& addr,
                                       int port) {
        receivedPacket = true;
        receivedData = packet;
    };

    // Send a message from transport1 to transport2
    std::vector<uint8_t> midiData = createNoteOn(64, 80);
    transport1.sendMessage(
        node2,
        "127.0.0.1",
        transport2.getPort(),
        1,
        midiData
    );

    // Wait for receive
    juce::Thread::sleep(100);

    assert(receivedPacket);
    assert(receivedData.getDeviceId() == 1);
    assert(receivedData.getMidiData() == midiData);

    std::cout << "  ✓ UDP send/receive works" << std::endl;

    // Check statistics
    auto stats1 = transport1.getStatistics();
    auto stats2 = transport2.getStatistics();

    assert(stats1.packetsSent > 0);
    assert(stats2.packetsReceived > 0);

    std::cout << "  ✓ Statistics tracking works" << std::endl;
    std::cout << "Test 2: PASSED\n" << std::endl;

    transport1.stop();
    transport2.stop();
}

// Test 3: Reliable Transport ACK/Retry
void testReliableTransport() {
    std::cout << "Test 3: Reliable Transport ACK/Retry..." << std::endl;

    UdpMidiTransport transport(0);
    transport.setNodeId(juce::Uuid());
    assert(transport.start());

    ReliableTransport::Config config;
    config.timeoutMs = 50;
    config.maxRetries = 2;

    ReliableTransport reliableTransport(transport, config);

    // Test successful delivery
    bool successCalled = false;
    bool failureCalled = false;

    juce::Uuid destNode;
    std::vector<uint8_t> sysexData = createSysEx();

    MidiPacket packet = MidiPacket::createDataPacket(
        transport.getNodeId(),
        destNode,
        5,
        sysexData,
        1000
    );

    reliableTransport.sendReliable(
        packet,
        "127.0.0.1",
        9999,
        [&]() { successCalled = true; },
        [&](const juce::String& reason) {
            failureCalled = true;
            std::cout << "  Failure reason: " << reason << std::endl;
        }
    );

    // Simulate ACK
    juce::Thread::sleep(10);
    reliableTransport.handleAck(1000, destNode);

    juce::Thread::sleep(10);

    assert(successCalled);
    assert(!failureCalled);

    std::cout << "  ✓ ACK handling works" << std::endl;

    // Test timeout and retry
    successCalled = false;
    failureCalled = false;

    MidiPacket packet2 = MidiPacket::createDataPacket(
        transport.getNodeId(),
        destNode,
        5,
        sysexData,
        2000
    );

    reliableTransport.sendReliable(
        packet2,
        "127.0.0.1",
        9998,
        [&]() { successCalled = true; },
        [&](const juce::String& reason) { failureCalled = true; }
    );

    // Wait for timeout and retries
    juce::Thread::sleep(300);

    assert(!successCalled);
    assert(failureCalled);

    auto stats = reliableTransport.getStatistics();
    assert(stats.retries > 0);
    assert(stats.reliableFailed > 0);

    std::cout << "  ✓ Timeout and retry works" << std::endl;
    std::cout << "Test 3: PASSED\n" << std::endl;

    transport.stop();
}

// Test 4: Message Buffer Reordering
void testMessageBuffer() {
    std::cout << "Test 4: Message Buffer Reordering..." << std::endl;

    MessageBuffer buffer;

    std::vector<MidiPacket> deliveredPackets;

    buffer.onPacketReady = [&](const MidiPacket& packet) {
        deliveredPackets.push_back(packet);
    };

    juce::Uuid nodeId;

    // Create packets out of order
    std::vector<uint8_t> data1 = createNoteOn(60, 100);
    std::vector<uint8_t> data2 = createNoteOn(62, 100);
    std::vector<uint8_t> data3 = createNoteOn(64, 100);
    std::vector<uint8_t> data4 = createNoteOn(65, 100);

    MidiPacket packet1 = MidiPacket::createDataPacket(nodeId, nodeId, 1, data1, 1);
    MidiPacket packet2 = MidiPacket::createDataPacket(nodeId, nodeId, 1, data2, 2);
    MidiPacket packet3 = MidiPacket::createDataPacket(nodeId, nodeId, 1, data3, 3);
    MidiPacket packet4 = MidiPacket::createDataPacket(nodeId, nodeId, 1, data4, 4);

    // Add packets out of order: 1, 3, 2, 4
    buffer.addPacket(packet1);
    assert(deliveredPackets.size() == 1);  // Packet 1 delivered immediately

    buffer.addPacket(packet3);
    assert(deliveredPackets.size() == 1);  // Packet 3 buffered (waiting for 2)

    buffer.addPacket(packet2);
    assert(deliveredPackets.size() == 3);  // Packets 2 and 3 delivered

    buffer.addPacket(packet4);
    assert(deliveredPackets.size() == 4);  // Packet 4 delivered

    // Verify order
    assert(deliveredPackets[0].getSequence() == 1);
    assert(deliveredPackets[1].getSequence() == 2);
    assert(deliveredPackets[2].getSequence() == 3);
    assert(deliveredPackets[3].getSequence() == 4);

    std::cout << "  ✓ Packet reordering works" << std::endl;

    // Check statistics
    auto stats = buffer.getStatistics();
    assert(stats.packetsReceived == 4);
    assert(stats.packetsDelivered == 4);
    assert(stats.packetsReordered > 0);

    std::cout << "  ✓ Statistics tracking works" << std::endl;
    std::cout << "Test 4: PASSED\n" << std::endl;
}

// Test 5: Sequence Number Wraparound
void testSequenceWraparound() {
    std::cout << "Test 5: Sequence Number Wraparound..." << std::endl;

    MessageBuffer buffer;
    buffer.setNextExpectedSequence(65534);

    std::vector<uint16_t> deliveredSequences;

    buffer.onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    juce::Uuid nodeId;
    std::vector<uint8_t> data = createNoteOn(60, 100);

    // Create packets around wraparound point
    MidiPacket packet1 = MidiPacket::createDataPacket(nodeId, nodeId, 1, data, 65534);
    MidiPacket packet2 = MidiPacket::createDataPacket(nodeId, nodeId, 1, data, 65535);
    MidiPacket packet3 = MidiPacket::createDataPacket(nodeId, nodeId, 1, data, 0);
    MidiPacket packet4 = MidiPacket::createDataPacket(nodeId, nodeId, 1, data, 1);

    buffer.addPacket(packet1);
    buffer.addPacket(packet2);
    buffer.addPacket(packet3);
    buffer.addPacket(packet4);

    assert(deliveredSequences.size() == 4);
    assert(deliveredSequences[0] == 65534);
    assert(deliveredSequences[1] == 65535);
    assert(deliveredSequences[2] == 0);
    assert(deliveredSequences[3] == 1);

    std::cout << "  ✓ Sequence wraparound handling works" << std::endl;
    std::cout << "Test 5: PASSED\n" << std::endl;
}

int main() {
    std::cout << "\n=== Phase 4 Network MIDI Transport Test Suite ===\n" << std::endl;

    try {
        testPacketSerialization();
        testUdpTransport();
        testReliableTransport();
        testMessageBuffer();
        testSequenceWraparound();

        std::cout << "\n=== ALL TESTS PASSED ===\n" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n!!! TEST FAILED !!!" << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
