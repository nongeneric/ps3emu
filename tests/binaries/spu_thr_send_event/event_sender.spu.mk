#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2005 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  
#

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk

include $(CELL_MK_DIR)/sdk.makedef.mk

SPU_SRCS     = event_sender.spu.c
SPU_TARGET   = event_sender.spu.elf

include $(CELL_MK_DIR)/sdk.target.mk


