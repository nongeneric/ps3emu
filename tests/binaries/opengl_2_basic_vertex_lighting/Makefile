#!/usr/bin/make
# SCE CONFIDENTIAL
# PlayStation(R)3 Programmer Tool Runtime Library 400.001
#                 Copyright (C) 2009 Sony Computer Entertainment Inc.
#                                                All Rights Reserved.

include ../../MakeDefs

GFX_COMMON=../../Common

PPU_INCDIRS		+= -Iinclude 
PPU_CPPFLAGS		+= -DPSGL
PPU_CXXSTDFLAGS		+= -fno-exceptions
PPU_SRCS		:= main.cpp \
			$(GFX_COMMON)/gfxLight.cpp \
			$(GFX_COMMON)/gfxObject.cpp \
			$(GFX_COMMON)/gfxCommon.cpp \
			$(GFX_COMMON)/gfxPad.cpp
			
PPU_TARGET		:= gfxSample.elf 
PPU_LDLIBS		:= $(GRAPHICS_SAMPLE_CORE_LIBS)

VPSHADER_SOURCES        := $(CELL_GRAPHICS_DATA_DIR)/shaders/Tutorial/vs_basicVertex.cg
FPSHADER_SOURCES        := $(CELL_GRAPHICS_DATA_DIR)/shaders/Tutorial/fs_basicVertex.cg

include ../../MakeRules
