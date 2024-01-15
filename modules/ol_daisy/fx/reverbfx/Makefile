# Project Name
TARGET = ex_pod_Encoder

# https://forum.electro-smith.com/t/out-of-flash-memory-walkthrough-with-samples/4370

#APP_TYPE = BOOT_SRAM


C_INCLUDES += -I../../../ol_dsp/modules/

CPP_SOURCES += main.cpp

OPT = -O3

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

# Tried to include oopsy, but it wasn't trivial.
#C_INCLUDES += -I$(OOPSY_DIR)/source

include $(SYSTEM_FILES_DIR)/Makefile

tail:
	screen -c /dev/null /dev/tty.usbmodem3975367D33301

