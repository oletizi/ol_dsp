//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_SERIALIZER_H
#define OL_DSP_SERIALIZER_H

#include "Control.h"
#include "Serial.h"

namespace ol::io {
    class ControlListener {
        virtual void HandleControl(ol::ctl::Control control) = 0;
    };

    class Serializer {
        virtual void WriteControl(ol::ctl::Control) = 0;
    };

    class Deserializer {
        virtual void Read(uint8_t *buffer) = 0;

        virtual void AddControlListener(ControlListener &) = 0;
    };

    class SimpleSerialzer : public Serializer, Deserializer {

    public:
        explicit SimpleSerialzer(Serial *serial) : serial_(serial) {}

        void WriteControl(ol::ctl::Control control) override {

        }

        void AddControlListener(ControlListener &listener) override {

        }

        void Read(uint8_t *buffer) override {

        }

    private:
        Serial *serial_;
    };
}


#endif //OL_DSP_SERIALIZER_H
