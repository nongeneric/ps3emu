############################################################
#  SCE CONFIDENTIAL
#  PlayStation(R)3 Programmer Tool Runtime Library 400.001
#  Copyright (C) 2006 Sony Computer Entertainment Inc.
#  All Rights Reserved.
############################################################

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

MK_TARGET = main.ppu.mk observer.spu.mk

include $(CELL_MK_DIR)/sdk.target.mk

# PPU_TO_SPU
GETLLAR_POLLING		= 2
SIGNAL_NOTIFICATION	= 4
# ifndef PPU_TO_SPU
# PPU_TO_SPU = GETLLAR_POLLING
# endif

# SPU_TO_PPU
EVENT_QUEUE_SEND	= 4
EVENT_QUEUE_THROW	= 5
DMA_PUT				= 6
ATOMIC_PUTLLUC		= 7
# ifndef SPU_TO_PPU
# SPU_TO_PPU = ATOMIC_PUTLLUC
# endif

# Local Variables:
# mode: Makefile
# tab-width:4
# End:
# vim:sw=4:sts=4:ts=4
