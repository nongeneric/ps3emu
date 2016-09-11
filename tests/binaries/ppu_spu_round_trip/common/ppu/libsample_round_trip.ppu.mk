############################################################
#  SCE CONFIDENTIAL
#  PlayStation(R)3 Programmer Tool Runtime Library 400.001
#  Copyright (C) 2006 Sony Computer Entertainment Inc.
#  All Rights Reserved.
############################################################

PPU_LIBSAMPLE_ROUND_TRIP_DIR ?= \
		$(CELL_SDK)/samples/tutorial/SPE_MFC_Tutorial/ppu_spu_round_trip/common/ppu
PPU_LIBSAMPLE_ROUND_TRIP = \
		$(PPU_LIBSAMPLE_ROUND_TRIP_DIR)/libsample_round_trip.ppu.a
PPU_INCDIRS		+=	-I$(PPU_LIBSAMPLE_ROUND_TRIP_DIR)
PPU_LOADLIBS	+=	$(PPU_LIBSAMPLE_ROUND_TRIP)

$(PPU_TARGET): $(PPU_LIBSAMPLE_ROUND_TRIP)

$(PPU_LIBSAMPLE_ROUND_TRIP):
	$(MAKE) -C $(PPU_LIBSAMPLE_ROUND_TRIP_DIR)

# Local Variables:
# mode: Makefile
# tab-width:4
# End:
# vim:sw=4:sts=4:ts=4
