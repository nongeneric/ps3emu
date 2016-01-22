#########################################################
# SCE CONFIDENTIAL
# PlayStation(R)3 Programmer Tool Runtime Library 400.001
# Copyright (C) 2007 Sony Computer Entertainment Inc.
# All Rights Reserved.
#########################################################

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS	= main_ppu.c getting_argp.spu.elf
PPU_TARGET	= sample_dma_getting_argp.elf

PPU_LIBSAMPLE_SPU_UTILSDIR	= ../../../../common/ppu/sample_spu_utils
include $(PPU_LIBSAMPLE_SPU_UTILSDIR)/libsample_spu_utils_ppu.mk

include $(CELL_MK_DIR)/sdk.target.mk

# Local Variables:
# mode: Makefile
# End:
