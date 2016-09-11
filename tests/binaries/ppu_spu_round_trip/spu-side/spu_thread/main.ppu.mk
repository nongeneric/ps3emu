############################################################
#  SCE CONFIDENTIAL
#  PlayStation(R)3 Programmer Tool Runtime Library 400.001
#  Copyright (C) 2006 Sony Computer Entertainment Inc.
#  All Rights Reserved.
############################################################

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS		=	responder.ppu.c \
					main.ppu.c
PPU_TARGET		=	main.ppu.elf

PPU_INCDIRS		+=	-I../../common
PPU_LDLIBS		=	-lfs_stub -lsysmodule_stub

# begin	libsample_round_trip.ppu
PPU_LIBSAMPLE_ROUND_TRIP_DIR = ../../common/ppu
include $(PPU_LIBSAMPLE_ROUND_TRIP_DIR)/libsample_round_trip.ppu.mk
# end	libsample_round_trip.ppu

# begin	PPU_TO_SPU
GETLLAR_POLLING		= 2
SIGNAL_NOTIFICATION	= 4
ifndef PPU_TO_SPU
PPU_TO_SPU = GETLLAR_POLLING
endif
PPU_CFLAGS += -DPPU_TO_SPU=$(PPU_TO_SPU)
# end	PPU_TO_SPU

# begin	SPU_TO_PPU
EVENT_QUEUE_SEND	= 4
EVENT_QUEUE_THROW	= 5
DMA_PUT				= 6
ATOMIC_PUTLLUC		= 7
ifndef SPU_TO_PPU
SPU_TO_PPU = ATOMIC_PUTLLUC
endif
PPU_CFLAGS += -DSPU_TO_PPU=$(SPU_TO_PPU)
# end	SPU_TO_PPU

include $(CELL_MK_DIR)/sdk.target.mk

# Local Variables:
# mode: Makefile
# tab-width:4
# End:
# vim:sw=4:sts=4:ts=4
