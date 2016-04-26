#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2006 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_SDK ?= /usr/local/cell
CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS		=	SampleApp.cpp $(VPSHADER_PPU_OBJS) $(FPSHADER_PPU_OBJS)
PPU_TARGET		=	fw_render_to_texture.ppu.elf

GCM_UTIL		=	$(CELL_COMMON_DIR)/gcmutil

SUBDIRS			=	$(CELL_FW_DIR) $(GCM_UTIL)

PPU_INCDIRS		+=	-I$(GCM_UTIL) -I$(CELL_FW_DIR)/include -I$(CELL_FW_DIR)/include/gcm
PPU_INCDIRS		+=	-I$(CELL_TARGET_PATH)/ppu/include/vectormath/cpp
GCC_PPU_CXXFLAGS 	+=	-fno-exceptions -fno-rtti --param large-function-growth=800
PPU_LIBS		+=	$(CELL_FW_DIR)/libfwgcm.a \
					$(GCM_UTIL)/gcmutil.a \
                    $(CELL_TARGET_PATH)/ppu/lib/libgcm_cmd.a \
					$(CELL_TARGET_PATH)/ppu/lib/libgcm_sys_stub.a
PPU_LDLIBS	 	+=	-lsysmodule_stub -lusbd_stub -lio_stub -lm -lstdc++ -lsysutil_stub
PPU_LDFLAGS		+=	-Wl,--strip-unused

VPSHADER_SRCS	=	vpshader.cg rtt_vpshader.cg
FPSHADER_SRCS	=	fpshader.cg rtt_fpshader.cg

VPSHADER_PPU_OBJS = $(patsubst %.cg, $(OBJS_DIR)/%.ppu.o, $(VPSHADER_SRCS))
FPSHADER_PPU_OBJS = $(patsubst %.cg, $(OBJS_DIR)/%.ppu.o, $(FPSHADER_SRCS))

include $(CELL_SAMPLE_DIR)/sdk/graphics/snavi/mk/graphics.makedef-snaviutil.mk
include $(CELL_MK_DIR)/sdk.target.mk

PPU_OBJS += $(VPSHADER_PPU_OBJS) $(FPSHADER_PPU_OBJS)

VPSHADER_FLAGS += -fastmath -fastprecision
FPSHADER_FLAGS += -fastmath -fastprecision

$(VPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.vpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@

$(FPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.fpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@

clean-local: ppu-clean ppu-prx-clean spu-clean cg-clean $(MK_TARGET_CLEAN) user-clean clean-objs