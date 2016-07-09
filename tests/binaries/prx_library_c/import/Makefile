#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2007 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_SDK ?= /usr/local/cell
CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
PRX_SAMPLES_DIR = ../..
include $(CELL_MK_DIR)/sdk.makedef.mk
include $(PRX_SAMPLES_DIR)/mk/prx.mk

PPU_SRCS = import.c
PPU_PRX_TARGET = library-c-import.prx
PPU_PRX_LDFLAGS += $(PRX_LDFLAGS_EXTRA)

PPU_LIBS = $(LIBSTUB_DIR)/library-c-export_stub.a

CLEANFILES = $(PRX_DIR)/$(PPU_SPRX_TARGET)

stub:

include $(CELL_MK_DIR)/sdk.target.mk
include $(PRX_SAMPLES_DIR)/mk/prx_target.mk



