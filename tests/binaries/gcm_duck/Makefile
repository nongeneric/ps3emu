#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2006 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  
#

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

MK_TARGET = duck.mk duck_spu.mk

include $(CELL_MK_DIR)/sdk.target.mk

clean-local: ppu-clean ppu-prx-clean spu-clean cg-clean $(MK_TARGET_CLEAN) user-clean clean-objs