/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2009 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef __SAMPLE_SYNC_COMMAND_H__
#define __SAMPLE_SYNC_COMMAND_H__

#include <stdint.h>
#include <cell/spurs/job_descriptor.h>

namespace sample_sync_command {
	static const unsigned int	NUM_SYNC_ITERATION = 100;
	static const unsigned int	JOB_QUEUE_DEPTH = 10000;
	static const unsigned int	NUM_SYNC_JOBS = 32;
	static const unsigned int	NUM_TAGS = 2;
	static const unsigned int	NUM_TOTAL_JOBS = NUM_SYNC_JOBS * 2 * NUM_TAGS * NUM_SYNC_ITERATION;
	static const unsigned int	NUM_JQ_SPU = 4;
	static const unsigned int	NUM_MAX_GRAB = 4;

	static const unsigned int	REQUEST_TYPE_SIGNAL = 1;
	static const unsigned int	REQUEST_TYPE_EXIT = 2;

	typedef struct PpuRequest {
		uint32_t	type;
		uint32_t	idRequester;
		uint32_t	idRequest;
		uint8_t		__pad[16 - sizeof(uint32_t) * 3];
	} __attribute__((aligned(16))) PpuRequest;

	typedef struct WorkAreaEntry {
		uint32_t	count;
		uint32_t	idWriter;
		uint32_t	__pad__[2];
	} WorkAreaEntry;

};

struct SampleSyncJobInputList {
	CellSpursJobInputList	data;
};

struct SampleSyncJobDescriptor : public CellSpursJobHeader {
	/* 16 bytes aligned */
	SampleSyncJobInputList	input;
	uint64_t	myNumber;
	/* 16 bytes aligned */
	uint64_t	eaSuspendData;
	uint64_t	eaRequestQueue;
	uint64_t	eaOutputWorkArea;
	uint8_t		__pad__[256 
					- sizeof(CellSpursJobHeader)
					- sizeof(SampleSyncJobInputList)
					- sizeof(uint64_t) * 4
					];
} __attribute__((aligned(16)));

#endif /* __SAMPLE_SYNC_COMMAND_H__ */
