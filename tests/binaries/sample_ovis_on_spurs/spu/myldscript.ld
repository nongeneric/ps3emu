SECTIONS  
{  
ov_vma_start = . ;
ov_lma_start = 0x40000;
vma_start_sec_quicksort = (ov_vma_start + 127) & ~(127);
lma_start_sec_quicksort = (ov_lma_start + 127) & ~(127);
sec_quicksort vma_start_sec_quicksort : AT ( lma_start_sec_quicksort ) { objs/quicksort.spu.o ( .text .rodata )  } 
vma_stop_sec_quicksort = (vma_start_sec_quicksort + SIZEOF( sec_quicksort ) + 127) & ~(127);
lma_stop_sec_quicksort = (lma_start_sec_quicksort + SIZEOF( sec_quicksort ) + 127) & ~(127);

vma_start_sec_bubblesort = (ov_vma_start + 127) & ~(127);
lma_start_sec_bubblesort = (lma_stop_sec_quicksort + 127) & ~(127);
sec_bubblesort vma_start_sec_bubblesort : AT ( lma_start_sec_bubblesort ) { objs/bubblesort.spu.o ( .text .rodata )  } 
vma_stop_sec_bubblesort = (vma_start_sec_bubblesort + SIZEOF( sec_bubblesort ) + 127) & ~(127);
lma_stop_sec_bubblesort = (lma_start_sec_bubblesort + SIZEOF( sec_bubblesort ) + 127) & ~(127);

vma_start_sec_rand_middle_square = (MAX( MAX( ov_vma_start , vma_stop_sec_quicksort ) , vma_stop_sec_bubblesort ) + 127) & ~(127);
lma_start_sec_rand_middle_square = (lma_stop_sec_bubblesort + 127) & ~(127);
sec_rand_middle_square vma_start_sec_rand_middle_square : AT ( lma_start_sec_rand_middle_square ) { objs/rand_middle_square.spu.o ( .text .rodata )  } 
vma_stop_sec_rand_middle_square = (vma_start_sec_rand_middle_square + SIZEOF( sec_rand_middle_square ) + 127) & ~(127);
lma_stop_sec_rand_middle_square = (lma_start_sec_rand_middle_square + SIZEOF( sec_rand_middle_square ) + 127) & ~(127);

vma_start_sec_rand_park_and_miller = (MAX( MAX( ov_vma_start , vma_stop_sec_quicksort ) , vma_stop_sec_bubblesort ) + 127) & ~(127);
lma_start_sec_rand_park_and_miller = (lma_stop_sec_rand_middle_square + 127) & ~(127);
sec_rand_park_and_miller vma_start_sec_rand_park_and_miller : AT ( lma_start_sec_rand_park_and_miller ) { objs/rand_park_and_miller.spu.o ( .text .rodata )  } 
vma_stop_sec_rand_park_and_miller = (vma_start_sec_rand_park_and_miller + SIZEOF( sec_rand_park_and_miller ) + 127) & ~(127);
lma_stop_sec_rand_park_and_miller = (lma_start_sec_rand_park_and_miller + SIZEOF( sec_rand_park_and_miller ) + 127) & ~(127);

. = MAX( MAX( MAX( MAX( ov_vma_start , vma_stop_sec_quicksort ) , vma_stop_sec_bubblesort ) , vma_stop_sec_rand_middle_square ) , vma_stop_sec_rand_park_and_miller );
ov_vma_stop = .;

}
INSERT BEFORE .text


SECTIONS
{
  .data.libovis :

	{ 
		 /* overlay data */ 
. = ALIGN(0x10);	_ovly_table = .;
. = ALIGN(0x10);	__ovly_info_sec_quicksort = .;	LONG(ADDR(sec_quicksort));
LONG((SIZEOF(sec_quicksort) + 127) & ~(127));
LONG(LOADADDR(sec_quicksort));
LONG(0);

. = ALIGN(0x10);	__ovly_info_sec_bubblesort = .;	LONG(ADDR(sec_bubblesort));
LONG((SIZEOF(sec_bubblesort) + 127) & ~(127));
LONG(LOADADDR(sec_bubblesort));
LONG(0);

. = ALIGN(0x10);	__ovly_info_sec_rand_middle_square = .;	LONG(ADDR(sec_rand_middle_square));
LONG((SIZEOF(sec_rand_middle_square) + 127) & ~(127));
LONG(LOADADDR(sec_rand_middle_square));
LONG(0);

. = ALIGN(0x10);	__ovly_info_sec_rand_park_and_miller = .;	LONG(ADDR(sec_rand_park_and_miller));
LONG((SIZEOF(sec_rand_park_and_miller) + 127) & ~(127));
LONG(LOADADDR(sec_rand_park_and_miller));
LONG(0);

. = ALIGN(0x10);	_novlys = .;	LONG(4);. = ALIGN(0x10);

 	} 
  
} 
INSERT AFTER .data
