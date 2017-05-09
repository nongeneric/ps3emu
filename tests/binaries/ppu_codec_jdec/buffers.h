/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */


#ifndef	__BUFFERS_H__
#define	__BUFFERS_H__

#include <types.h>
#include <sys/synchronization.h>

/*E member of MyBuffsElem should be */
/*E modified for your environment. */
typedef struct {
	void        *buffer;
	size_t       size;
	uint32_t     width;
	uint32_t     height;
	uint32_t     status;
	uint32_t     interval;
	uint64_t     timestamp[2];
} MyBuffsElem;

/*E main body of buffers. */
typedef struct {
	MyBuffsElem *element;
	uint32_t     offset;
	uint32_t     buffs_num;
	uint32_t     buffs_open;
	uint32_t     buffs_head;
	uint32_t     buffs_tail;
	uint32_t     buffs_count;
	sys_cond_t   buffs_cond;
	sys_mutex_t  buffs_mutex;
} MyBuffs;

/*E create all instances for buffer. */
int32_t
my_buffs_create( MyBuffs *buffs_obj,
				 uint32_t buff_num,
				 size_t   buff_size,
				 uint32_t buff_align );

/*E initialize all buffers. */
void
my_buffs_open( MyBuffs *buffs_obj );

/*E prepare an empty buffer. */
int32_t
my_buffs_prepare( MyBuffs *buffs_obj,
				  MyBuffsElem **buff_elem );

/*E send the prepared buffer. */
int32_t
my_buffs_send( MyBuffs *buffs_obj );

/*E receive the buffer from sender. */
int32_t
my_buffs_receive( MyBuffs *buffs_obj,
				  MyBuffsElem **buff_elem );

/*E release the received buffer. */
int32_t
my_buffs_release( MyBuffs *buffs_obj );

/*E finalize all instances for buffer. */
int32_t
my_buffs_close( MyBuffs *buffs_obj );

/*E destroy all instances for buffer. */
int32_t
my_buffs_destroy( MyBuffs *buffs_obj );


#endif /*E __BUFFERS_H__ */
