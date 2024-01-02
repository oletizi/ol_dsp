.PHONY: all build upload monitor

PORT := $(shell ../../find-port.sh)
LIB_SRC_DIR := $(shell cd ../../../ && pwd)
BUILD_DIR := $(shell cd ./build && pwd)
OL_DSP_TMP := ${BUILD_DIR}/ol_dsp
LIB_ZIP_FILE := ${BUILD_DIR}/ol_teensy.zip
LIB_OL_DSP_FILE := ${BUILD_DIR}/ol_dsp.zip

# Set CLEAN to --clean when changing build.usbtype build property
# --clean was needed to get midi to work right. Probably not needed for every build.
#CLEAN := --clean
CLEAN :=
all: build upload

config:
	arduino-cli config set library.enable_unsafe_install true

build-dir:
	if ! test -d ${BUILD_DIR}; then mkdir ${BUILD_DIR}; fi

lib: build-dir config
	echo "BUILD DIR: ${BUILD_DIR}"
	echo "LIB SRC DIR: ${LIB_SRC_DIR}"
	echo "LIB ZIP FILE: ${LIB_ZIP_FILE}"
	if test -d ${OL_DSP_TMP}; then rm -rf ${OL_DSP_TMP}/*; else mkdir -p ${OL_DSP_TMP}; fi
	if test -f ${LIB_OL_DSP_FILE}; then rm ${LIB_OL_DSP_FILE}; fi
	if test -f ${LIB_ZIP_FILE}; then rm ${LIB_ZIP_FILE}; fi
	rsync -av --exclude 'build' --exclude '.vscode' --exclude 'CMakeLists.txt' --exclude 'test' --exclude 'workouts' ${LIB_SRC_DIR}/. ${OL_DSP_TMP}/
	#cp library.properties ${OL_DSP_TMP}/  && cp ../../ol_dsp.h ${OL_DSP_TMP}/
	(cd ${BUILD_DIR} && zip -r ${LIB_OL_DSP_FILE} ./ol_dsp)
	(cd ${LIB_SRC_DIR} && zip -x "*workouts*" -r ${LIB_ZIP_FILE}  ./ol_teensy)
	#arduino-cli lib uninstall ol_teensy
	arduino-cli lib install --zip-path ${LIB_OL_DSP_FILE}
	arduino-cli lib install --zip-path ${LIB_ZIP_FILE}

build: lib
	arduino-cli compile $(CLEAN) --fqbn teensy:avr:teensy41 --build-property build.usbtype=USB_MIDI_SERIAL .

upload:
	arduino-cli upload -p $(PORT) --fqbn teensy:avr:teensy41 .

monitor:
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