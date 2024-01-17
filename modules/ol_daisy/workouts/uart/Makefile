# Project Name
TARGET = ex_pod_Encoder

# https://forum.electro-smith.com/t/out-of-flash-memory-walkthrough-with-samples/4370
# !!!!!!! IMPORTANT: Remove <iostream> or you'll run out of memory !!!!!!!!!!
PROJECT_DIR = ../../../..
OL_DSP_DIR   = ${PROJECT_DIR}/modules

# Includes
C_INCLUDES += -I${OL_DSP_DIR}

# Sources
CPP_SOURCES = main.cpp

# Library Locations
LIBDAISY_DIR = ${PROJECT_DIR}/libs/libDaisy
DAISYSP_DIR = ${PROJECT_DIR}/libs/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

include $(SYSTEM_FILES_DIR)/Makefile
