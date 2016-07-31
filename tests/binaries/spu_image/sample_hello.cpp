/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
* Copyright (C) 2010 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cell/spurs.h>
#include <sys/spu_image.h>

#include "sample_config.h"

/* embedded SPU ELF symbols */
extern const CellSpursTaskBinInfo _binary_task_task_hello_spu_elf_taskbininfo;

int sample_main()
{
	int ret;

	sys_spu_image_t spu_img;
	ret = sys_spu_image_import(&spu_img, (void*)_binary_task_task_hello_spu_elf_taskbininfo.eaElf, SYS_SPU_IMAGE_DIRECT);
	if (ret != CELL_OK) {
		printf("sys_spu_image_open failed %x\n", ret);
		exit(1);
	}

	printf("type: %08x\n", spu_img.type);
	printf("entry point: %08x\n", spu_img.entry_point);
	//printf("segs: %08x\n", (uint32_t)spu_img.segs);
	printf("nsegs: %08x\n", spu_img.nsegs);

	int a = sizeof(sys_spu_segment_t);

	for (int i = 0; i < spu_img.nsegs; ++i) {
		printf("type: %08x\n", spu_img.segs[i].type);
		printf("ls start: %08x\n", spu_img.segs[i].ls_start);
		printf("size: %08x\n", spu_img.segs[i].size);
		printf("pa_start/value: %08x\n", spu_img.segs[i].src.pa_start);
	}

	ret = sys_spu_image_close(&spu_img);
	if (ret != CELL_OK) {
		printf("sys_spu_image_close failed %x\n", ret);
		exit(ret);
	}

	return 0;
}
