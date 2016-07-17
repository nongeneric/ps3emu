#########################################################
# SCE CONFIDENTIAL
# PlayStation(R)3 Programmer Tool Runtime Library 400.001
# Copyright (C) 2007 Sony Computer Entertainment Inc.
# All Rights Reserved.
#########################################################


CELL_MK_DIR ?= $(CELL_SDK)/samples/mk

include $(CELL_MK_DIR)/sdk.makedef.mk


LIBSAMPLE_SPU_UTILS_ROOT ?= ../../../../common/ppu/sample_spu_utils
PPU_LIBSAMPLE_SPU_UTILS	= $(LIBSAMPLE_SPU_UTILS_ROOT)/libsample_spu_utils.a
PPU_INCDIRS	+= -I$(LIBSAMPLE_SPU_UTILS_ROOT)
$(PPU_TARGET): $(PPU_LIBSAMPLE_SPU_UTILS)

PPU_LDLIBS = -lsync_stub $(PPU_LIBSAMPLE_SPU_UTILS)  -lsysmodule_stub

PPU_SRCS     = sample_sync_mutex_ppu.c spu/sample_sync_mutex_spu.elf
PPU_TARGET   = sample_sync_mutex_ppu.elf



include $(CELL_MK_DIR)/sdk.target.mk

