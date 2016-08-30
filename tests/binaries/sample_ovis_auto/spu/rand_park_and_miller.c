/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include "functions.h"


#include <stdint.h>
#include <string.h>

#define A 16807
#define M 2147483657
int rand_park_and_miller(char* msg)
{
	__builtin_memcpy(msg, "PM", strlen("PM") + 1);
	static int seed = 1;
	seed = (int)(((int64_t)seed * A)%M);
	return seed;
}

