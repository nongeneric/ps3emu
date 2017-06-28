/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#ifndef __SORT_BODY_H_INCLUDED__
#define __SORT_BODY_H_INCLUDED__

#include "common.h"
#include <spu_intrinsics.h>

unsigned int QSortSub(unsigned int start_ea,unsigned int end_ea);  // E not recursive 
KeyType SelectPivot(unsigned int start_ea,unsigned int end_ea,SortNodeType& p_data);

INLINE int SelectPivotOnLS(const SortNodeType& a,const SortNodeType& b,const SortNodeType& c)
{
	static int middle[8]=
	{
		0, // E same all!
		2, 
		0,
		1,
		1,
		0,
		2,
		0 // E Error!
	};
	int result;
	result =(Key(a)>Key(b))?1:0;
	result+=(Key(b)>Key(c))?2:0;
	result+=(Key(c)>Key(a))?4:0;
	return middle[result];
}

#endif  //__SORT_BODY_H_INCLUDED__

/*
 * Local Variables:
 * mode: C++
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
