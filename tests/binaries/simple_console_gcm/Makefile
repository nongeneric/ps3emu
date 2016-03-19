#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2007 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_SDK ?= /usr/local/cell
CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS		=	main.cpp
PPU_TARGET		=	simple_console_gcm.ppu.elf

GCM_UTIL		=	$(CELL_COMMON_DIR)/gcmutil

SUBDIRS			=	$(GCM_UTIL) $(CELL_FW_DIR)

PPU_INCDIRS		+=	-I$(CELL_FW_DIR)/include -I$(CELL_FW_DIR)/include/gcm\
					-I$(GCM_UTIL)
PPU_INCDIRS		+=	-I$(CELL_TARGET_PATH)/ppu/include/vectormath/cpp
GCC_PPU_CXXFLAGS 	+=	-fno-rtti --param large-function-growth=800
PPU_LIBS		+=	$(CELL_FW_DIR)/libfwgcm.a \
					$(GCM_UTIL)/gcmutil.a
PPU_LDLIBS	 	+=	-ldbgfont_gcm -lgcm_cmd -lgcm_sys_stub -lsysmodule_stub -lusbd_stub -lio_stub -lm -lstdc++ -lsysutil_stub

include $(CELL_SAMPLE_DIR)/sdk/graphics/snavi/mk/graphics.makedef-snaviutil.mk
include $(CELL_MK_DIR)/sdk.target.mk

$(VPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.vpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@

$(FPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.fpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@
