#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2007 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_SDK ?= /usr/local/cell
CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
PRX_SAMPLES_DIR = ../..
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS 	= main.c
PPU_TARGET 	= simple-c-main.elf
CLEANFILES 	= $(PRX_SAMPLES_DIR)/$(PPU_TARGET)
CLEANFILES	+= $(PRX_SAMPLES_DIR)/$(PPU_SELF_TARGET)

stub:

install	:
	$(CP) $(PPU_TARGET) $(PRX_SAMPLES_DIR)
	$(CP) $(PPU_SELF_TARGET) $(PRX_SAMPLES_DIR)

include $(CELL_MK_DIR)/sdk.target.mk



