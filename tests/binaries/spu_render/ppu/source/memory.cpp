/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#include "memory.h"

namespace Sys{
// static member declar
Memory::HeapBase Memory::mainHeap;

Memory::VramHeap Memory::mappedMainHeap;
Memory::VramHeap Memory::localHeap;

Memory::HeapBase Memory::temporaryMain;

};// namespace Memory