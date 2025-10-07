/**
 * MIDI Mesh Integration Test
 *
 * Tests end-to-end MIDI routing through mesh network:
 * 1. Sends MIDI to virtual1 (Node 1 input)
 * 2. Verifies it arrives at virtual2 (Node 2 output)
 */

#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <iostream>
#include <chrono>
#include <thread>

class MidiMeshTester : public juce::MidiInputCallback
{
public:
    MidiMeshTester() = default;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        std::lock_guard<std::mutex> lock(receivedMutex);
        receivedMessages.push_back(message);

        std::cout << "Received MIDI: ";
        if (message.isNoteOn())
            std::cout << "Note On - " << message.getNoteNumber() << " vel=" << message.getVelocity();
        else if (message.isNoteOff())
            std::cout << "Note Off - " << message.getNoteNumber();
        else if (message.isController())
            std::cout << "CC " << message.getControllerNumber() << " val=" << message.getControllerValue();
        std::cout << std::endl;
    }

    bool waitForMessage(int timeoutMs = 5000)
    {
        auto startTime = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - startTime < std::chrono::milliseconds(timeoutMs))
        {
            {
                std::lock_guard<std::mutex> lock(receivedMutex);
                if (!receivedMessages.empty())
                    return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return false;
    }

    std::vector<juce::MidiMessage> getReceivedMessages()
    {
        std::lock_guard<std::mutex> lock(receivedMutex);
        return receivedMessages;
    }

private:
    std::mutex receivedMutex;
    std::vector<juce::MidiMessage> receivedMessages;
};

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\n===== MIDI Mesh Integration Test =====" << std::endl;
    std::cout << "Testing MIDI routing through mesh network\n" << std::endl;

    // Parse command line args
    std::string mode = "send";
    if (argc > 1)
        mode = argv[1];

    if (mode == "send")
    {
        // SENDER: Send MIDI to virtual1
        std::cout << "Mode: SEND to virtual1" << std::endl;

        // Find virtual1 output
        auto devices = juce::MidiOutput::getAvailableDevices();
        std::cout << "\nAvailable MIDI outputs:" << std::endl;
        for (int i = 0; i < devices.size(); i++)
            std::cout << "  " << i << ": " << devices[i].name << std::endl;

        juce::MidiOutput* midiOut = nullptr;
        for (const auto& device : devices)
        {
            if (device.name.contains("virtual1"))
            {
                midiOut = juce::MidiOutput::openDevice(device.identifier).release();
                std::cout << "\nOpened: " << device.name << std::endl;
                break;
            }
        }

        if (!midiOut)
        {
            std::cerr << "ERROR: Could not open virtual1 MIDI output" << std::endl;
            return 1;
        }

        // Send test messages
        std::cout << "\nSending test MIDI messages..." << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Note On
        auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
        midiOut->sendMessageNow(noteOn);
        std::cout << "  Sent: Note On (60, vel=100)" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Note Off
        auto noteOff = juce::MidiMessage::noteOff(1, 60);
        midiOut->sendMessageNow(noteOff);
        std::cout << "  Sent: Note Off (60)" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Control Change
        auto cc = juce::MidiMessage::controllerEvent(1, 7, 64);
        midiOut->sendMessageNow(cc);
        std::cout << "  Sent: CC 7 (val=64)" << std::endl;

        std::cout << "\nTest messages sent. Check receiver output." << std::endl;

        delete midiOut;
    }
    else if (mode == "receive")
    {
        // RECEIVER: Listen on virtual2
        std::cout << "Mode: RECEIVE from virtual2" << std::endl;

        // Find virtual2 input
        auto devices = juce::MidiInput::getAvailableDevices();
        std::cout << "\nAvailable MIDI inputs:" << std::endl;
        for (int i = 0; i < devices.size(); i++)
            std::cout << "  " << i << ": " << devices[i].name << std::endl;

        MidiMeshTester tester;
        std::unique_ptr<juce::MidiInput> midiIn;

        for (const auto& device : devices)
        {
            if (device.name.contains("virtual2"))
            {
                midiIn = juce::MidiInput::openDevice(device.identifier, &tester);
                if (midiIn)
                {
                    midiIn->start();
                    std::cout << "\nListening on: " << device.name << std::endl;
                    break;
                }
            }
        }

        if (!midiIn)
        {
            std::cerr << "ERROR: Could not open virtual2 MIDI input" << std::endl;
            return 1;
        }

        // Wait for messages
        std::cout << "\nWaiting for MIDI messages (10 seconds)..." << std::endl;

        if (tester.waitForMessage(10000))
        {
            auto messages = tester.getReceivedMessages();
            std::cout << "\n✓ SUCCESS: Received " << messages.size() << " MIDI message(s)" << std::endl;
            return 0;
        }
        else
        {
            std::cout << "\n✗ FAILED: No MIDI messages received" << std::endl;
            return 1;
        }
    }
    else
    {
        std::cerr << "Usage: " << argv[0] << " [send|receive]" << std::endl;
        return 1;
    }

    return 0;
}
