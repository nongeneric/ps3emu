#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2008 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_SDK ?= /usr/local/cell
CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

CGCDISASM       =	$(CELL_HOST_PATH)/Cg/bin/sce-cgcdisasm
CGCSTRIP        =	$(CELL_HOST_PATH)/Cg/bin/sce-cgcstrip

PAD_UTIL    =  $(CELL_COMMON_DIR)/padutil
SUBDIRS     += $(PAD_UTIL)
PPU_INCDIRS += -I$(PAD_UTIL) -I$(CELL_SDK)/samples/common/gcmutil
PPU_LIBS    += $(PAD_UTIL)/padutil.a

VPSHADER_HEADER =	vpshader_params.h
FPSHADER_HEADER =	fpshader_params.h

PPU_SRCS = $(CELL_COMMON_DIR)/gcmutil/EntryPoint.c $(VPSHADER_HEADER) $(FPSHADER_HEADER) duck.cpp fs.cpp gtf.cpp geometry.cpp disp.cpp shader.cpp texture.cpp spu.cpp memory.cpp $(VPSHADER_PPU_OBJS) $(FPSHADER_PPU_OBJS)
PPU_TARGET = duck.ppu.elf

# Remote texture file directory
GCC_PPU_CPPFLAGS += -DGCM_SAMPLE_DATA_PATH="SYS_HOST_ROOT \"/\"CELL_DATA_DIR \"/graphics/gcm\""
GCC_PPU_CPPFLAGS += -DGCM_SAMPLE_HOME_PATH="SYS_HOST_ROOT \"/\" CELL_SAMPLE_DIR \"/sdk/graphics/gcm\""
GCC_PPU_CPPFLAGS += -DCELL_SAMPLE_DIR=\"$(CELL_SAMPLE_DIR)\"

SNC_PPU_CPPFLAGS += -DGCM_SAMPLE_DATA_PATH="SYS_HOST_ROOT \"/\"CELL_DATA_DIR \"/graphics/gcm\""
SNC_PPU_CPPFLAGS += -DGCM_SAMPLE_HOME_PATH="SYS_HOST_ROOT \"/\" CELL_SAMPLE_DIR \"/sdk/graphics/gcm\""
SNC_CELL_SAMPLE_DIR = $(shell echo $(CELL_SAMPLE_DIR) | sed s/"^\/.\/"/"\/"/)
SNC_PPU_CPPFLAGS += -DCELL_SAMPLE_DIR=\"$(SNC_CELL_SAMPLE_DIR)\"

GCC_PPU_CXXFLAGS += -fno-exceptions -fno-rtti
PPU_LIBS	+= $(CELL_TARGET_PATH)/ppu/lib/libgcm_cmd.a
PPU_LIBS	+= $(CELL_TARGET_PATH)/ppu/lib/libgcm_sys_stub.a
PPU_LDLIBS 	+= -lfs_stub -lio_stub -lsysmodule_stub -lm -lsysutil_stub -lpadfilter
PPU_LDFLAGS	+=	-Wl,--strip-unused

VPSHADER_SRCS = vpshader.cg
FPSHADER_SRCS = fpshader.cg

VPSHADER_OBJS =	vpshader.vpo
FPSHADER_OBJS =	fpshader.fpo

VPSHADER_PPU_OBJS = $(patsubst %.cg, $(OBJS_DIR)/%.ppu.o, $(VPSHADER_SRCS))
FPSHADER_PPU_OBJS = $(patsubst %.cg, $(OBJS_DIR)/%.ppu.o, $(FPSHADER_SRCS))

include $(CELL_SAMPLE_DIR)/sdk/graphics/snavi/mk/graphics.makedef-snaviutil.mk
include $(CELL_MK_DIR)/sdk.target.mk

PPU_OBJS += $(VPSHADER_PPU_OBJS) $(FPSHADER_PPU_OBJS)
PPU_CLEAN_OBJS += $(VPSHADER_HEADER) $(FPSHADER_HEADER)

duck.cpp : $(VPSHADER_HEADER) $(FPSHADER_HEADER)

$(VPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.vpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@

$(FPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.fpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@

$(VPSHADER_HEADER) : $(VPSHADER_OBJS)
	$(CGCSTRIP) -param $<
	$(CGCDISASM) -header $<

$(FPSHADER_HEADER) : $(FPSHADER_OBJS)
	$(CGCSTRIP) -param $<
	$(CGCDISASM) -header $<

clean-local: ppu-clean ppu-prx-clean spu-clean cg-clean user-clean clean-objs
