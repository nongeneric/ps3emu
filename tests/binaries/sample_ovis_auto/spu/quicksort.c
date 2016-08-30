/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include "functions.h"
#include <string.h>


static void _quicksort(int a[], int first, int last);

void quicksort(int a[], int n, char* msg)
{
	__builtin_memcpy(msg, "QS", strlen("QS") + 1);
	_quicksort(a,0, n-1);
}



static void _quicksort(int a[], int first, int last)
{
	int i,j;
	int x,t;
	
	x = a[(first+last)/2];
	i = first; j= last;
	for(;;){
		while(a[i] < x){
			i++;
		}
		while(x < a[j]){
			j--;
		}
		if(i>=j){
			break;
		}
		t = a[i];
		a[i] = a[j];
		a[j] = t;
		i++;
		j--;
	}
	if(first < (i -1 )){
		_quicksort(a, first, i-1);
	}
	if((j+1) < last){
		_quicksort(a, j+1, last);
	}
}

