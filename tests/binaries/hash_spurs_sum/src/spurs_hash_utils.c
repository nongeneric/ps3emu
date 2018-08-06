/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2006-2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

/*E Standard C Libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*E Lv2 OS headers */
#include <sys/spu_initialize.h>
#include <sys/spu_thread.h>
#include <sys/ppu_thread.h>

#include <cell/sysmodule.h>
/*E SPURS */
#include <cell/spurs.h>

#define NUM_OF_SPU 6
#define SAMPLE_SPURS_THREAD_GROUP_PRIORITY 250


CellSpurs *spurs=NULL;
CellSpursTaskset2 *taskset=NULL;
#define	SPURS_SPU_NUM 5
#define	SPU_THREAD_GROUP_PRIORITY 250
#include <spu_printf.h>

//E prototypes:
int hashlib_spurs_terminate(void);
int hashlib_spurs_init(CellSpursTaskset2 **_taskset);


static int sampleSpursUtilGetSecondaryPpuThreadPriority(int *prio);

static int sampleSpursUtilGetSecondaryPpuThreadPriority(int *prio)
{
        int ret;
        sys_ppu_thread_t my_ppu_thread_id;
        ret = sys_ppu_thread_get_id(&my_ppu_thread_id);
        if(ret){
                return ret;
        }
        ret = sys_ppu_thread_get_priority(my_ppu_thread_id, prio);
        if(ret){
                return ret;
        }
        *prio = *prio - 1;
        return 0;
}




int hashlib_spurs_init(CellSpursTaskset2 **_taskset)
{
	int ret;
	CellSpursAttribute _spursAttributes;
	CellSpursTasksetAttribute2 _taskSetAttributes;
	int ppu_thr_prio;

	spurs = (CellSpurs*)memalign(CELL_SPURS_ALIGN, sizeof(CellSpurs));
	*_taskset = taskset = memalign(CELL_SPURS_TASKSET_ALIGN,sizeof(CellSpursTaskset2));
	
	ret = cellSysmoduleInitialize();
	if(ret != CELL_OK){
		printf("cellSysmoduleInitialize() failed : %x\n", ret);
		return ret;
	}
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_SPURS);
	if(ret != CELL_OK){
		printf("cellSysmoduleLoadModule() failed : %x\n", ret);
		return ret;
	}
	ret = sys_spu_initialize (NUM_OF_SPU, 0);

	if (ret != 0) {
		printf("System initialization failed.\n");
		printf("## libspurs :FAILED ##\n");
		return ret;
	}


	ret = sampleSpursUtilGetSecondaryPpuThreadPriority(&ppu_thr_prio);
	if (ret) {
		printf("get PPU thread priority failed : %d\n", ret);
		return ret;
	}

	ret = cellSpursAttributeInitialize(&_spursAttributes,
										SPURS_SPU_NUM,
										SAMPLE_SPURS_THREAD_GROUP_PRIORITY,
										ppu_thr_prio,
										true);

	if(ret != CELL_OK){
		printf("SPURS ,fail to initialize SPURS attributes :%x.\n",ret);
		return ret;
	}

	ret = spu_printf_initialize(999, NULL);
	if(ret != CELL_OK){
		printf("Initialization of spu printf sever failed : %x.\n",ret);
		return ret;
	}


	ret = cellSpursAttributeEnableSpuPrintfIfAvailable(&_spursAttributes);
	if(ret != CELL_OK){
		printf("SPURS, failed to enable spu printf : %x.\n",ret);
	}

	ret = cellSpursInitializeWithAttribute(spurs,&_spursAttributes);
	if(ret != CELL_OK){
		printf("SPURS failed to initialize: %x.\n",ret);
	}


	cellSpursTasksetAttribute2Initialize(&_taskSetAttributes);
	_taskSetAttributes.name = "hash_utils";
	_taskSetAttributes.maxContention = SPURS_SPU_NUM;
	uint8_t prios[8] = {1, 1, 1, 1, 1, 1, 1, 1};
	memcpy(_taskSetAttributes.priority, prios,sizeof(prios));
	cellSpursCreateTaskset2(spurs,*_taskset,&_taskSetAttributes);
	if(ret != CELL_OK){
		printf("Taskset2 failed to create : %x.\n",ret);

		return ret;
	}
	return CELL_OK;
}

int hashlib_spurs_terminate(void)
{
	int ret;
	cellSpursDestroyTaskset2(taskset);
	printf ("PPU: wait for taskset shutdown...\n");



	ret = cellSpursFinalize (spurs);
	if (ret) {
		printf("cellSpursFinalize failed : %d\n", ret);
		return ret;
	}

	if(ret != 0){
		printf("SPURS cleanup failed.\n");
		printf("## libspurs : FAILED ##\n");
		return ret;
	}

	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_SPURS);
	if(ret != CELL_OK){
		printf("cellSysmoduleUnloadModule() failed : %x\n", ret);
		return ret;
	}

	ret = cellSysmoduleFinalize();
	if(ret != CELL_OK){
		printf("cellSysmoduleFinalize() failed : %x\n", ret);
		return ret;
	}

	free (spurs);
	free (taskset);
	printf("PPU: finished.\n");
	return CELL_OK;
}
