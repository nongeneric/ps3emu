#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2007 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_SDK ?= /usr/local/cell
CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
PRX_SAMPLES_DIR = ../..
include $(CELL_MK_DIR)/sdk.makedef.mk
include $(PRX_SAMPLES_DIR)/mk/prx.mk

PPU_SRCS = export.c
PPU_PRX_TARGET = call-prx-export.prx
PPU_PRX_LDFLAGS += $(PRX_LDFLAGS_EXTRA)

LIBSTUB	= call-prx-export_stub.a
ARCHIVE	= cellPrxCallPrxExportForUser_stub.a

CLEANFILES = $(ARCHIVE) $(LIBSTUB_DIR)/$(LIBSTUB) $(PRX_DIR)/$(PPU_SPRX_TARGET)

stub:	$(PPU_DEPENDS) $(PPU_OBJS_DEPENDS)
	$(MAKE) $(PPU_OBJS_DEPENDS)
	$(PPU_PRX_CXXLD) $(PPU_OBJS) -zgenstub
	$(CP) $(ARCHIVE) $(LIBSTUB_DIR)/$(LIBSTUB)

include $(CELL_MK_DIR)/sdk.target.mk
include $(PRX_SAMPLES_DIR)/mk/prx_target.mk



