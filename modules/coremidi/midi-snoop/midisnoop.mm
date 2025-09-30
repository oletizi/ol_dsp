#import <Foundation/Foundation.h>
#import <CoreMIDI/CoreMIDI.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>

// ANSI color codes for output
#define COLOR_INPUT "\033[32m"   // Green for input
#define COLOR_OUTPUT "\033[34m"  // Blue for output
#define COLOR_RESET "\033[0m"
#define COLOR_HEADER "\033[1;33m" // Bold yellow for headers
#define COLOR_ERROR "\033[1;31m"  // Bold red for errors

struct MIDISourceInfo {
    MIDIEndpointRef endpoint;
    std::string name;
    bool isInput;
};

// Format MIDI message bytes for display
std::string formatMIDIBytes(const Byte* data, size_t length) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < length; i++) {
        ss << std::setw(2) << static_cast<int>(data[i]);
        if (i < length - 1) ss << " ";
    }
    return ss.str();
}

// Decode MIDI message type
std::string getMIDIMessageType(Byte statusByte) {
    Byte messageType = statusByte & 0xF0;
    Byte channel = statusByte & 0x0F;

    std::stringstream ss;
    switch (messageType) {
        case 0x80: ss << "Note Off (ch " << (int)channel + 1 << ")"; break;
        case 0x90: ss << "Note On (ch " << (int)channel + 1 << ")"; break;
        case 0xA0: ss << "Poly Aftertouch (ch " << (int)channel + 1 << ")"; break;
        case 0xB0: ss << "Control Change (ch " << (int)channel + 1 << ")"; break;
        case 0xC0: ss << "Program Change (ch " << (int)channel + 1 << ")"; break;
        case 0xD0: ss << "Channel Aftertouch (ch " << (int)channel + 1 << ")"; break;
        case 0xE0: ss << "Pitch Bend (ch " << (int)channel + 1 << ")"; break;
        case 0xF0:
            switch (statusByte) {
                case 0xF0: ss << "SysEx Start"; break;
                case 0xF1: ss << "MIDI Time Code"; break;
                case 0xF2: ss << "Song Position"; break;
                case 0xF3: ss << "Song Select"; break;
                case 0xF6: ss << "Tune Request"; break;
                case 0xF7: ss << "SysEx End"; break;
                case 0xF8: ss << "Clock"; break;
                case 0xFA: ss << "Start"; break;
                case 0xFB: ss << "Continue"; break;
                case 0xFC: ss << "Stop"; break;
                case 0xFE: ss << "Active Sensing"; break;
                case 0xFF: ss << "System Reset"; break;
                default: ss << "Unknown System"; break;
            }
            break;
        default: ss << "Unknown"; break;
    }
    return ss.str();
}

// MIDI read callback for inputs
void midiInputCallback(const MIDIPacketList* packetList, void* readProcRefCon, void* srcConnRefCon) {
    MIDISourceInfo* sourceInfo = static_cast<MIDISourceInfo*>(srcConnRefCon);

    const MIDIPacket* packet = &packetList->packet[0];
    for (UInt32 i = 0; i < packetList->numPackets; i++) {
        std::cout << COLOR_INPUT << "[INPUT] " << COLOR_RESET
                  << sourceInfo->name << " : "
                  << getMIDIMessageType(packet->data[0]) << " : "
                  << formatMIDIBytes(packet->data, packet->length) << std::endl;

        packet = MIDIPacketNext(packet);
    }
}

// MIDI read callback for outputs (monitoring)
void midiOutputCallback(const MIDIPacketList* packetList, void* readProcRefCon, void* srcConnRefCon) {
    MIDISourceInfo* sourceInfo = static_cast<MIDISourceInfo*>(srcConnRefCon);

    const MIDIPacket* packet = &packetList->packet[0];
    for (UInt32 i = 0; i < packetList->numPackets; i++) {
        std::cout << COLOR_OUTPUT << "[OUTPUT] " << COLOR_RESET
                  << sourceInfo->name << " : "
                  << getMIDIMessageType(packet->data[0]) << " : "
                  << formatMIDIBytes(packet->data, packet->length) << std::endl;

        packet = MIDIPacketNext(packet);
    }
}

// Get endpoint name
std::string getEndpointName(MIDIEndpointRef endpoint) {
    CFStringRef name = nullptr;
    char nameBuf[256];

    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name);
    if (name) {
        CFStringGetCString(name, nameBuf, sizeof(nameBuf), kCFStringEncodingUTF8);
        CFRelease(name);
        return std::string(nameBuf);
    }
    return "Unknown";
}

int main(int argc, char* argv[]) {
    @autoreleasepool {
        std::cout << COLOR_HEADER << "=== CoreMIDI Spy ===" << COLOR_RESET << std::endl;
        std::cout << "Monitoring all MIDI inputs and outputs. Press Ctrl+C to exit." << std::endl << std::endl;

        MIDIClientRef client = 0;
        MIDIPortRef inputPort = 0;
        MIDIPortRef outputPort = 0;

        // Create MIDI client
        OSStatus status = MIDIClientCreate(CFSTR("MIDI Spy"), nullptr, nullptr, &client);
        if (status != noErr) {
            std::cerr << COLOR_ERROR << "Error: Failed to create MIDI client (status: " << status << ")" << COLOR_RESET << std::endl;
            return 1;
        }

        // Create input port for monitoring sources
        status = MIDIInputPortCreate(client, CFSTR("Spy Input Port"), midiInputCallback, nullptr, &inputPort);
        if (status != noErr) {
            std::cerr << COLOR_ERROR << "Error: Failed to create input port (status: " << status << ")" << COLOR_RESET << std::endl;
            return 1;
        }

        // Create input port for monitoring destinations (outputs)
        status = MIDIInputPortCreate(client, CFSTR("Spy Output Port"), midiOutputCallback, nullptr, &outputPort);
        if (status != noErr) {
            std::cerr << COLOR_ERROR << "Error: Failed to create output monitoring port (status: " << status << ")" << COLOR_RESET << std::endl;
            return 1;
        }

        std::vector<MIDISourceInfo*> sourceInfos;

        // Monitor all MIDI sources (inputs)
        ItemCount sourceCount = MIDIGetNumberOfSources();
        std::cout << COLOR_HEADER << "Found " << sourceCount << " MIDI input sources:" << COLOR_RESET << std::endl;

        for (ItemCount i = 0; i < sourceCount; i++) {
            MIDIEndpointRef source = MIDIGetSource(i);
            std::string name = getEndpointName(source);
            std::cout << "  [" << i << "] " << name << std::endl;

            MIDISourceInfo* info = new MIDISourceInfo{source, name, true};
            sourceInfos.push_back(info);

            status = MIDIPortConnectSource(inputPort, source, info);
            if (status != noErr) {
                std::cerr << COLOR_ERROR << "Warning: Failed to connect to input source: " << name << COLOR_RESET << std::endl;
            }
        }

        // Monitor all MIDI destinations (outputs)
        ItemCount destCount = MIDIGetNumberOfDestinations();
        std::cout << COLOR_HEADER << "\nFound " << destCount << " MIDI output destinations:" << COLOR_RESET << std::endl;

        for (ItemCount i = 0; i < destCount; i++) {
            MIDIEndpointRef dest = MIDIGetDestination(i);
            std::string name = getEndpointName(dest);
            std::cout << "  [" << i << "] " << name << std::endl;

            MIDISourceInfo* info = new MIDISourceInfo{dest, name, false};
            sourceInfos.push_back(info);

            status = MIDIPortConnectSource(outputPort, dest, info);
            if (status != noErr) {
                std::cerr << COLOR_ERROR << "Warning: Failed to connect to output destination: " << name << COLOR_RESET << std::endl;
            }
        }

        std::cout << std::endl << COLOR_HEADER << "Listening for MIDI messages..." << COLOR_RESET << std::endl << std::endl;

        // Run loop to keep listening
        CFRunLoopRun();

        // Cleanup
        for (auto info : sourceInfos) {
            delete info;
        }

        if (inputPort) MIDIPortDispose(inputPort);
        if (outputPort) MIDIPortDispose(outputPort);
        if (client) MIDIClientDispose(client);

        return 0;
    }
}