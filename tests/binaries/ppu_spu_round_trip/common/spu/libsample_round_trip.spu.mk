############################################################
#  SCE CONFIDENTIAL
#  PlayStation(R)3 Programmer Tool Runtime Library 400.001
#  Copyright (C) 2006 Sony Computer Entertainment Inc.
#  All Rights Reserved.
############################################################

SPU_LIBSAMPLE_ROUND_TRIP_DIR ?= \
		$(CELL_SDK)/samples/tutorial/SPE_MFC_Tutorial/ppu_spu_round_trip/common/spu
SPU_LIBSAMPLE_ROUND_TRIP = \
		$(SPU_LIBSAMPLE_ROUND_TRIP_DIR)/libsample_round_trip.spu.a
SPU_INCDIRS		+=	-I$(SPU_LIBSAMPLE_ROUND_TRIP_DIR)
SPU_LOADLIBS	+=	$(SPU_LIBSAMPLE_ROUND_TRIP)

$(SPU_TARGET): $(SPU_LIBSAMPLE_ROUND_TRIP)

$(SPU_LIBSAMPLE_ROUND_TRIP):
	$(MAKE) -C $(SPU_LIBSAMPLE_ROUND_TRIP_DIR)

# Local Variables:
# mode: Makefile
# tab-width:4
# End:
# vim:sw=4:sts=4:ts=4
