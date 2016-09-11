############################################################
#  SCE CONFIDENTIAL
#  PlayStation(R)3 Programmer Tool Runtime Library 400.001
#  Copyright (C) 2006 Sony Computer Entertainment Inc.
#  All Rights Reserved.
############################################################

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS		=	file.ppu.c \
					spu_thread.ppu.c \
					raw_spu.ppu.c \
					interrupt_thread.ppu.c \
					statistics.ppu.c

PPU_LIB_TARGET	=	libsample_round_trip.ppu.a

PPU_INCDIRS		+=	-I../

include $(CELL_MK_DIR)/sdk.target.mk

# Local Variables:
# mode: Makefile
# tab-width:4
# End:
# vim:sw=4:sts=4:ts=4
