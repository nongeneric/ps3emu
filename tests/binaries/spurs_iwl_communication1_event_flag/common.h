/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

#include <sdk_version.h>

#if	CELL_SDK_VERSION >= 0x080000
typedef	uint32_t	lv2_ea_t;
#else   /* CELL_SDK_VERSION >= 0x080000 */
typedef	uint64_t	lv2_ea_t;
#endif  /* CELL_SDK_VERSION >= 0x080000 */

#define	CEIL128(x)	((x + 127) & ~127)

typedef struct {
	lv2_ea_t	event_flag_ppu_spu_ea;
	lv2_ea_t	event_flag_spu_spu_ea;
	lv2_ea_t	event_flag_spu_ppu_ea;
	uint8_t		padding[CEIL128(sizeof(lv2_ea_t) * 3) - sizeof(lv2_ea_t) * 3];
} Args;

#endif /* __COMMON_H__ */

