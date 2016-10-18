/* SCE CONFIDENTIAL
* PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

#define	BUFFER_AREA_SIZE	(1024 * 16)

typedef struct Command {
	int			type;
	uint32_t	__pad__;
	uint64_t	eaSrc;
	uint64_t	eaDst;
	uint64_t	value;
} __attribute__((aligned(16))) Command;

enum CommandType {
	CommandIncrement,
	CommandCheckValue,
	CommandTerminate
};

#endif /* __COMMON_H__ */

