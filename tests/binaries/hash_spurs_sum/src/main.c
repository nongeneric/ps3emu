/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 475.001
* Copyright (C) 2006-2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

/*E standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*E Lv2 OS headers */
#include <sys/spu_initialize.h>
#include <sys/spu_thread.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>

#include <cell/sysmodule.h>
/*E SPURS */
#include <cell/spurs.h>
#include <cell/cell_fs.h>

#include <sys/process.h>

#include "md5_sum_spurs.h"
#include "sha1_sum_spurs.h"
#include "sha224_sum_spurs.h"
#include "sha256_sum_spurs.h"
#include "sha384_sum_spurs.h"
#include "sha512_sum_spurs.h"
#include "spurs_hash_utils.h"

static const char g_szUsage[] = "usage: sum.self mount_point file_name\n";
CellSpursTaskset2 *taskset;


int main(int argc, const char *const argv[])
{
	int ret;


	printf("SPU Sum samples\n");
	printf("-----------------------------------------------------------------------\n");

	if (argc < 3) {
		printf(g_szUsage);
		return (0);
	}

	//E initialise hash library SPURS environment
	ret = hashlib_spurs_init(&taskset);
	if (ret < 0) {
		printf("unable to initialise SPURS\n");
	}

	printf("SPU Sum MD5:\n");
	ret = md5_sum_spu(argv[1], argv[2], taskset);
	if ( ret < 0) {
		printf("sample failed\n");
	}

	printf("-----------------------------------------------------------------------\n");
	printf("SPU Sum SHA-1:\n");
	ret = sha1_sum_spu(argv[1],argv[2], taskset);
	if ( ret < 0) {
		printf("sample failed\n");
	}

	printf("-----------------------------------------------------------------------\n");
	printf("SPU Sum SHA-224:\n");
	ret = sha224_sum_spu(argv[1], argv[2], taskset);
	if ( ret < 0) {
		printf("sample failed\n");
	}

	printf("-----------------------------------------------------------------------\n");
	printf("SPU Sum SHA-256:\n");
	ret = sha256_sum_spu(argv[1], argv[2], taskset);
	if ( ret < 0) {
		printf("sample failed\n");
	}

	printf("-----------------------------------------------------------------------\n");
	printf("SPU Sum SHA-384:\n");
	ret = sha384_sum_spu(argv[1], argv[2], taskset);
	if ( ret < 0) {
		printf("sample failed\n");
	}

	printf("-----------------------------------------------------------------------\n");
	printf("SPU Sum SHA-512:\n");
	ret = sha512_sum_spu(argv[1], argv[2], taskset);
	if ( ret < 0) {
		printf("sample failed\n");
	}

	ret = hashlib_spurs_terminate();
}
