#   SCE CONFIDENTIAL
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2006 Sony Computer Entertainment Inc.
#   All Rights Reserved.

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

include ../MakeDefs

# Uncomment the following lines to debug
# PPU_OPTIMIZE_LV=-O0

PPU_CWARNFLAGS ?= $(CWARNFLAGS_L2)
PPU_CXXWARNFLAGS ?= $(CXXWARNFLAGS_L2)

PPU_CXXFLAGS+=	-ffast-math

PPU_SRCS=	ppu_main.cpp

PPU_INCDIRS+=	-I$(CELL_TARGET_PATH)/ppu/include/vectormath/cpp
PPU_INCDIRS+=	-I$(PARTICLE_SAMPLES_COMMON)/include

PPU_LDLIBS+=	-L$(PPU_PSGL_LIBDIR) -lPSGL -lPSGLU -lgcm_cmd -lgcm_sys_stub -lsysutil_stub -lresc_stub -lsysmodule_stub

PPU_LIBS+=		$(PARTICLE_SAMPLES_COMMON)/lib/libparticle_common.a

PPU_TARGET=	01_particle_main.elf

EXTRA_TARGET = shaders.bin

SHADERS_BIN = $(CELL_DATA_DIR)/graphics/shaders.bin

CLEANFILES = $(EXTRA_TARGET)

shaders.bin:
	@if [ -f $(SHADERS_BIN) ]; then echo "Copying $(SHADERS_BIN)"; cp -f $(SHADERS_BIN) $@; else echo "WARNING: $(SHADERS_BIN) not found, some fixed-function GL features may not render correctly."; fi

SUBDIRS=	../particle_samples_common

include $(CELL_MK_DIR)/sdk.target.mk
