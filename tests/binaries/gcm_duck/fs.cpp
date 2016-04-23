/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2009 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include "fs.h"
#include "memory.h"
#include <cell/sysmodule.h>
#include <cell/cell_fs.h>
#include <cell/gcm.h>
#include <assert.h>

int initFs(void)
{
    if( cellSysmoduleLoadModule(CELL_SYSMODULE_FS) < 0 )
		printf( "LoadModule failed...: 0x%x\n", CELL_SYSMODULE_FS );
	return CELL_OK;
}

int loadFile(uint32_t* gtf_addr, char* filename, uint32_t location) 
{
	uint64_t nread;
	CellFsErrno err;
	int fd;
	CellFsStat stat;

	// Open file
	err = cellFsOpen( filename, CELL_FS_O_RDONLY, &fd, NULL, 0 );
	if(err != CELL_FS_SUCCEEDED) { return -1; }

	// Get file statistics
	err = cellFsFstat( fd, &stat );
	if(err != CELL_FS_SUCCEEDED) { return -1; }

	assert( stat.st_size > 0 );

	switch (location) {
		case CELL_GCM_LOCATION_MAIN:
		*gtf_addr = (uint32_t)mainMemoryAlign(128, (uint32_t)stat.st_size);
		break;
		case CELL_GCM_LOCATION_LOCAL:
		*gtf_addr = (uint32_t)localMemoryAlign(128, (uint32_t)stat.st_size);
		break;
		case 2:
		*gtf_addr = (uint32_t)localMemoryAlign2(128, (uint32_t)stat.st_size);
		break;
		default:
		printf("unsupported location\n");
		break;
	}

	// Read whole texture
	err = cellFsRead( fd, (void*)*gtf_addr, stat.st_size, &nread );
	if(err != CELL_FS_SUCCEEDED) {
		return -1;
	}

	assert( nread == stat.st_size );

	// Close File
	err = cellFsClose( fd );
	if(err != CELL_FS_SUCCEEDED) {
		return -1;
	}

	return CELL_OK;
}


