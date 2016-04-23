/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#ifndef __SPU_H_
#define __SPU_H_
#include <stdio.h>
#include <sys/paths.h>
/*E
 * Macros to obtain the upper and lower 32 bits of a 64-bit effective address
 */
#define EAH(ea64) (uint32_t)((uint64_t)(ea64) >> 32)
#define EAL(ea64) (uint32_t)((uint64_t)(ea64) & 0xFFFFFFFFULL)

/*E
 * EIEIO: Guarantees the previous register access have completed. 
 */
#define EIEIO __asm__ volatile("eieio")

#define SPU_PROG  (SYS_APP_HOME "/duck.spu.elf")
#define MFC_TAG      0
#define MFC_PUT   0x20
#define BUF_SIZE   128

#define MAX_PHYSICAL_SPU       6 
#define MAX_RAW_SPU            1

/*E 
 * DMA buffers that are aligned to 128 bytes 
 */

typedef struct
{
	uint32_t  fragment_constant_addr;
	uint32_t  spu_read_label_addr;
	uint32_t  spu_write_label_addr;
} SpuParam_t;


void setupSpuSharedBuffer(uint32_t const_addr, uint32_t read_addr, uint32_t write_addr);
void signalSpuFromPpu(void);
int initSpu(void );
void signalSpuFromRsx(void);
void waitSignalFromSpu(void);
void clearSignalFromSpu(void);
#endif //__SPU_H_
