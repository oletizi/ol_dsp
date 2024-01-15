//
// Created by Orion Letizi on 11/29/23.
//

#ifndef OL_DSP_DAISY_DUMMY_H
#define OL_DSP_DAISY_DUMMY_H

#include <cstddef>
#include <cstdint>

#define DSY_SDRAM_BSS
namespace daisy {
    struct Led {
        void SetRed(float);

        void SetGreen(float);

        void SetBlue(float);
    };

    struct Button {
        bool RisingEdge();
    };

    struct Knob {
        void Process();

        float Value();
    };

    struct InputBuffer {
    };
    struct OutputBuffer {
    };

    struct AudioHandle {
        /** Interleaving Input buffer
         ** audio is prepared as { L0, R0, L1, R1, . . . LN, RN }]
         ** this is const, as the user shouldn't modify it
        */
        typedef const float *InterleavingInputBuffer;

        /** Interleaving Output buffer
         ** audio is prepared as { L0, R0, L1, R1, . . . LN, RN }
        */
        typedef float *InterleavingOutputBuffer;

        typedef void (*InterleavingAudioCallback)(InterleavingInputBuffer in,
                                                  InterleavingOutputBuffer out,
                                                  size_t size);

        typedef void (*AudioCallback)(InputBuffer in,
                                      OutputBuffer out,
                                      size_t size);
    };

    struct System {
        static int GetNow();

        static void Delay(int);
    };


    enum MidiType {
        NoteOn,
        NoteOff,
        ControlChange
    };

    struct NoteOnEvent
    {
        int     channel;  /**< & */
        uint8_t note;     /**< & */
        uint8_t velocity; /**< & */
    };
    struct NoteOffEvent
    {
        int     channel;  /**< & */
        uint8_t note;     /**< & */
        uint8_t velocity; /**< & */
    };

    struct ControlChangeEvent {
        int control_number;
        int value;
    };

    struct MidiEvent {
        int channel;
        MidiType type;

        NoteOnEvent AsNoteOn();

        NoteOffEvent AsNoteOff();

        ControlChangeEvent AsControlChange();
    };

    struct MidiUartHandler {
        void StartReceive();

        void Listen();

        bool HasEvents();

        MidiEvent PopEvent();
    };

    struct DaisyPod {
        Led led1;
        Led led2;
        Button button1;
        Button button2;
        Knob knob1;
        Knob knob2;
        MidiUartHandler midi;

        void Init();

        void UpdateLeds();

        void ProcessAllControls();

        void SetAudioBlockSize(int);

        float AudioSampleRate();

        void StartAdc();;

        void StartAudio(AudioHandle::InterleavingAudioCallback cb);;

        void StartAudio(AudioHandle::AudioCallback cb);;

    };

    struct DaisySeed {
        static void StartLog(bool b);

        template<typename... VA>
        inline static void PrintLine(const char *format, VA... va) {};

        static void PrintLine(const char *message);
    };


}

#endif //OL_DSP_DAISY_DUMMY_H
