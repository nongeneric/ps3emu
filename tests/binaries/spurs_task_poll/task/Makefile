#########################################################
# SCE CONFIDENTIAL
# PlayStation(R)3 Programmer Tool Runtime Library 400.001
# Copyright (C) 2010 Sony Computer Entertainment Inc.
# All Rights Reserved.
#########################################################

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

SPU_SRCS	= poll.cpp
SPU_TARGET	= task_poll.spu.elf
SPU_CFLAGS	= -mspurs-task
SPU_LDFLAGS	= -mspurs-task
SPU_LDLIBS	= -ldma

include $(CELL_MK_DIR)/sdk.target.mk
