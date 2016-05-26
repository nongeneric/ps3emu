#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2008 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_SDK ?= /usr/local/cell
CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

SAMPLE_NAME = strip_branch

GCM_UTIL = $(CELL_COMMON_DIR)/gcmutil
PAD_UTIL  =   $(CELL_COMMON_DIR)/padutil
GTFDIR	=	$(CELL_COMMON_DIR)/gtf
SUBDIRS = $(GCM_UTIL) $(PAD_UTIL) $(GTFDIR)

PPU_SRCS =	$(wildcard *.cpp)
PPU_TARGET		= $(SAMPLE_NAME).ppu.elf

PPU_INCDIRS += -I$(GCM_UTIL) -I$(PAD_UTIL) -I$(GTFDIR)
PPU_LIBS += $(GCM_UTIL)/gcmutil.a $(PAD_UTIL)/padutil.a $(GTFDIR)/cellGtfLoader.a
PPU_LDLIBS += -lgcm_cmd -lgcm_sys_stub -lsysutil_stub -lsysmodule_stub -lfs_stub -lio_stub -lpadfilter -ldbgfont_gcm -lcgb
PPU_LDFLAGS	+=	-Wl,--strip-unused

GCC_PPU_CXXFLAGS	+= -fno-exceptions -fno-rtti --param large-function-growth=800

VPSHADER_SRCS	=	$(wildcard vs_*.cg)
FPSHADER_SRCS	=	$(wildcard fs_*.cg)

include $(CELL_SAMPLE_DIR)/sdk/graphics/snavi/mk/graphics.makedef-snaviutil.mk
include $(CELL_MK_DIR)/sdk.target.mk

VPSHADER_FLAGS += -fastmath -fastprecision
FPSHADER_FLAGS += -fastmath -fastprecision

clean-local: ppu-clean ppu-prx-clean spu-clean cg-clean $(MK_TARGET_CLEAN) user-clean clean-objs