/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include "spu.h"
#include <cell/gcm.h>
#include <sys/spu_initialize.h>
#include <sys/raw_spu.h>
#include <sys/spu_utility.h>

#include "gcmutil_error.h"

#include "snaviutil.h"

using namespace cell::Gcm;

static SpuParam_t spuparam __attribute__ ((aligned(128)));
static sys_raw_spu_t id;
void setupSpuSharedBuffer(uint32_t const_addr, uint32_t read_addr, uint32_t write_addr)
{
	spuparam.fragment_constant_addr = const_addr;
	spuparam.spu_read_label_addr = read_addr;
	spuparam.spu_write_label_addr = write_addr;
}

void signalSpuFromPpu(void)
{
	sys_raw_spu_mmio_write(id, SPU_In_MBox, (uint32_t)&spuparam);
}

int initSpu(void )
{
	uint32_t entry;
	int ret;

	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);
	if (ret != CELL_OK) {
 		fprintf(stderr, "sys_spu_initialize failed: %d\n", ret);
		return ret;
	}
	if ((ret = sys_raw_spu_create(&id, NULL)) != CELL_OK) {
		fprintf(stderr, "Fail: sys_raw_spu_create failed (%#x).\n", ret);
		return ret;
	}

	if ((ret = sys_raw_spu_load(id, CELL_SNAVI_ADJUST_PATH(SPU_PROG), &entry)) != CELL_OK) {
		fprintf(stderr, "Fail: sys_raw_spu_load failed (%#x)\n", ret);
		sys_raw_spu_destroy(id);
		return ret;
	}

	sys_raw_spu_mmio_write(id, SPU_NPC, entry);
	sys_raw_spu_mmio_write(id, SPU_RunCntl, 0x1);
	EIEIO;

	return CELL_OK;
}

void signalSpuFromRsx(void)
{
	uint32_t inline_transfer_offset;
	CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset((void*)spuparam.spu_read_label_addr, &inline_transfer_offset));
	uint32_t inline_transfer_data[4] = {0xbeefbeef, 0xbeefbeef, 0xbeefbeef, 0xbeefbeef};
	cellGcmInlineTransfer(inline_transfer_offset, inline_transfer_data, 4, CELL_GCM_LOCATION_MAIN);
}

void waitSignalFromSpu(void)
{
	cellGcmSetWaitLabel(64, 0xabcdabcd);
}

void clearSignalFromSpu(void)
{
	cellGcmSetWriteCommandLabel(64, 0xdddddddd);
}
