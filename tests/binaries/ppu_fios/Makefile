#   SCE CONFIDENTIAL                                      
#   PlayStation(R)3 Programmer Tool Runtime Library 400.001
#   Copyright (C) 2008 Sony Computer Entertainment Inc.   
#   All Rights Reserved.                                  

CELL_MK_DIR	?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_INCDIRS	+= -I$(CELL_SDK)/target/ppu/include

PPU_LDLIBDIR	+= -L$(CELL_SDK)/target/ppu/lib

PPU_SRCS	= FiosSimple.cpp \
	GameData.cpp \
	SysCache.cpp \
	HDCache.cpp

PPU_LDLIBS	+= -lfios -lfs_stub -ledgezlib -lspurs_stub -lsync_stub -lio_stub -lsysmodule_stub -lsysutil_stub -lsysutil_game_stub -lsysutil_np_stub

PPU_TARGET	= FiosSimple.elf

include $(CELL_MK_DIR)/sdk.target.mk

