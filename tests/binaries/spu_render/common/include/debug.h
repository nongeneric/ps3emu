#ifndef COMMON_DEBUG_H
#define COMMON_DEBUG_H

#ifdef _DEBUG
#define MY_ASSERT(x) do{if(!(x)){ __builtin_snpause();}}while(false)
#define MY_ALIGN_CHECK16(x) MY_ASSERT(((x) & 0xf) == 0)
#define MY_C(statement) \
	do{ int32_t hr = (statement); MY_ASSERT(hr == CELL_OK); }while(false)
#ifdef __PPU__
#define MY_DPRINTF(...) printf(__VA_ARGS__)
#else // !__PPU__
#define MY_DPRINTF(...) spu_printf(__VA_ARGS__)
#endif // __PPU__
#else // !DEBUG
#define MY_ASSERT(x) (void)(x)
#define MY_ALIGN_CHECK16(x)
#define MY_DPRINTF(...)
#define MY_C(statement) statement
#endif // DEBUG

// SPU only debug macro
#ifdef __SPU__
#ifdef _DEBUG
#include <cell/lsguard.h>
#define MY_LSGUARD_CHECK()							\
	do{if(cellLsGuardCheckCorruption() != CELL_OK){	\
			__builtin_snpause();					\
			cellLsGuardRehash(); 					\
		}}while(false)
#define MY_LSGUARD_REHASH() cellLsGuardRehash()
#else // ! _DEBUG
#define MY_LSGUARD_CHECK()
#define MY_LSGUARD_REHASH()
#endif // _DEBUG
#endif // __SPU__

#endif // COMMON_DEBUG_H
