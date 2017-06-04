/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */


#ifndef PPU_FILE_H
#define PPU_FILE_H

#include <stdint.h>
#include "memory.h"

namespace Sys{
	namespace File{
		void* loadFile(const char* filename, Memory::HeapBase& heap, uint32_t align=4);
	};
}; // namespace Sys
#endif // PPU_FILE_H
