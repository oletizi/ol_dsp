/**
 * JUCE-based MIDI Device Tester for reliable SysEx handling
 *
 * This tester provides a simple command-line interface for testing
 * the Launch Control XL3 protocol with proper SysEx support.
 */

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

//==============================================================================
class MidiDeviceTester : public juce::MidiInputCallback
{
public:
    MidiDeviceTester()
    {
        // Initialize JUCE's message thread
        juce::MessageManager::getInstance();
    }

    ~MidiDeviceTester()
    {
        closeAllPorts();
    }

    void listPorts()
    {
        auto inputs = juce::MidiInput::getAvailableDevices();
        auto outputs = juce::MidiOutput::getAvailableDevices();

        std::cout << "\nAvailable MIDI Inputs:" << std::endl;
        for (const auto& device : inputs)
        {
            std::cout << "  " << device.identifier.toStdString()
                      << ": " << device.name.toStdString() << std::endl;
        }

        std::cout << "\nAvailable MIDI Outputs:" << std::endl;
        for (const auto& device : outputs)
        {
            std::cout << "  " << device.identifier.toStdString()
                      << ": " << device.name.toStdString() << std::endl;
        }
    }

    bool openInput(const juce::String& deviceName)
    {
        auto devices = juce::MidiInput::getAvailableDevices();

        for (const auto& device : devices)
        {
            if (device.name.contains(deviceName))
            {
                midiInput = juce::MidiInput::openDevice(device.identifier, this);
                if (midiInput)
                {
                    midiInput->start();
                    std::cout << "Opened input: " << device.name.toStdString() << std::endl;
                    return true;
                }
            }
        }

        std::cerr << "Failed to open input: " << deviceName.toStdString() << std::endl;
        return false;
    }

    bool openOutput(const juce::String& deviceName)
    {
        auto devices = juce::MidiOutput::getAvailableDevices();

        for (const auto& device : devices)
        {
            if (device.name.contains(deviceName))
            {
                midiOutput = juce::MidiOutput::openDevice(device.identifier);
                if (midiOutput)
                {
                    std::cout << "Opened output: " << device.name.toStdString() << std::endl;
                    return true;
                }
            }
        }

        std::cerr << "Failed to open output: " << deviceName.toStdString() << std::endl;
        return false;
    }

    bool openDawInput(const juce::String& deviceName)
    {
        auto devices = juce::MidiInput::getAvailableDevices();

        for (const auto& device : devices)
        {
            if (device.name.contains(deviceName))
            {
                dawInput = juce::MidiInput::openDevice(device.identifier, this);
                if (dawInput)
                {
                    dawInput->start();
                    std::cout << "Opened DAW input: " << device.name.toStdString() << std::endl;
                    return true;
                }
            }
        }

        std::cerr << "Failed to open DAW input: " << deviceName.toStdString() << std::endl;
        return false;
    }

    bool openDawOutput(const juce::String& deviceName)
    {
        auto devices = juce::MidiOutput::getAvailableDevices();

        for (const auto& device : devices)
        {
            if (device.name.contains(deviceName))
            {
                dawOutput = juce::MidiOutput::openDevice(device.identifier);
                if (dawOutput)
                {
                    std::cout << "Opened DAW output: " << device.name.toStdString() << std::endl;
                    return true;
                }
            }
        }

        std::cerr << "Failed to open DAW output: " << deviceName.toStdString() << std::endl;
        return false;
    }

    void closeAllPorts()
    {
        if (midiInput)
        {
            midiInput->stop();
            midiInput.reset();
        }
        midiOutput.reset();
        if (dawInput)
        {
            dawInput->stop();
            dawInput.reset();
        }
        dawOutput.reset();
    }

    // MidiInputCallback interface
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                  const juce::MidiMessage& message) override
    {
        bool isDaw = (source == dawInput.get());

        if (message.isSysEx())
        {
            auto data = message.getSysExData();
            auto size = message.getSysExDataSize();

            std::cout << "[" << (isDaw ? "DAW" : "MIDI") << " IN] SysEx ("
                      << size << " bytes): ";

            // Print first 16 bytes and last 4
            for (int i = 0; i < std::min(16, size); ++i)
            {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << std::uppercase << (int)data[i] << " ";
            }

            if (size > 16)
            {
                std::cout << "... ";
                for (int i = size - 4; i < size; ++i)
                {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << std::uppercase << (int)data[i] << " ";
                }
            }
            std::cout << std::dec << std::endl;

            // Store SysEx responses
            if (!isDaw)
            {
                lastSysExResponse.clear();
                lastSysExResponse.insert(lastSysExResponse.begin(), data, data + size);
            }
        }
        else
        {
            // Regular MIDI message
            auto rawData = message.getRawData();
            auto size = message.getRawDataSize();

            std::cout << "[" << (isDaw ? "DAW" : "MIDI") << " IN] ";
            for (int i = 0; i < size; ++i)
            {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << std::uppercase << (int)rawData[i] << " ";
            }
            std::cout << std::dec << std::endl;

            // Store DAW responses
            if (isDaw && size >= 3 && rawData[0] == 0xB6 && rawData[1] == 30)
            {
                lastDawSlotResponse = rawData[2];
            }
        }
    }

    void sendMidiMessage(const std::vector<uint8_t>& data)
    {
        if (!midiOutput) return;

        if (data.size() > 0 && data[0] == 0xF0) // SysEx
        {
            midiOutput->sendMessageNow(
                juce::MidiMessage::createSysExMessage(data.data(), (int)data.size())
            );
        }
        else
        {
            midiOutput->sendMessageNow(
                juce::MidiMessage(data.data(), (int)data.size())
            );
        }
    }

    void sendDawMessage(const std::vector<uint8_t>& data)
    {
        if (!dawOutput) return;

        dawOutput->sendMessageNow(
            juce::MidiMessage(data.data(), (int)data.size())
        );
    }

    void testLCXL3Protocol()
    {
        std::cout << "\n=== Testing Launch Control XL3 Protocol ===" << std::endl;

        // Test handshake
        std::cout << "\n1. Testing handshake..." << std::endl;
        lastSysExResponse.clear();
        sendMidiMessage({0xF0, 0x00, 0x20, 0x29, 0x00, 0x42, 0x02, 0xF7});

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (!lastSysExResponse.empty() && lastSysExResponse.size() > 7)
        {
            std::string serial;
            for (size_t i = 7; i < lastSysExResponse.size() - 1; ++i)
            {
                if (lastSysExResponse[i] >= 32 && lastSysExResponse[i] < 127)
                    serial += (char)lastSysExResponse[i];
            }
            std::cout << "   ✓ Handshake successful - Serial: " << serial << std::endl;
        }
        else
        {
            std::cout << "   ⚠️ No handshake response" << std::endl;
        }

        // Test slot selection and read/write
        for (int slot = 0; slot < 3; ++slot)
        {
            std::cout << "\n2. Testing slot " << slot << "..." << std::endl;

            // Phase 1: Query current slot
            std::cout << "   Query current slot..." << std::endl;
            lastDawSlotResponse = -1;

            sendDawMessage({0x9F, 11, 127}); // Note On ch16
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            sendDawMessage({0xB7, 30, 0});   // CC ch8 query
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            if (lastDawSlotResponse >= 0)
            {
                std::cout << "   Current slot: " << (lastDawSlotResponse - 6) << std::endl;
            }

            sendDawMessage({0x9F, 11, 0});   // Note Off ch16
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            // Phase 2: Set slot
            uint8_t ccValue = slot + 6;
            std::cout << "   Setting slot " << slot << "..." << std::endl;

            sendDawMessage({0x9F, 11, 127}); // Note On ch16
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            sendDawMessage({0xB6, 30, ccValue}); // CC ch7 set
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            sendDawMessage({0x9F, 11, 0});   // Note Off ch16
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Write test data
            std::string testName = "JUCE_S" + std::to_string(slot);
            std::vector<uint8_t> writeSysEx = {
                0xF0,
                0x00, 0x20, 0x29,
                0x02,
                0x15,
                0x05,
                0x00,
                0x45,  // Write
                0x00,  // Slot byte (always 0)
                (uint8_t)slot,  // Flag byte (actual slot)
                // Header
                0x01, 0x20, 0x10, 0x2A
            };

            // Add name
            for (char c : testName)
                writeSysEx.push_back(c);

            // Pad name to 16 bytes
            while (writeSysEx.size() < 31)
                writeSysEx.push_back(0x00);

            // Add minimal control data (48 controls)
            for (int i = 0; i < 48; ++i)
            {
                writeSysEx.push_back(0x48 + i / 24);
                writeSysEx.push_back(0x10 + i % 24);
                writeSysEx.push_back(0x02);
                writeSysEx.push_back(0x05);
                writeSysEx.push_back(0x00);
                writeSysEx.push_back(0x01);
                writeSysEx.push_back(0x40);
                writeSysEx.push_back(0x00);
                writeSysEx.push_back(i);
                writeSysEx.push_back(0x7F);
                writeSysEx.push_back(0x00);
            }

            writeSysEx.push_back(0xF7);

            std::cout << "   Writing \"" << testName << "\"..." << std::endl;
            lastSysExResponse.clear();
            sendMidiMessage(writeSysEx);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            if (!lastSysExResponse.empty() && lastSysExResponse.size() > 8 &&
                lastSysExResponse[8] == 0x15)
            {
                std::cout << "   ✓ Write acknowledged" << std::endl;
            }

            // Read back
            std::cout << "   Reading back..." << std::endl;
            std::vector<uint8_t> readSysEx = {
                0xF0,
                0x00, 0x20, 0x29,
                0x02,
                0x15,
                0x05,
                0x00,
                0x40,  // Read
                0x00,
                (uint8_t)slot,
                0xF7
            };

            lastSysExResponse.clear();
            sendMidiMessage(readSysEx);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (!lastSysExResponse.empty() && lastSysExResponse.size() > 30)
            {
                std::string readName;
                for (int i = 14; i < 30 && i < lastSysExResponse.size(); ++i)
                {
                    if (lastSysExResponse[i] >= 32 && lastSysExResponse[i] < 127)
                        readName += (char)lastSysExResponse[i];
                }

                std::cout << "   Read name: \"" << readName << "\"" << std::endl;

                if (readName == testName)
                {
                    std::cout << "   ✅ SUCCESS - Slot " << slot << " working!" << std::endl;
                }
                else
                {
                    std::cout << "   ⚠️ Name mismatch" << std::endl;
                }
            }
            else
            {
                std::cout << "   ⚠️ No read response" << std::endl;
            }
        }

        std::cout << "\n=== Test Complete ===" << std::endl;
    }

private:
    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    std::unique_ptr<juce::MidiInput> dawInput;
    std::unique_ptr<juce::MidiOutput> dawOutput;

    std::vector<uint8_t> lastSysExResponse;
    int lastDawSlotResponse = -1;
};

//==============================================================================
int main(int argc, char* argv[])
{
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    MidiDeviceTester tester;

    std::cout << "\nJUCE MIDI Device Tester for Launch Control XL3" << std::endl;
    std::cout << "===============================================" << std::endl;

    // List available ports
    tester.listPorts();

    // Open LCXL3 ports
    std::cout << "\nOpening Launch Control XL3 ports..." << std::endl;

    bool success = true;
    success &= tester.openOutput("LCXL3 1 MIDI In");
    success &= tester.openInput("LCXL3 1 MIDI Out");
    success &= tester.openDawOutput("LCXL3 1 DAW In");
    success &= tester.openDawInput("LCXL3 1 DAW Out");

    if (!success)
    {
        std::cerr << "\nFailed to open all required ports!" << std::endl;
        return 1;
    }

    std::cout << "\n✓ All ports opened successfully" << std::endl;

    // Run the test
    tester.testLCXL3Protocol();

    // Clean up
    tester.closeAllPorts();

    std::cout << "\n✓ Ports closed" << std::endl;

    return 0;
}
