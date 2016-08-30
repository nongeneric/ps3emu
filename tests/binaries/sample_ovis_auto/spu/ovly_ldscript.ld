SECTIONS  
{  
ov_vma_start = .;
ov_lma_start = 0x40000;
vma_start_objs__bubblesort_spu_o = (ov_vma_start + 15) & ~(15);
lma_start_objs__bubblesort_spu_o = (ov_lma_start + 15) & ~(15);
objs__bubblesort_spu_o vma_start_objs__bubblesort_spu_o : AT( lma_start_objs__bubblesort_spu_o){  objs/bubblesort.spu.o(.text .rodata) }
vma_stop_objs__bubblesort_spu_o = (vma_start_objs__bubblesort_spu_o + SIZEOF(objs__bubblesort_spu_o) + 15) & ~(15);
lma_stop_objs__bubblesort_spu_o = (lma_start_objs__bubblesort_spu_o + SIZEOF(objs__bubblesort_spu_o) + 15) & ~(15);

vma_start_objs__quicksort_spu_o = (ov_vma_start + 15) & ~(15);
lma_start_objs__quicksort_spu_o = (lma_stop_objs__bubblesort_spu_o + 15) & ~(15);
objs__quicksort_spu_o vma_start_objs__quicksort_spu_o : AT( lma_start_objs__quicksort_spu_o){  objs/quicksort.spu.o(.text .rodata) }
vma_stop_objs__quicksort_spu_o = (vma_start_objs__quicksort_spu_o + SIZEOF(objs__quicksort_spu_o) + 15) & ~(15);
lma_stop_objs__quicksort_spu_o = (lma_start_objs__quicksort_spu_o + SIZEOF(objs__quicksort_spu_o) + 15) & ~(15);

vma_start_objs__rand_middle_square_spu_o = (ov_vma_start + 15) & ~(15);
lma_start_objs__rand_middle_square_spu_o = (lma_stop_objs__quicksort_spu_o + 15) & ~(15);
objs__rand_middle_square_spu_o vma_start_objs__rand_middle_square_spu_o : AT( lma_start_objs__rand_middle_square_spu_o){  objs/rand_middle_square.spu.o(.text .rodata) }
vma_stop_objs__rand_middle_square_spu_o = (vma_start_objs__rand_middle_square_spu_o + SIZEOF(objs__rand_middle_square_spu_o) + 15) & ~(15);
lma_stop_objs__rand_middle_square_spu_o = (lma_start_objs__rand_middle_square_spu_o + SIZEOF(objs__rand_middle_square_spu_o) + 15) & ~(15);

vma_start_objs__rand_park_and_miller_spu_o = (ov_vma_start + 15) & ~(15);
lma_start_objs__rand_park_and_miller_spu_o = (lma_stop_objs__rand_middle_square_spu_o + 15) & ~(15);
objs__rand_park_and_miller_spu_o vma_start_objs__rand_park_and_miller_spu_o : AT( lma_start_objs__rand_park_and_miller_spu_o){  objs/rand_park_and_miller.spu.o(.text .rodata) }
vma_stop_objs__rand_park_and_miller_spu_o = (vma_start_objs__rand_park_and_miller_spu_o + SIZEOF(objs__rand_park_and_miller_spu_o) + 15) & ~(15);
lma_stop_objs__rand_park_and_miller_spu_o = (lma_start_objs__rand_park_and_miller_spu_o + SIZEOF(objs__rand_park_and_miller_spu_o) + 15) & ~(15);

. = (( MAX( vma_stop_objs__rand_park_and_miller_spu_o , MAX( vma_stop_objs__rand_middle_square_spu_o , MAX( vma_stop_objs__quicksort_spu_o , MAX( vma_stop_objs__bubblesort_spu_o , 0)))) + 127) & ~(127));
ov_vma_stop = .;
}
INSERT BEFORE .text


SECTIONS
{
  .data.libovis :

	{ 
		 /* overlay data */ 
. = ALIGN(0x10);	_ovly_table = .;
. = ALIGN(0x10); __ovly_info_objs__bubblesort_spu_o = .;  LONG(ADDR(objs__bubblesort_spu_o)); 
LONG((SIZEOF(objs__bubblesort_spu_o) + 15) & ~(15));
LONG(LOADADDR(objs__bubblesort_spu_o));LONG(0);. = ALIGN(0x10); __ovly_info_objs__quicksort_spu_o = .;  LONG(ADDR(objs__quicksort_spu_o)); 
LONG((SIZEOF(objs__quicksort_spu_o) + 15) & ~(15));
LONG(LOADADDR(objs__quicksort_spu_o));LONG(0);. = ALIGN(0x10); __ovly_info_objs__rand_middle_square_spu_o = .;  LONG(ADDR(objs__rand_middle_square_spu_o)); 
LONG((SIZEOF(objs__rand_middle_square_spu_o) + 15) & ~(15));
LONG(LOADADDR(objs__rand_middle_square_spu_o));LONG(0);. = ALIGN(0x10); __ovly_info_objs__rand_park_and_miller_spu_o = .;  LONG(ADDR(objs__rand_park_and_miller_spu_o)); 
LONG((SIZEOF(objs__rand_park_and_miller_spu_o) + 15) & ~(15));
LONG(LOADADDR(objs__rand_park_and_miller_spu_o));LONG(0);. = ALIGN(0x10);	_novlys = .;	LONG(4); . = ALIGN(0x10);	
 	} 
  
} 
INSERT AFTER .data
