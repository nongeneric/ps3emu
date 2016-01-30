#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2005 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk

include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_LDLIBS += -lsysutil_stub

PPU_SRCS     = raw_spu_printf.ppu.c
PPU_TARGET   = raw_spu_printf.ppu.elf

include $(CELL_MK_DIR)/sdk.target.mk


