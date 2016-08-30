/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include <stdio.h>
#include <cell/ovis.h>
#include <cell/ovis/debug.h>
#include <cell/ovis/mapper.h>

#include <sys/spu_thread.h>
#include <stdlib.h>
#include <cell/dma.h>

#include "functions.h"

#define QUICK_SORT 1
#define BUBBLE_SORT 0
#define RAND_MIDDLE_SQUARE 1
#define RAND_PARK_AND_MILLER 0


#define MYTAG 3
#define MYTAG_MASK (1<<MYTAG)

#define N_ONEWAY 8
#define N_WAY    16
int  ramdom_orderd_array[N_ONEWAY] __attribute__((aligned(128)));

#define MESSAGE_SIZE 128
char message[N_WAY + 1][2][MESSAGE_SIZE] __attribute__((aligned(128)));

void set_array(uint64_t obuf, int array[], char* mrand, char* msort);


void set_array(uint64_t obuf, int array[], char* mrand, char* msort)
{
	int sort_selector = array[1];
	int j;
	(void)obuf;

			

	if(array[0] == RAND_MIDDLE_SQUARE){
		for(j =0; j<N_ONEWAY; j++){
			ramdom_orderd_array[j] = rand_middle_square(mrand);
		}
	}else{
		for(j =0; j<N_ONEWAY; j++){
			ramdom_orderd_array[j] = rand_park_and_miller(mrand);
		}
	}

	if(sort_selector == QUICK_SORT){
		quicksort((int*)array, N_ONEWAY, msort);
	}else{
		bubblesort((int*)array, N_ONEWAY, msort);
	}
}



int main(uint64_t obuf, uint64_t ea_message, uint64_t ea_ramdom_orderd_array)
{ 
	cellOvisInitializeAutoMapping(obuf, MYTAG);

	int i ;
	snprintf((char*)message[N_WAY][0],MESSAGE_SIZE, 
			 "Addresses are:\n"
			 "\trand_middle_square\t=0x%p\n"
			 "\trand_park_and_miller\t=0x%p\n"
			 "\tquicksort\t=0x%p\n"
			 "\tbubblesort\t=0x%p\n",
			 (void*)rand_middle_square, 
			 (void*)rand_park_and_miller,
			 (void*)quicksort,
			 (void*)bubblesort);
	
	for(i=0; i<N_WAY; i++){

		cellDmaGet(ramdom_orderd_array, 
				   ea_ramdom_orderd_array + i * sizeof(int) * N_ONEWAY, 
				   sizeof(int) * N_ONEWAY, MYTAG, 0 ,0 );
		cellDmaWaitTagStatusAll(1<<MYTAG);


		set_array(obuf, (int*)ramdom_orderd_array, (char*)message[i][0], (char*)message[i][1]);

		cellDmaPut(ramdom_orderd_array, 
				   ea_ramdom_orderd_array + i * sizeof(int) * N_ONEWAY, 
				   sizeof(int) * N_ONEWAY, MYTAG, 0 ,0 );
 		cellDmaWaitTagStatusAll(1<<MYTAG);
		
	}

	cellDmaPut(message , ea_message, MESSAGE_SIZE * 2 * (N_WAY+1), MYTAG, 0 ,0 );
	cellDmaWaitTagStatusAll(1<<MYTAG);

	
  return 0;
}
