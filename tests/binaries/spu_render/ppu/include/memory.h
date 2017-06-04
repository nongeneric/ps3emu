/*  SCE CONFIDENTIAL
*  PlayStation(R)3 Programmer Tool Runtime Library 400.001
*  Copyright (C) 2011 Sony Computer Entertainment Inc.
*  All Rights Reserved.
*/

#ifndef PPU_MEMORY_H
#define PPU_MEMORY_H

#include <cell/gcm.h>
#include "debug.h"

namespace Sys{
	class Memory{
	public:
		class HeapBase{
		public:
			HeapBase(){}
			inline uintptr_t getBegin(){ return begin; }
			inline uintptr_t getCurrent(){ return current; }
			inline uintptr_t getEnd(){ return end; }
			inline size_t getSize(){ return size; }
			inline size_t getRest(){ return end - current;}
			template<typename Data> inline Data* alloc(uint32_t align = __alignof__(Data)){
				uintptr_t ret_addr = alloc_internal(sizeof(Data), align);
				return reinterpret_cast<Data*>(ret_addr);
			}
			template<typename Data> inline Data* calloc(uint32_t num_of_Data, uint32_t align = __alignof__(Data)){
				uintptr_t ret_addr = alloc_internal(sizeof(Data)*num_of_Data, align);
				return reinterpret_cast<Data*>(ret_addr);
			}
			inline void* alloc(uint32_t size, uint32_t alignment=4){
				uintptr_t ret_addr = alloc_internal(size,alignment);
				return reinterpret_cast<void*>(ret_addr);
			}
			inline void init(void* addr, size_t size_in_byte){
				begin = reinterpret_cast<uintptr_t>(addr);
				size = size_in_byte;
				reset();
			}
			inline void reset(){
				current = begin;
				end = begin + size;
			}
		private:
			inline uintptr_t alloc_internal(size_t size, uint32_t alignment){
				uintptr_t ret = align(alignment);
				current += size;
				MY_ASSERT(current <= end);
				return ret;
			}
			inline uintptr_t align(uint32_t alignment){
				current = (current + alignment-1) & ~(alignment-1);
				return current;
			}
			uintptr_t begin;
			uintptr_t current;
			uintptr_t end;
			size_t size;
		};

		class VramHeap : public HeapBase{
		public:
			VramHeap(){};
			inline uint32_t AddressToOffset(void* addr){return offset_base + reinterpret_cast<uintptr_t>(addr) - getBegin();}
			inline void init(void* addr, size_t size_in_byte){
				HeapBase::init(addr, size_in_byte);
				uint32_t offset;
				MY_C(cellGcmAddressToOffset(addr,&offset));
				setOffset(offset);
			}
			inline uint32_t getOffsetBase() { return offset_base; }
		private:
			inline void setOffset(uint32_t offset){ offset_base = offset; }
			uintptr_t offset_base;
		};

	private:
		static HeapBase mainHeap;
		static VramHeap mappedMainHeap;
		static VramHeap localHeap;
		static HeapBase temporaryMain;

	public:
		inline static HeapBase& getMainMemoryHeap(){return mainHeap;};
		inline static VramHeap& getMappedMainMemoryHeap(){return mappedMainHeap;};
		inline static VramHeap& getLocalMemoryHeap(){return localHeap;};
		inline static HeapBase& getTemporaryHeap(){return temporaryMain;};
	}; // namespace Memory
};
#endif //PPU_MEMORY_H
