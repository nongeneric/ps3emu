/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include "functions.h"
#include <string.h>

int rand_middle_square(char* msg)
{
	static int seed = (int)1111111111111111;
	long long int tmp = seed;
	__builtin_memcpy(msg, "MS", strlen("MS") + 1);
	tmp = tmp * tmp;
	//tmp &= 0xffffffff0000;
	tmp >>= 16;
	seed = (int)tmp;
	return seed;
}

