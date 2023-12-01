.PHONY: all build upload monitor

PORT := $(shell ./find-port.sh)
# Set CLEAN to --clean when changing build.usbtype build property
#CLEAN := --clean
CLEAN :=
all: build upload

# --clean was needed to get midi to work right. Probably not needed for every build.
build:
	arduino-cli compile $(CLEAN) --fqbn arduino:avr:uno --build-property build.usbtype=USB_MIDI_SERIAL .

upload: build
	arduino-cli upload -p $(PORT) --fqbn arduino:avr:uno . #&& arduino-cli monitor -p $(PORT)

monitor: upload
	arduino-cli monitor -p $(PORT)
board:
	arduino-cli board list

# USB type:
# https://forum.arduino.cc/t/arduino-cli-teesnsy-notes-questions-documentation/1068881/2
#
#"Serial" -> build.usbtype=USB_SERIAL
#"Dual Serial" -> build.usbtype=USB_DUAL_SERIAL
#"Triple Serial" -> build.usbtype=USB_TRIPLE_SERIAL
#"Keyboard" -> build.usbtype=USB_KEYBOARDONLY
#"Keyboard + Touch Screen" -> build.usbtype=USB_TOUCHSCREEN
#"Keyboard + Mouse + Touch Screen" -> build.usbtype=USB_HID_TOUCHSCREEN
#"Keyboard + Mouse + Joystick" -> build.usbtype=USB_HID
#"Serial + Keyboard + Mouse + Joystick	" -> build.usbtype=USB_SERIAL_HID
#"MIDI" -> build.usbtype=USB_MIDI
#"MIDIx4" -> build.usbtype=USB_MIDI4
#"MIDIx16" -> build.usbtype=USB_MIDI16
#"Serial + MIDI" -> build.usbtype=USB_MIDI_SERIAL
#"Serial + MIDIx4" -> build.usbtype=USB_MIDI4_SERIAL
#"Serial + MIDIx16" -> build.usbtype=USB_MIDI16_SERIAL
#"Audio" -> build.usbtype=USB_AUDIO
#"Serial + MIDI + Audio" -> build.usbtype=USB_MIDI_AUDIO_SERIAL
#"Serial + MIDIx16 + Audio" -> build.usbtype=USB_MIDI16_AUDIO_SERIAL
#"MTP Disk (Experimental)" -> build.usbtype=USB_MTPDISK
#"Serial + MTP Disk (Experimental)" -> build.usbtype=USB_MTPDISK_SERIAL
#"Raw HID" -> build.usbtype=USB_RAWHID
#"Flight Sim Controls" -> build.usbtype=USB_FLIGHTSIM
#"Flight Sim Controls + Joystick" -> build.usbtype=USB_FLIGHTSIM_JOYSTICK