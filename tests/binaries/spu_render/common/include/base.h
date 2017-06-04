#ifndef COMMON_BASE_H
#define COMMON_BASE_H

#ifdef __SPU__
typedef uint32_t spu_addr;
#else //__PPU__
typedef uintptr_t spu_addr;
#endif

#endif // COMMON_BASE_H