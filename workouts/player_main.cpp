#include "juce_audio_formats/juce_audio_formats.h"
#include "juce_audio_devices/juce_audio_devices.h"

int main(int argc, char *argv[]) {
    std::cout << "Hello!" << std::endl;
    std::cout << "argc: " << argc << std::endl;
    if (argc < 2) {
        std::cout << "Dude. Give me an absolute path to an audio file." << std::endl;
        exit(1);
    }
    juce::File file(argv[1]);
    if (!file.existsAsFile()) {
        std::cout << "Dude. Give me a file that exists." << std::endl;
        exit(1);
    }
    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    juce::AudioFormatManager afm;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioSourcePlayer player;

    afm.registerBasicFormats();
    juce::AudioFormat *fmt = nullptr;
    for (int i = 0; i < afm.getNumKnownFormats(); i++) {
        fmt = afm.getKnownFormat(i);
        std::cout << "Format: " << fmt->getFormatName() << std::endl;
        if (fmt->canHandleFile(file)) {
            std::cout << "  CAN handle file: " << file.getFileName() << std::endl;
            break;
        }
    }
    if (fmt != nullptr) {
        std::cout << "Done setting up audio player." << std::endl;
    } else {
        std::cout << "Unknown audio format." << std::endl;
        exit(1);
    }

    const std::unique_ptr<juce::FileInputStream> &f_stream = file.createInputStream();
    std::cout << " created input stream." << std::endl;
    juce::AudioFormatReader *f_reader = fmt->createReaderFor(f_stream.get(), true);
    std::cout << "  created AudioFormatReader." << std::endl;
    juce::AudioFormatReaderSource f_reader_source(f_reader, true);
    std::cout << "  created AudioFormatReaderSource." << std::endl;
    player.setSource(&f_reader_source);
    std::cout << "  set AudioFormatReaderSource as source for AudioSourcePlayer." << std::endl;
    player.setGain(0.5);
    deviceManager.addAudioCallback(&player);

    std::cout << "Playing an audio file: " << file.getFileName() << std::endl;
    std::cout << "t: play test sound" << std::endl;
    std::cout << "q: quit" << std::endl;
    while (auto c = getchar()) {
        if (c == 't') {
            deviceManager.playTestSound();
        }
        if (c == 'q' || c == 'Q') {
            break;
        }
    }
}