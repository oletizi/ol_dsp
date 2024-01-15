# Project Name
TARGET = ex_pod_Encoder

# https://forum.electro-smith.com/t/out-of-flash-memory-walkthrough-with-samples/4370
# !!!!!!! IMPORTANT: Remove <iostream> or you'll run out of memory !!!!!!!!!!
#APP_TYPE = BOOT_SRAM
PROJECT_DIR = ../../..
OL_DSP_DIR   = ${PROJECT_DIR}/modules
DATTORRO_DIR = ${PROJECT_DIR}/libs/dattorro-verb
SOUNDPIPE_DIR = ${PROJECT_DIR}/libs/Soundpipe
# Includes

C_INCLUDES += -I${OL_DSP_DIR} -I${DATTORRO_DIR} -I${SOUNDPIPE_DIR}/h
# Sources
CPP_SOURCES += main.cpp \
	${OL_DSP_DIR}/corelib/ol_corelib.cpp \
	${OL_DSP_DIR}/fxlib/FxRack.cpp \
	${OL_DSP_DIR}/fxlib/Delay.cpp \
	${OL_DSP_DIR}/fxlib/Filter.cpp \
	${OL_DSP_DIR}/fxlib/Reverb.cpp \
	${OL_DSP_DIR}/fxlib/Saturator.cpp \
	${OL_DSP_DIR}/synthlib/Multivoice.cpp \
	${OL_DSP_DIR}/synthlib/Voice.cpp \
	${DATTORRO_DIR}/verb.cpp

#OPT = -O3
OPT = -Os

# Library Locations
LIBDAISY_DIR = ${PROJECT_DIR}/libs/libDaisy
DAISYSP_DIR = ${PROJECT_DIR}/libs/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

# Tried to include oopsy, but it wasn't trivial.
#C_INCLUDES += -I$(OOPSY_DIR)/source

include $(SYSTEM_FILES_DIR)/Makefile

tail:
	screen -c /dev/null /dev/tty.usbmodem3975367D33301

