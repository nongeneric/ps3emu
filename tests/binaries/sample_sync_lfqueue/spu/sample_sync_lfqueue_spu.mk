#########################################################
# SCE CONFIDENTIAL
# PlayStation(R)3 Programmer Tool Runtime Library 400.001
# Copyright (C) 2007 Sony Computer Entertainment Inc.
# All Rights Reserved.
#########################################################

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk

include $(CELL_MK_DIR)/sdk.makedef.mk


SPU_LDLIBS = -lsync -ldma

SPU_SRCS     = sample_sync_lfqueue_spu.c
SPU_TARGET   = sample_sync_lfqueue_spu.elf

include $(CELL_MK_DIR)/sdk.target.mk

