/*  SCE CONFIDENTIAL
*  PlayStation(R)3 Programmer Tool Runtime Library 400.001
*  Copyright (C) 2010 Sony Computer Entertainment Inc.
*  All Rights Reserved.
*/

#include <cell/sysmodule.h>
#include <cell/cell_fs.h>
#include "file.h"
#include "memory.h"

namespace Sys{
namespace File{

void* loadFile(const char* filename, Sys::Memory::HeapBase& heap, uint32_t align){
	// Open file
	int fd;
	MY_C(cellFsOpen( filename, CELL_FS_O_RDONLY, &fd, NULL, 0 ));
	// Get file statistics
	CellFsStat stat;
	MY_C(cellFsFstat( fd, &stat ));
	void* buffer = heap.alloc(stat.st_size,align);
	// Read whole file
	MY_C(cellFsRead(fd, (void*)buffer, stat.st_size, NULL ));
	// Close File
	MY_C(cellFsClose( fd ));
	return buffer;
}

}; // namespace File
}; // namespace Sys