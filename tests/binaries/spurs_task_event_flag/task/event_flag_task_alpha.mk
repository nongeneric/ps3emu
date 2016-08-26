#########################################################
# SCE CONFIDENTIAL
# PlayStation(R)3 Programmer Tool Runtime Library 360.001
# Copyright (C) 2007 Sony Computer Entertainment Inc.
# All Rights Reserved.
#########################################################

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

SPU_SRCS	= event_flag_task_alpha.cpp
SPU_TARGET	= event_flag_task_alpha.spu.elf
SPU_CFLAGS	+= -mspurs-task
SPU_LDLIBS	+= -ldma
SPU_LDFLAGS	+= -mspurs-task


include $(CELL_MK_DIR)/sdk.target.mk
