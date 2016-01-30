/*
 *   SCE CONFIDENTIAL                                      
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001 
 *   Copyright (C) 2004 Sony Computer Entertainment Inc.    
 *   All Rights Reserved.
 */

/*E                                   
 * File: hello.spu.c
 * Description:
 *   SPU's printf workes as follows.
 *   1. Push its arguments to a stack located in its LS.
 *   2. Pass the LS address of the stack as an SPU_EVENT_PU_THREAD_CALL event.
 *   3. Wait for a message at SPU_MB, and return it.
 */
#include <spu_printf.h>

int main(void)
{
	spu_printf("Hello, World 1\n");
	spu_printf("Hello, World 2\n");
	spu_printf("%c, %o, %d, 0x%x, 0X%X, %4d, \"%s\"\n", 'a', 10, 20, 30, 40, 50,
			   "test");

	return 0;
}

