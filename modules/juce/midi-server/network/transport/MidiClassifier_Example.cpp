/**
 * Integration Example: Using MidiClassifier and MidiMessageRouter
 *
 * This example demonstrates how to integrate message classification and routing
 * into a MIDI input callback for dual-transport MIDI architecture.
 */

#include "MidiClassifier.h"
#include "MidiMessageRouter.h"
#include "UdpMidiTransport.h"
#include "ReliableTransport.h"
#include <juce_audio_devices/juce_audio_devices.h>

using namespace NetworkMidi;

/**
 * Example MIDI input handler that routes messages to appropriate transports.
 *
 * This class demonstrates the integration point for Phase C.4 of the
 * dual-transport MIDI architecture.
 */
class MidiInputHandler : public juce::MidiInputCallback {
public:
    /**
     * Constructor.
     *
     * @param router Message router that handles classification and transport selection
     * @param destNode Destination node UUID for outgoing messages
     * @param destAddress Destination IP address
     * @param destPort Destination port number
     */
    MidiInputHandler(
        MidiMessageRouter& router,
        const juce::Uuid& destNode,
        const juce::String& destAddress,
        int destPort
    )
        : router(router)
        , destNode(destNode)
        , destAddress(destAddress)
        , destPort(destPort)
    {
    }

    /**
     * MIDI input callback - called on MIDI input thread.
     *
     * This is where we integrate the classifier and router:
     * 1. Receive MIDI message from input device
     * 2. Router classifies message (real-time vs non-real-time)
     * 3. Router sends to appropriate transport (UDP vs TCP)
     *
     * Thread-safe: Can be called from multiple MIDI input threads.
     * Real-time safe: No allocation, no locks, no blocking.
     */
    void handleIncomingMidiMessage(
        juce::MidiInput* source,
        const juce::MidiMessage& message
    ) override {
        // Get device ID from source (implementation-specific)
        uint16_t deviceId = getDeviceIdForSource(source);

        // Route message - classification and transport selection happen automatically
        bool success = router.routeMessage(
            message,
            deviceId,
            destNode,
            destAddress,
            destPort
        );

        if (!success) {
            // Log error (optional)
            messagesDropped++;
        }
    }

    /**
     * Get statistics.
     */
    uint64_t getMessagesDropped() const {
        return messagesDropped.load();
    }

private:
    /**
     * Map MIDI input source to device ID.
     *
     * Implementation-specific - could use:
     * - Device registry lookup
     * - Source identifier hash
     * - Pre-configured mapping
     */
    uint16_t getDeviceIdForSource(juce::MidiInput* source) const {
        // Example implementation: use index as device ID
        auto devices = juce::MidiInput::getAvailableDevices();
        for (int i = 0; i < devices.size(); ++i) {
            if (devices[i].identifier == source->getIdentifier()) {
                return static_cast<uint16_t>(i);
            }
        }
        return 0;  // Default device ID
    }

    MidiMessageRouter& router;
    juce::Uuid destNode;
    juce::String destAddress;
    int destPort;

    std::atomic<uint64_t> messagesDropped{0};
};

/**
 * Example application showing complete setup.
 */
class MidiRoutingExample {
public:
    MidiRoutingExample() {
        // Create transports
        udpTransport = std::make_unique<UdpMidiTransport>(5004);
        udpTransport->setNodeId(juce::Uuid());
        udpTransport->start();

        reliableTransport = std::make_unique<ReliableTransport>(*udpTransport);

        // Create router
        router = std::make_unique<MidiMessageRouter>(*udpTransport, *reliableTransport);

        // Enable detailed statistics tracking
        router->setDetailedTracking(true);

        // Set up error callback
        router->onRoutingError = [](const juce::String& error, const juce::MidiMessage& msg) {
            DBG("Routing error: " << error);
            DBG("Message: " << msg.getDescription());
        };

        // Create MIDI input handler
        inputHandler = std::make_unique<MidiInputHandler>(
            *router,
            juce::Uuid(),  // Destination node UUID
            "192.168.1.100",  // Destination IP
            5004  // Destination port
        );

        // Open MIDI input device
        auto devices = juce::MidiInput::getAvailableDevices();
        if (!devices.isEmpty()) {
            midiInput = juce::MidiInput::openDevice(devices[0].identifier, inputHandler.get());
            if (midiInput) {
                midiInput->start();
                DBG("MIDI input started: " << devices[0].name);
            }
        }
    }

    ~MidiRoutingExample() {
        if (midiInput) {
            midiInput->stop();
        }
        udpTransport->stop();
    }

    /**
     * Print statistics.
     */
    void printStatistics() {
        auto stats = router->getStatistics();

        DBG("===== MIDI Routing Statistics =====");
        DBG("Real-time messages sent (UDP):  " << stats.realtimeMessagesSent);
        DBG("Non-real-time messages sent (TCP): " << stats.nonRealtimeMessagesSent);
        DBG("Routing errors:                 " << stats.routingErrors);
        DBG("Total bytes sent:               " << stats.totalBytesSent);
        DBG("");
        DBG("Message type breakdown:");
        DBG("  Note On/Off:      " << stats.noteMessages);
        DBG("  Control Change:   " << stats.controlChangeMessages);
        DBG("  MIDI Clock:       " << stats.clockMessages);
        DBG("  SysEx:            " << stats.sysexMessages);
        DBG("  Other:            " << stats.otherMessages);
        DBG("");
        DBG("Messages dropped:  " << inputHandler->getMessagesDropped());
    }

    /**
     * Test message classification.
     */
    void testClassification() {
        DBG("===== Testing Message Classification =====");

        // Create test messages
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
        juce::MidiMessage cc = juce::MidiMessage::controllerEvent(1, 7, 127);
        juce::MidiMessage clock = juce::MidiMessage::midiClock();

        uint8_t sysexData[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
        juce::MidiMessage sysex(sysexData, 5);

        // Classify and explain
        DBG("Note On: " << explainClassification(noteOn));
        DBG("Control Change: " << explainClassification(cc));
        DBG("MIDI Clock: " << explainClassification(clock));
        DBG("SysEx: " << explainClassification(sysex));
    }

private:
    std::unique_ptr<UdpMidiTransport> udpTransport;
    std::unique_ptr<ReliableTransport> reliableTransport;
    std::unique_ptr<MidiMessageRouter> router;
    std::unique_ptr<MidiInputHandler> inputHandler;
    std::unique_ptr<juce::MidiInput> midiInput;
};

/**
 * Usage Example:
 *
 * int main() {
 *     juce::MessageManager::getInstance();
 *
 *     // Create and initialize
 *     MidiRoutingExample example;
 *
 *     // Test classification
 *     example.testClassification();
 *
 *     // Run for a while...
 *     juce::Thread::sleep(10000);  // 10 seconds
 *
 *     // Print statistics
 *     example.printStatistics();
 *
 *     return 0;
 * }
 */
