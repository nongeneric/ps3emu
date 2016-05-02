#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2007 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_SDK ?= /usr/local/cell
CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS 	= main.ppu.c
PPU_TARGET 	= mself-main.ppu.elf
PPU_LDLIBS	+= -lfs_stub -lsysmodule_stub

include $(CELL_MK_DIR)/sdk.target.mk


