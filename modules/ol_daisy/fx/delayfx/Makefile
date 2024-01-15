# Project Name
TARGET = sandbox2
# APP_TYPE = BOOT_SRAM

OL_DSP_DIR = ../../../../ol_dsp/modules

# Includes
OL_DSP_INCLUDES = -I${OL_DSP_DIR}

C_INCLUDES += ${OL_DSP_INCLUDES}

# Sources
CPP_SOURCES = main.cpp \
	${OL_DSP_DIR}/corelib/ol_corelib.cpp \
	${OL_DSP_DIR}/fxlib/Delay.cpp \
	${OL_DSP_DIR}/fxlin/DelayControlPanel.cpp

# Library Locations
LIBDAISY_DIR = ../../../../DaisyExamples/libDaisy/
DAISYSP_DIR = ../../../../DaisyExamples/DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
