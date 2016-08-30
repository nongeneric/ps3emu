/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>
#include <stdint.h>
#include <cell/sheap/key_sheap_buffer.h>

#include <cell/dma.h>
#include <cellstatus.h>
#include <string.h>
#include <stdio.h>
#include <spu_printf.h>
#include <stdlib.h>

#define BUFFER_SIZE 128

#define N_BUFFER (128/sizeof(unsigned int))
unsigned int buffer1[N_BUFFER] __attribute__ ((aligned (128)));
unsigned int buffer2[N_BUFFER] __attribute__ ((aligned (128)));
unsigned int current_buffer[N_BUFFER] __attribute__ ((aligned (128)));


int isPrime(uint64_t p_array, unsigned int n_array, unsigned int n);
int isPrime2(unsigned int a_buffer[], int n_array, unsigned int n);



int isPrime(uint64_t p_array, unsigned int n_array, unsigned int n)
{
	unsigned int back_tag = 3;
	unsigned int m_bufs = n_array/32;
	unsigned int *for_buffer = buffer1;
	unsigned int *back_buffer = buffer2;
	unsigned int *tmp;

	unsigned int m=0;
	if(m_bufs > 0){
		cellDmaGet(back_buffer, p_array + m * BUFFER_SIZE , 128, back_tag, 0, 0);
		for(m=0; m<m_bufs; m++){
			cellDmaWaitTagStatusAll(1<<back_tag);
			tmp = for_buffer;
			for_buffer = back_buffer;
			back_buffer = tmp;
			if(m<m_bufs-1){
				cellDmaGet(back_buffer, p_array + (m+1) * BUFFER_SIZE , 128, back_tag, 0, 0);
			}
			for(unsigned int k=0;k<32;k++){
				if((n % for_buffer[k]) == 0){
					return 0;
				}
			}
		}
	}
	return 1;
}


int isPrime2(unsigned int a_buffer[], int n_array, unsigned int n)
{
	int j;
	for(j = 0; j<n_array; j++){
		if((n % a_buffer[j]) == 0){
			return 0;
		}
	}
	return 1;
}

int main(uint64_t ea_sheap, uint64_t key, uint64_t N) 
{
	//N = 32;
	unsigned int i,n;
	unsigned int tag1 = 3;
	uint64_t unknown_size = 0;
	int ret;
	CellKeySheapBuffer b;
	uint64_t ans;

	ret = cellKeySheapBufferNew(&b, ea_sheap, key, unknown_size);
	if(ret != CELL_OK){
		exit(ret);
	}

	ans = cellKeySheapBufferGetEa(&b);

	n = 2;
	i = 0;
	while(i<N){
		unsigned int i_old = i &  ~31;
		if(isPrime(ans, i_old, n)){
			if(isPrime2(current_buffer, i - i_old, n)){
				current_buffer[i - i_old] = n;
				if((i - i_old)== 31){

					cellDmaPut(current_buffer, ans+i_old*sizeof(unsigned int), 128, tag1, 0, 0);
					cellDmaWaitTagStatusAll(1<<tag1);
					
				}
						
				i++;

			}
			n++;
		}else{
			n++;
		}
	}
	unsigned int i_old = i &  ~31;

	cellDmaPut(current_buffer, ans+i_old*sizeof(unsigned int), 128, tag1, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag1);
	
	cellKeySheapBufferDelete(&b);

	return 0;
}
