/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#ifndef __COMMON_H_INCLUDED__
#define __COMMON_H_INCLUDED__

#define ASSERT_ON

#define MAX_SPU_NUM  5

#define INLINE inline __attribute__((always_inline))

#if 1
#include <spu_intrinsics.h>
#include <spu_printf.h>
#define dbg_printf spu_printf
#else
#define dbg_printf(...)
#endif

#if 1
/* spurs */
#include <cell/spurs.h>
#define HALT do{cellSpursShutdownTaskset (cellSpursGetTasksetAddress ());cellSpursExit ();}while(0)
#else
#define HALT	spu_hcmpeq(0,0)
#endif

#ifdef ASSERT_ON
#define AssertIf(x)  do{if(x){dbg_printf("\nUnexpected Error!\n at %s \n     (%d line %s)\n Assertion Exp is [%s]\n\n",__PRETTY_FUNCTION__,__LINE__,__FILE__,#x);HALT;}}while(0)
#else
//#define AssertIf(x)  do{(x);}while(0)
#define AssertIf(x)  
#endif

#define IF0(_x)	if(__builtin_expect((_x),0))
#define IF1(_x)	if(__builtin_expect((_x),1))
#define WHILE0(_x)	while(__builtin_expect((_x),0))
#define WHILE1(_x)	while(__builtin_expect((_x),1))

typedef vector unsigned int SortNodeType;
typedef unsigned int        KeyType;

INLINE static KeyType Key(const SortNodeType& s)
{
	return spu_extract(s,0);
}


#endif  //__COMMON_H_INCLUDED__

/*
 * Local Variables:
 * mode:C++
 * c-file-style: "stroustrup"
 * tab-width:4
 * End:
 * vim:ts=4:sw=4:
 */
