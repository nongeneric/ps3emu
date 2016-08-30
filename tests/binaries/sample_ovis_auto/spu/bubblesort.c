/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include "functions.h"
#include <string.h>

void bubblesort(int a[], int n, char* msg)
{
	int i, j, k;
	int x;
	k = n-1;
	while(k>=0){
		j = -1;
		for(i=1; i<=k; i++){
			if(a[i-1] > a[i]){
				j = i-1;
				x = a[j]; 
				a[j] = a[i];
				a[i] = x;
			}
		}
		k = j;
	}
	__builtin_memcpy(msg, "BS", strlen("BS") + 1);
}
