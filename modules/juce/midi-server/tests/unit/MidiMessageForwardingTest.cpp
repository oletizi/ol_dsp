/**
 * MidiMessageForwardingTest.cpp
 *
 * Unit tests for Phase 3: MidiRouter message forwarding with RouteManager integration
 * Tests: message forwarding, filtering, multi-destination routing, statistics, edge cases
 *
 * Coverage Target: 80%+
 */

#include "network/routing/MidiRouter.h"
#include "network/routing/RouteManager.h"
#include "network/routing/DeviceRegistry.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

using namespace NetworkMidi;
using namespace testing;

//==============================================================================
// Mock NetworkTransport for testing network message forwarding
//==============================================================================

class MockNetworkTransport : public NetworkTransport {
public:
    struct SentMessage {
        juce::Uuid destNode;
        uint16_t deviceId;
        std::vector<uint8_t> midiData;
    };

    void sendMidiMessage(const juce::Uuid& destNode,
                        uint16_t deviceId,
                        const std::vector<uint8_t>& midiData) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        sentMessages.push_back({destNode, deviceId, midiData});
    }

    std::vector<SentMessage> getSentMessages() const {
        std::lock_guard<std::mutex> lock(mutex);
        return sentMessages;
    }

    void clearSentMessages() {
        std::lock_guard<std::mutex> lock(mutex);
        sentMessages.clear();
    }

    int getSentMessageCount() const {
        std::lock_guard<std::mutex> lock(mutex);
        return static_cast<int>(sentMessages.size());
    }

private:
    mutable std::mutex mutex;
    std::vector<SentMessage> sentMessages;
};

//==============================================================================
// Mock MidiPortInterface for testing local message forwarding
//==============================================================================

class MockMidiPort : public MidiPortInterface {
public:
    explicit MockMidiPort(const juce::String& name, bool isInputPort)
        : portName(name), inputPort(isInputPort)
    {
    }

    void sendMessage(const std::vector<uint8_t>& data) override {
        std::lock_guard<std::mutex> lock(mutex);
        sentMessages.push_back(data);
    }

    std::vector<std::vector<uint8_t>> getMessages() override {
        std::lock_guard<std::mutex> lock(mutex);
        auto result = receivedMessages;
        receivedMessages.clear();
        return result;
    }

    juce::String getName() const override {
        return portName;
    }

    bool isInput() const override {
        return inputPort;
    }

    bool isOutput() const override {
        return !inputPort;
    }

    // Test helper methods
    void addReceivedMessage(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex);
        receivedMessages.push_back(data);
    }

    std::vector<std::vector<uint8_t>> getSentMessages() const {
        std::lock_guard<std::mutex> lock(mutex);
        return sentMessages;
    }

    void clearSentMessages() {
        std::lock_guard<std::mutex> lock(mutex);
        sentMessages.clear();
    }

    int getSentMessageCount() const {
        std::lock_guard<std::mutex> lock(mutex);
        return static_cast<int>(sentMessages.size());
    }

private:
    juce::String portName;
    bool inputPort;
    mutable std::mutex mutex;
    std::vector<std::vector<uint8_t>> sentMessages;
    std::vector<std::vector<uint8_t>> receivedMessages;
};

//==============================================================================
// MIDI Message Helpers
//==============================================================================

std::vector<uint8_t> createNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    return {static_cast<uint8_t>(0x90 | (channel - 1)), note, velocity};
}

std::vector<uint8_t> createNoteOff(uint8_t channel, uint8_t note) {
    return {static_cast<uint8_t>(0x80 | (channel - 1)), note, 0};
}

std::vector<uint8_t> createControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
    return {static_cast<uint8_t>(0xB0 | (channel - 1)), cc, value};
}

std::vector<uint8_t> createProgramChange(uint8_t channel, uint8_t program) {
    return {static_cast<uint8_t>(0xC0 | (channel - 1)), program};
}

std::vector<uint8_t> createPitchBend(uint8_t channel, uint16_t value) {
    uint8_t lsb = value & 0x7F;
    uint8_t msb = (value >> 7) & 0x7F;
    return {static_cast<uint8_t>(0xE0 | (channel - 1)), lsb, msb};
}

MidiMessageType getMidiMessageType(const std::vector<uint8_t>& midiData) {
    if (midiData.empty()) return MidiMessageType::None;

    uint8_t status = midiData[0] & 0xF0;
    switch (status) {
        case 0x80: return MidiMessageType::NoteOff;
        case 0x90: return MidiMessageType::NoteOn;
        case 0xA0: return MidiMessageType::PolyAftertouch;
        case 0xB0: return MidiMessageType::ControlChange;
        case 0xC0: return MidiMessageType::ProgramChange;
        case 0xD0: return MidiMessageType::ChannelAftertouch;
        case 0xE0: return MidiMessageType::PitchBend;
        case 0xF0: return MidiMessageType::SystemMessage;
        default: return MidiMessageType::None;
    }
}

uint8_t getMidiChannel(const std::vector<uint8_t>& midiData) {
    if (midiData.empty()) return 0;
    return (midiData[0] & 0x0F) + 1;  // Convert to 1-based channel
}

//==============================================================================
// Test Fixture
//==============================================================================

class MidiMessageForwardingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create core components
        deviceRegistry = std::make_unique<DeviceRegistry>();
        routingTable = std::make_unique<RoutingTable>();
        routeManager = std::make_unique<RouteManager>(*deviceRegistry);
        midiRouter = std::make_unique<MidiRouter>(*deviceRegistry, *routingTable);

        // Create mock network transport
        mockTransport = std::make_unique<MockNetworkTransport>();
        midiRouter->setNetworkTransport(mockTransport.get());

        // Setup test node IDs
        localNode = juce::Uuid::null();
        remoteNode1 = juce::Uuid();
        remoteNode2 = juce::Uuid();
        remoteNode3 = juce::Uuid();

        // Add test devices to registry
        deviceRegistry->addLocalDevice(1, "Local Input 1", "input", "TestVendor");
        deviceRegistry->addLocalDevice(2, "Local Output 1", "output", "TestVendor");
        deviceRegistry->addLocalDevice(3, "Local Output 2", "output", "TestVendor");

        deviceRegistry->addRemoteDevice(remoteNode1, 10, "Remote Output 1", "output", "RemoteVendor");
        deviceRegistry->addRemoteDevice(remoteNode1, 11, "Remote Input 1", "input", "RemoteVendor");
        deviceRegistry->addRemoteDevice(remoteNode2, 20, "Remote Output 2", "output", "RemoteVendor");
        deviceRegistry->addRemoteDevice(remoteNode3, 30, "Remote Output 3", "output", "RemoteVendor");

        // Add routes to routing table
        routingTable->addRoute(localNode, 1, "Local Input 1", "input");
        routingTable->addRoute(localNode, 2, "Local Output 1", "output");
        routingTable->addRoute(localNode, 3, "Local Output 2", "output");
        routingTable->addRoute(remoteNode1, 10, "Remote Output 1", "output");
        routingTable->addRoute(remoteNode1, 11, "Remote Output 2", "output");
        routingTable->addRoute(remoteNode2, 20, "Remote Output 3", "output");
        routingTable->addRoute(remoteNode3, 30, "Remote Output 4", "output");

        // Register mock local ports
        auto port2 = std::make_unique<MockMidiPort>("Local Output 1", false);
        auto port3 = std::make_unique<MockMidiPort>("Local Output 2", false);
        mockPort2 = port2.get();
        mockPort3 = port3.get();
        midiRouter->registerLocalPort(2, std::move(port2));
        midiRouter->registerLocalPort(3, std::move(port3));
    }

    void TearDown() override {
        midiRouter.reset();
        routeManager.reset();
        routingTable.reset();
        deviceRegistry.reset();
        mockTransport.reset();
    }

    // Helper to create and add a forwarding rule
    std::string addRule(const juce::Uuid& srcNode, uint16_t srcDev,
                       const juce::Uuid& dstNode, uint16_t dstDev,
                       int priority = 100, bool enabled = true) {
        ForwardingRule rule(srcNode, srcDev, dstNode, dstDev);
        rule.priority = priority;
        rule.enabled = enabled;
        return routeManager->addRule(rule);
    }

    // Helper to forward a message using RouteManager rules
    void forwardMessage(const juce::Uuid& srcNode, uint16_t srcDev,
                       const std::vector<uint8_t>& midiData) {
        // Get destinations from RouteManager
        auto destinations = routeManager->getDestinations(srcNode, srcDev);

        // Forward to each destination
        for (const auto& rule : destinations) {
            if (!rule.enabled) continue;

            // Apply filters
            uint8_t channel = getMidiChannel(midiData);
            MidiMessageType msgType = getMidiMessageType(midiData);

            if (!rule.shouldForward(channel, msgType)) {
                routeManager->updateRuleStatistics(rule.ruleId.toStdString(), false);
                continue;
            }

            // Forward the message
            midiRouter->sendMessageToNode(rule.destinationNodeId(),
                                         rule.destinationDeviceId(),
                                         midiData);

            // Update statistics
            routeManager->updateRuleStatistics(rule.ruleId.toStdString(), true);
        }
    }

    // Test infrastructure
    std::unique_ptr<DeviceRegistry> deviceRegistry;
    std::unique_ptr<RoutingTable> routingTable;
    std::unique_ptr<RouteManager> routeManager;
    std::unique_ptr<MidiRouter> midiRouter;
    std::unique_ptr<MockNetworkTransport> mockTransport;

    // Mock ports (non-owning pointers)
    MockMidiPort* mockPort2 = nullptr;
    MockMidiPort* mockPort3 = nullptr;

    // Test nodes
    juce::Uuid localNode;
    juce::Uuid remoteNode1;
    juce::Uuid remoteNode2;
    juce::Uuid remoteNode3;
};

//==============================================================================
// Basic Forwarding Tests
//==============================================================================

TEST_F(MidiMessageForwardingTest, ForwardsSingleDestination) {
    // Create rule: Local Input 1 → Remote Output 1
    addRule(localNode, 1, remoteNode1, 10);

    // Send MIDI message
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    // Verify message was forwarded
    EXPECT_EQ(1, mockTransport->getSentMessageCount());

    auto sent = mockTransport->getSentMessages();
    ASSERT_EQ(1u, sent.size());
    EXPECT_EQ(remoteNode1, sent[0].destNode);
    EXPECT_EQ(10, sent[0].deviceId);
    EXPECT_EQ(noteOn, sent[0].midiData);
}

TEST_F(MidiMessageForwardingTest, ForwardsMultipleDestinations) {
    // Create rules with different priorities
    addRule(localNode, 1, remoteNode1, 10, 200);  // Highest priority
    addRule(localNode, 1, remoteNode2, 20, 100);  // Medium priority
    addRule(localNode, 1, remoteNode3, 30, 50);   // Lowest priority

    // Send MIDI message
    auto controlChange = createControlChange(1, 7, 64);
    forwardMessage(localNode, 1, controlChange);

    // Verify all three destinations received the message (in priority order)
    EXPECT_EQ(3, mockTransport->getSentMessageCount());

    auto sent = mockTransport->getSentMessages();
    ASSERT_EQ(3u, sent.size());

    // Should be ordered by priority (highest first)
    EXPECT_EQ(remoteNode1, sent[0].destNode);
    EXPECT_EQ(10, sent[0].deviceId);

    EXPECT_EQ(remoteNode2, sent[1].destNode);
    EXPECT_EQ(20, sent[1].deviceId);

    EXPECT_EQ(remoteNode3, sent[2].destNode);
    EXPECT_EQ(30, sent[2].deviceId);
}

TEST_F(MidiMessageForwardingTest, NoForwardingWhenNoRules) {
    // Don't add any rules

    // Send MIDI message
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    // Verify no messages were forwarded
    EXPECT_EQ(0, mockTransport->getSentMessageCount());
}

TEST_F(MidiMessageForwardingTest, DisabledRulesNotUsed) {
    // Create enabled and disabled rules
    addRule(localNode, 1, remoteNode1, 10, 100, true);   // Enabled
    addRule(localNode, 1, remoteNode2, 20, 100, false);  // Disabled

    // Send MIDI message
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    // Verify only enabled rule forwarded
    EXPECT_EQ(1, mockTransport->getSentMessageCount());

    auto sent = mockTransport->getSentMessages();
    ASSERT_EQ(1u, sent.size());
    EXPECT_EQ(remoteNode1, sent[0].destNode);
}

//==============================================================================
// Filter Tests - Channel Filtering
//==============================================================================

TEST_F(MidiMessageForwardingTest, ChannelFilterMatches) {
    // Create rule with channel 1 filter
    ForwardingRule rule(localNode, 1, remoteNode1, 10);
    rule.channelFilter = ChannelFilter{1};
    auto ruleId = routeManager->addRule(rule);

    // Send message on channel 1 (should forward)
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    EXPECT_EQ(1, mockTransport->getSentMessageCount());

    // Check statistics
    auto updatedRule = routeManager->getRule(ruleId);
    ASSERT_TRUE(updatedRule.has_value());
    EXPECT_EQ(1u, updatedRule->statistics.messagesForwarded);
    EXPECT_EQ(0u, updatedRule->statistics.messagesDropped);
}

TEST_F(MidiMessageForwardingTest, ChannelFilterRejects) {
    // Create rule with channel 1 filter
    ForwardingRule rule(localNode, 1, remoteNode1, 10);
    rule.channelFilter = ChannelFilter{1};
    auto ruleId = routeManager->addRule(rule);

    // Send message on channel 2 (should NOT forward)
    auto noteOn = createNoteOn(2, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    EXPECT_EQ(0, mockTransport->getSentMessageCount());

    // Check statistics
    auto updatedRule = routeManager->getRule(ruleId);
    ASSERT_TRUE(updatedRule.has_value());
    EXPECT_EQ(0u, updatedRule->statistics.messagesForwarded);
    EXPECT_EQ(1u, updatedRule->statistics.messagesDropped);
}

TEST_F(MidiMessageForwardingTest, NoChannelFilterForwardsAll) {
    // Create rule without channel filter
    addRule(localNode, 1, remoteNode1, 10);

    // Send messages on different channels
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));
    forwardMessage(localNode, 1, createNoteOn(5, 64, 100));
    forwardMessage(localNode, 1, createNoteOn(16, 72, 100));

    // All should be forwarded
    EXPECT_EQ(3, mockTransport->getSentMessageCount());
}

//==============================================================================
// Filter Tests - Message Type Filtering
//==============================================================================

TEST_F(MidiMessageForwardingTest, MessageTypeFilterNoteOnly) {
    // Create rule that only forwards note messages
    ForwardingRule rule(localNode, 1, remoteNode1, 10);
    rule.messageTypeFilter = MidiMessageType::NoteOn | MidiMessageType::NoteOff;
    routeManager->addRule(rule);

    // Send different message types
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));      // Should forward
    forwardMessage(localNode, 1, createNoteOff(1, 60));          // Should forward
    forwardMessage(localNode, 1, createControlChange(1, 7, 64)); // Should NOT forward
    forwardMessage(localNode, 1, createProgramChange(1, 10));    // Should NOT forward

    // Only note messages should be forwarded
    EXPECT_EQ(2, mockTransport->getSentMessageCount());
}

TEST_F(MidiMessageForwardingTest, MessageTypeFilterControlChange) {
    // Create rule that only forwards control changes
    ForwardingRule rule(localNode, 1, remoteNode1, 10);
    rule.messageTypeFilter = MidiMessageType::ControlChange;
    routeManager->addRule(rule);

    // Send different message types
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));      // Should NOT forward
    forwardMessage(localNode, 1, createControlChange(1, 7, 64)); // Should forward
    forwardMessage(localNode, 1, createControlChange(1, 10, 127)); // Should forward
    forwardMessage(localNode, 1, createPitchBend(1, 8192));      // Should NOT forward

    // Only CC messages should be forwarded
    EXPECT_EQ(2, mockTransport->getSentMessageCount());
}

TEST_F(MidiMessageForwardingTest, MessageTypeFilterAll) {
    // Create rule with default filter (All)
    addRule(localNode, 1, remoteNode1, 10);

    // Send different message types
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));
    forwardMessage(localNode, 1, createControlChange(1, 7, 64));
    forwardMessage(localNode, 1, createProgramChange(1, 10));
    forwardMessage(localNode, 1, createPitchBend(1, 8192));

    // All should be forwarded
    EXPECT_EQ(4, mockTransport->getSentMessageCount());
}

//==============================================================================
// Filter Tests - Combined Filters
//==============================================================================

TEST_F(MidiMessageForwardingTest, CombinedChannelAndMessageTypeFilters) {
    // Create rule: Channel 1 AND Note messages only
    ForwardingRule rule(localNode, 1, remoteNode1, 10);
    rule.channelFilter = ChannelFilter{1};
    rule.messageTypeFilter = MidiMessageType::NoteOn | MidiMessageType::NoteOff;
    routeManager->addRule(rule);

    // Test various combinations
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));      // Ch1 + Note → Forward
    forwardMessage(localNode, 1, createNoteOn(2, 60, 100));      // Ch2 + Note → Reject
    forwardMessage(localNode, 1, createControlChange(1, 7, 64)); // Ch1 + CC → Reject
    forwardMessage(localNode, 1, createControlChange(2, 7, 64)); // Ch2 + CC → Reject

    // Only first message should be forwarded
    EXPECT_EQ(1, mockTransport->getSentMessageCount());
}

//==============================================================================
// Statistics Tests
//==============================================================================

TEST_F(MidiMessageForwardingTest, StatisticsIncrementForwarded) {
    auto ruleId = addRule(localNode, 1, remoteNode1, 10);

    // Forward multiple messages
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));
    forwardMessage(localNode, 1, createNoteOn(1, 64, 100));
    forwardMessage(localNode, 1, createNoteOn(1, 67, 100));

    // Check statistics
    auto rule = routeManager->getRule(ruleId);
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(3u, rule->statistics.messagesForwarded);
    EXPECT_EQ(0u, rule->statistics.messagesDropped);
}

TEST_F(MidiMessageForwardingTest, StatisticsIncrementDropped) {
    // Create rule with channel 1 filter
    ForwardingRule rule(localNode, 1, remoteNode1, 10);
    rule.channelFilter = ChannelFilter{1};
    auto ruleId = routeManager->addRule(rule);

    // Send messages on wrong channel
    forwardMessage(localNode, 1, createNoteOn(2, 60, 100));
    forwardMessage(localNode, 1, createNoteOn(3, 64, 100));

    // Check statistics
    auto updatedRule = routeManager->getRule(ruleId);
    ASSERT_TRUE(updatedRule.has_value());
    EXPECT_EQ(0u, updatedRule->statistics.messagesForwarded);
    EXPECT_EQ(2u, updatedRule->statistics.messagesDropped);
}

TEST_F(MidiMessageForwardingTest, StatisticsMultipleRules) {
    auto ruleId1 = addRule(localNode, 1, remoteNode1, 10, 100);
    auto ruleId2 = addRule(localNode, 1, remoteNode2, 20, 100);

    // Forward messages (both rules should be triggered)
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));
    forwardMessage(localNode, 1, createNoteOn(1, 64, 100));

    // Check statistics for both rules
    auto rule1 = routeManager->getRule(ruleId1);
    auto rule2 = routeManager->getRule(ruleId2);

    ASSERT_TRUE(rule1.has_value());
    ASSERT_TRUE(rule2.has_value());

    EXPECT_EQ(2u, rule1->statistics.messagesForwarded);
    EXPECT_EQ(2u, rule2->statistics.messagesForwarded);
}

TEST_F(MidiMessageForwardingTest, AggregateStatistics) {
    addRule(localNode, 1, remoteNode1, 10);
    addRule(localNode, 1, remoteNode2, 20);

    // Forward multiple messages
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));
    forwardMessage(localNode, 1, createNoteOn(1, 64, 100));
    forwardMessage(localNode, 1, createNoteOn(1, 67, 100));

    // Check aggregate statistics
    auto stats = routeManager->getStatistics();
    EXPECT_EQ(2u, stats.enabledRules);
    EXPECT_EQ(6u, stats.totalMessagesForwarded);  // 3 messages × 2 rules
}

//==============================================================================
// Multi-Hop Forwarding Tests
//==============================================================================

TEST_F(MidiMessageForwardingTest, MultiHopForwarding) {
    // Setup chain: Local Input 1 → Remote Output 1 → Remote Output 2
    addRule(localNode, 1, remoteNode1, 10);
    addRule(remoteNode1, 10, remoteNode2, 20);

    // Send message from local node
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    // First hop: Local → Remote1
    EXPECT_EQ(1, mockTransport->getSentMessageCount());

    // Simulate Remote1 receiving and forwarding
    mockTransport->clearSentMessages();
    forwardMessage(remoteNode1, 10, noteOn);

    // Second hop: Remote1 → Remote2
    EXPECT_EQ(1, mockTransport->getSentMessageCount());

    auto sent = mockTransport->getSentMessages();
    ASSERT_EQ(1u, sent.size());
    EXPECT_EQ(remoteNode2, sent[0].destNode);
    EXPECT_EQ(20, sent[0].deviceId);
}

//==============================================================================
// Edge Cases and Error Handling
//==============================================================================

TEST_F(MidiMessageForwardingTest, EmptyMidiMessageHandling) {
    addRule(localNode, 1, remoteNode1, 10);

    // Try to forward empty message
    std::vector<uint8_t> emptyMessage;

    // MidiRouter should reject empty messages
    midiRouter->sendMessageToNode(remoteNode1, 10, emptyMessage);

    // No message should be sent
    EXPECT_EQ(0, mockTransport->getSentMessageCount());
}

TEST_F(MidiMessageForwardingTest, InvalidMidiDataHandling) {
    addRule(localNode, 1, remoteNode1, 10);

    // Create malformed MIDI data (incomplete message)
    std::vector<uint8_t> invalidMessage = {0x90};  // Note On without note/velocity

    // Forward should still work (router doesn't validate MIDI content)
    forwardMessage(localNode, 1, invalidMessage);

    EXPECT_EQ(1, mockTransport->getSentMessageCount());
}

TEST_F(MidiMessageForwardingTest, NullNetworkTransportHandling) {
    // Remove network transport
    midiRouter->setNetworkTransport(nullptr);

    addRule(localNode, 1, remoteNode1, 10);

    // Try to forward (should fail gracefully)
    auto noteOn = createNoteOn(1, 60, 100);
    midiRouter->sendMessageToNode(remoteNode1, 10, noteOn);

    // Check router statistics show error
    auto stats = midiRouter->getStatistics();
    EXPECT_EQ(1u, stats.routingErrors);
}

TEST_F(MidiMessageForwardingTest, UnknownSourceDevice) {
    // Don't add any rules for device 99

    // Try to forward from non-existent source
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 99, noteOn);

    // No messages should be forwarded
    EXPECT_EQ(0, mockTransport->getSentMessageCount());
}

TEST_F(MidiMessageForwardingTest, UnknownDestinationDevice) {
    // Add rule to non-existent destination (validation should catch this)
    ForwardingRule rule(localNode, 1, remoteNode1, 999);

    EXPECT_THROW(routeManager->addRule(rule), std::runtime_error);
}

//==============================================================================
// Local Port Forwarding Tests
//==============================================================================

TEST_F(MidiMessageForwardingTest, ForwardsToLocalPort) {
    // Add routing table entry for local device
    routingTable->addRoute(localNode, 2, "Local Output 1", "output");

    // Create rule: Local Input 1 → Local Output 1
    addRule(localNode, 1, localNode, 2);

    // Send MIDI message
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    // Verify local port received message (not network transport)
    EXPECT_EQ(0, mockTransport->getSentMessageCount());
    EXPECT_EQ(1, mockPort2->getSentMessageCount());

    auto sent = mockPort2->getSentMessages();
    ASSERT_EQ(1u, sent.size());
    EXPECT_EQ(noteOn, sent[0]);
}

TEST_F(MidiMessageForwardingTest, ForwardsMixedLocalAndRemote) {
    // Add routes
    routingTable->addRoute(localNode, 2, "Local Output 1", "output");

    // Create rules to both local and remote destinations
    addRule(localNode, 1, localNode, 2, 100);    // Local
    addRule(localNode, 1, remoteNode1, 10, 100); // Remote

    // Send MIDI message
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    // Verify both destinations received message
    EXPECT_EQ(1, mockPort2->getSentMessageCount());
    EXPECT_EQ(1, mockTransport->getSentMessageCount());
}

//==============================================================================
// Priority and Rule Ordering Tests
//==============================================================================

TEST_F(MidiMessageForwardingTest, RespectsPriorityOrdering) {
    // Add rules with different priorities
    addRule(localNode, 1, remoteNode3, 30, 50);   // Lowest priority
    addRule(localNode, 1, remoteNode1, 10, 200);  // Highest priority
    addRule(localNode, 1, remoteNode2, 20, 100);  // Medium priority

    // Forward message
    auto noteOn = createNoteOn(1, 60, 100);
    forwardMessage(localNode, 1, noteOn);

    // Verify messages sent in priority order (highest first)
    auto sent = mockTransport->getSentMessages();
    ASSERT_EQ(3u, sent.size());

    EXPECT_EQ(remoteNode1, sent[0].destNode);  // Priority 200
    EXPECT_EQ(remoteNode2, sent[1].destNode);  // Priority 100
    EXPECT_EQ(remoteNode3, sent[2].destNode);  // Priority 50
}

//==============================================================================
// Performance and Stress Tests
//==============================================================================

TEST_F(MidiMessageForwardingTest, HandlesHighMessageThroughput) {
    addRule(localNode, 1, remoteNode1, 10);

    // Send many messages rapidly
    const int messageCount = 1000;
    for (int i = 0; i < messageCount; ++i) {
        auto noteOn = createNoteOn(1, 60 + (i % 12), 100);
        forwardMessage(localNode, 1, noteOn);
    }

    // All messages should be forwarded
    EXPECT_EQ(messageCount, mockTransport->getSentMessageCount());
}

TEST_F(MidiMessageForwardingTest, HandlesMultipleConcurrentSources) {
    // Add rules from multiple sources
    addRule(localNode, 1, remoteNode1, 10);
    addRule(localNode, 2, remoteNode1, 10);
    addRule(localNode, 3, remoteNode1, 10);

    // Send from multiple sources
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));
    forwardMessage(localNode, 2, createNoteOn(2, 64, 100));
    forwardMessage(localNode, 3, createNoteOn(3, 67, 100));

    // All messages should be forwarded
    EXPECT_EQ(3, mockTransport->getSentMessageCount());
}

//==============================================================================
// Thread Safety Tests
//==============================================================================

TEST_F(MidiMessageForwardingTest, ThreadSafeMessageForwarding) {
    addRule(localNode, 1, remoteNode1, 10);

    std::atomic<int> messagesForwarded{0};
    std::vector<std::thread> threads;

    // Forward messages from multiple threads
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, &messagesForwarded, i]() {
            for (int j = 0; j < 20; ++j) {
                auto noteOn = createNoteOn(1, 60 + (j % 12), 100);
                forwardMessage(localNode, 1, noteOn);
                messagesForwarded++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All messages should be forwarded
    EXPECT_EQ(100, messagesForwarded);
    EXPECT_EQ(100, mockTransport->getSentMessageCount());
}

TEST_F(MidiMessageForwardingTest, ThreadSafeRuleModification) {
    std::vector<std::string> ruleIds;

    // Add initial rules
    for (int i = 0; i < 5; ++i) {
        auto id = addRule(localNode, 1, remoteNode1, 10, 100 + i);
        ruleIds.push_back(id);
    }

    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    // Thread that modifies rules
    threads.emplace_back([this, &running, &ruleIds]() {
        int count = 0;
        while (running && count < 10) {
            // Add and remove rules
            auto newId = addRule(localNode, 1, remoteNode2, 20, 150);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            routeManager->removeRule(newId);
            count++;
        }
    });

    // Thread that forwards messages
    threads.emplace_back([this, &running]() {
        while (running) {
            forwardMessage(localNode, 1, createNoteOn(1, 60, 100));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Let threads run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;

    for (auto& thread : threads) {
        thread.join();
    }

    // Should not crash and should have forwarded some messages
    EXPECT_GT(mockTransport->getSentMessageCount(), 0);
}

//==============================================================================
// Integration Tests with RouteManager
//==============================================================================

TEST_F(MidiMessageForwardingTest, IntegrationWithRouteManagerFilters) {
    // Create complex rule with multiple filters
    ForwardingRule rule(localNode, 1, remoteNode1, 10);
    rule.priority = 100;
    rule.enabled = true;
    rule.channelFilter = ChannelFilter{1};
    rule.messageTypeFilter = MidiMessageType::NoteOn | MidiMessageType::NoteOff;
    auto ruleId = routeManager->addRule(rule);

    // Send various messages
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));      // Match: Ch1 + NoteOn
    forwardMessage(localNode, 1, createNoteOff(1, 60));          // Match: Ch1 + NoteOff
    forwardMessage(localNode, 1, createControlChange(1, 7, 64)); // Reject: wrong type
    forwardMessage(localNode, 1, createNoteOn(2, 64, 100));      // Reject: wrong channel

    // Only 2 messages should be forwarded
    EXPECT_EQ(2, mockTransport->getSentMessageCount());

    // Verify statistics
    auto updatedRule = routeManager->getRule(ruleId);
    ASSERT_TRUE(updatedRule.has_value());
    EXPECT_EQ(2u, updatedRule->statistics.messagesForwarded);
    EXPECT_EQ(2u, updatedRule->statistics.messagesDropped);
}

TEST_F(MidiMessageForwardingTest, DynamicRuleUpdatesDuringForwarding) {
    auto ruleId = addRule(localNode, 1, remoteNode1, 10);

    // Forward some messages
    forwardMessage(localNode, 1, createNoteOn(1, 60, 100));
    EXPECT_EQ(1, mockTransport->getSentMessageCount());

    // Disable the rule
    auto rule = routeManager->getRule(ruleId);
    ASSERT_TRUE(rule.has_value());
    rule->enabled = false;
    routeManager->updateRule(ruleId, *rule);

    // Forward more messages (should not forward)
    mockTransport->clearSentMessages();
    forwardMessage(localNode, 1, createNoteOn(1, 64, 100));
    EXPECT_EQ(0, mockTransport->getSentMessageCount());

    // Re-enable the rule
    rule->enabled = true;
    routeManager->updateRule(ruleId, *rule);

    // Forward again (should forward)
    forwardMessage(localNode, 1, createNoteOn(1, 67, 100));
    EXPECT_EQ(1, mockTransport->getSentMessageCount());
}
