#########################################################
# SCE CONFIDENTIAL
# PlayStation(R)3 Programmer Tool Runtime Library 400.001
# Copyright (C) 2010 Sony Computer Entertainment Inc.
# All Rights Reserved.
#########################################################

CELL_MK_DIR ?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

SUBDIRS = task

SPU_ELF_TO_PPU_OBJ_FLAGS = --verbose
SPURS_TASKS	=\
	task/task.spu.elf\
	task/sub_task1.spu.elf\
	task/sub_task2.spu.elf\
	task/sub_task3.spu.elf
PPU_SRCS	= sample_start.cpp sample_create_on_task.cpp $(SPURS_TASKS)
PPU_TARGET	= sample_spurs_task_create_on_task.elf
PPU_LDLIBS	= -lspurs_stub -lsysmodule_stub

include $(CELL_MK_DIR)/sdk.target.mk
