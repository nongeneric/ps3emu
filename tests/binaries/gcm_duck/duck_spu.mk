#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2005 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  
#

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk

include $(CELL_MK_DIR)/sdk.makedef.mk

SPU_CFLAGS  += -mraw
SPU_LDFLAGS += -mraw

SPU_SRCS     = duck_spu.cpp
SPU_TARGET   = duck.spu.elf

include $(CELL_MK_DIR)/sdk.target.mk


