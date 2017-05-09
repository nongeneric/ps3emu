/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2006 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffers.h"

/*E create all instances for buffer. */
int32_t my_buffs_create( MyBuffs  *buffs_obj,
						 uint32_t  buff_num,
						 size_t    buff_size,
						 uint32_t  buff_align )
{

	sys_cond_attribute_t cond_attr;
	sys_mutex_attribute_t mutex_attr;

	buffs_obj->buffs_open = 0;
	buffs_obj->buffs_num  = buff_num;

	sys_cond_attribute_initialize( cond_attr );
	sys_mutex_attribute_initialize( mutex_attr );

	if( CELL_OK != sys_mutex_create( &buffs_obj->buffs_mutex,
									 &mutex_attr ) ){
		printf( "my_buffs_create> sys_mutex_create failed.\n" );
		return -1;
	}

	if( CELL_OK != sys_cond_create( &buffs_obj->buffs_cond,
									buffs_obj->buffs_mutex,
									&cond_attr ) ){
		printf( "my_buffs_create> sys_cond_create failed.\n" );
		return -1;
	}

	buffs_obj->element =
		(MyBuffsElem*)memalign( 16, sizeof(MyBuffsElem)*buff_num );
	if( NULL == buffs_obj->element ){
		printf( "my_buffs_create> memalign failed.\n" );
		return -1;
	}

	for( uint32_t i_cnt = 0; i_cnt < buff_num; i_cnt++ ){
		buffs_obj->element[i_cnt].buffer = memalign( buff_align, buff_size );
		if( NULL == buffs_obj->element[i_cnt].buffer ){
			printf( "my_buffs_create> memalign failed.\n" );
			return -1;
		}
	}


	return 0;

}

/*E initialize all buffers. */
void my_buffs_open( MyBuffs *buffs_obj )
{

	buffs_obj->buffs_head  = 0;
	buffs_obj->buffs_tail  = 0;
	buffs_obj->buffs_count = 0;
	buffs_obj->buffs_open  = 1;

}

/*E prepare an empty buffer. */
int32_t my_buffs_prepare( MyBuffs *buffs_obj,
						  MyBuffsElem **buff_elem )
{

	int32_t retval = 0;

	sys_mutex_lock( buffs_obj->buffs_mutex, 0 );

	do{

		while( (buffs_obj->buffs_num <= (buffs_obj->buffs_count+1)) &&
			   buffs_obj->buffs_open )
			sys_cond_wait(buffs_obj->buffs_cond, 0);

		if( !buffs_obj->buffs_open ){
			retval = -1;
			break;
		}
		
		if( buff_elem != NULL )
			*buff_elem = &buffs_obj->element[buffs_obj->buffs_tail];

	}while(0);

	sys_cond_signal( buffs_obj->buffs_cond );
	sys_mutex_unlock( buffs_obj->buffs_mutex );

	return retval;

}

/*E send the prepared buffer. */
int32_t my_buffs_send( MyBuffs *buffs_obj )
{
	
	int32_t retval = 0;

	sys_mutex_lock( buffs_obj->buffs_mutex, 0 );

	do{

		while( (buffs_obj->buffs_num <= (buffs_obj->buffs_count+1)) &&
			   buffs_obj->buffs_open )
			sys_cond_wait( buffs_obj->buffs_cond, 0 );
		
		if( !buffs_obj->buffs_open ){
			retval = -1;
			break;
		}
		
		buffs_obj->buffs_tail =
			(buffs_obj->buffs_tail+1) % buffs_obj->buffs_num;
		buffs_obj->buffs_count++;
		
	}while(0);

	sys_cond_signal( buffs_obj->buffs_cond );
	sys_mutex_unlock( buffs_obj->buffs_mutex );

	return retval;
}

/*E receive the buffer from sender. */
int32_t my_buffs_receive( MyBuffs *buffs_obj,
						  MyBuffsElem **buff_elem )
{

	int32_t retval = 0;

	sys_mutex_lock( buffs_obj->buffs_mutex, 0 );

	do{

		while( (0 >= buffs_obj->buffs_count) &&
			   buffs_obj->buffs_open )
			sys_cond_wait( buffs_obj->buffs_cond, 0 );

		if( !buffs_obj->buffs_open ){
			retval = -1;
			break;
		}

		if( buff_elem != NULL )
			*buff_elem = &buffs_obj->element[buffs_obj->buffs_head];

	}while(0);

	sys_cond_signal( buffs_obj->buffs_cond );
	sys_mutex_unlock( buffs_obj->buffs_mutex );
    
	return retval;
}

/*E release the received buffer. */
int32_t my_buffs_release( MyBuffs *buffs_obj )
{

	int32_t retval = 0;

	sys_mutex_lock( buffs_obj->buffs_mutex, 0 );

	do{

		if( !buffs_obj->buffs_open ||
			0 == buffs_obj->buffs_count )
			break;

		buffs_obj->buffs_head =
			(buffs_obj->buffs_head+1) % buffs_obj->buffs_num;
		buffs_obj->buffs_count--;

	}while(0);
		
	sys_cond_signal( buffs_obj->buffs_cond );
	sys_mutex_unlock( buffs_obj->buffs_mutex );

	return retval;
}

/*E finalize all instances for buffer. */
int32_t my_buffs_close( MyBuffs *buffs_obj )
{

	sys_mutex_lock( buffs_obj->buffs_mutex, 0 );
	buffs_obj->buffs_open = 0;
	sys_cond_signal( buffs_obj->buffs_cond );
	sys_mutex_unlock( buffs_obj->buffs_mutex );
	return 0;

}

/*E destroy all instances for buffer. */
int32_t my_buffs_destroy( MyBuffs *buffs_obj )
{

	for( uint32_t i_cnt = 0; i_cnt < buffs_obj->buffs_num; i_cnt++ )
		free( buffs_obj->element[i_cnt].buffer );

	if( CELL_OK != sys_cond_destroy(buffs_obj->buffs_cond) ){
		printf( "my_buffs_destroy> sys_cond_destroy failed.\n" );
		return -1;
	}

	if( CELL_OK != sys_mutex_destroy(buffs_obj->buffs_mutex) ){
		printf( "my_buffs_destroy> sys_mutex_destroy failed.\n" );
		return -1;
	}

	free( buffs_obj->element );

	return 0;

}
