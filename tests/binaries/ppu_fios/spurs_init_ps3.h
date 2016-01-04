/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2008 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/
/**
	\file spurs_init_ps3.h
	SPURS initialization for sample code.
*/
#ifndef __FIOS_SPURS_INIT_PS3_H__
#define __FIOS_SPURS_INIT_PS3_H__

#include <assert.h>
#include <stdio.h>

#include <sys/spu_initialize.h>
#include <sys/event.h>
#include <sys/spu_thread.h>
#include <sys/ppu_thread.h>
#include <spu_printf.h>

#include <cell/spurs/lv2_event_queue.h>
#include <cell/fios.h>
#include <cell/spurs.h>

class SpursInitializer
{
public:
	SpursInitializer()
	{
		// Initialize SPURs.
		printf("Initializing SPURS\n");
		sys_spu_initialize( 6, 0 );
		int numSPUs = 1;
		int err = cellSpursInitialize(&mSpurs_, numSPUs, 150, my_thread_priority() - 1, false);
		assert(err == CELL_OK);
		if (err != CELL_OK) printf("cellSpursInitialize failed: %08X\n", (int)err);
	}

	~SpursInitializer()
	{
		// Finalize SPURS
		printf("Finalizing SPURS\n");
		int err = cellSpursFinalize(&mSpurs_);
		assert(err == CELL_OK);
		if (err != CELL_OK) printf("cellSpursFinalize failed: %08X\n", (int)err);
	}

        CellSpurs* getSpursInstance() {
		return &mSpurs_;
        }

private:
	int my_thread_priority()
	{
		sys_ppu_thread_t my_thread_id;
		int prio;
		if (sys_ppu_thread_get_id(&my_thread_id) == CELL_OK &&
			sys_ppu_thread_get_priority(my_thread_id, &prio) == CELL_OK)
			return prio;
		else
			return 1000;
	}

private:
	CellSpurs mSpurs_;
};

#endif // __FIOS_SPURS_INIT_PS3_H__
