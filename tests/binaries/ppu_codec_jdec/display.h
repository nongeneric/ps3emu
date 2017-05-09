/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

/*E initialize display device. */
int32_t my_disp_create( uint32_t xsize,
						uint32_t ysize );

/*E set texture to the buffer. */
int32_t my_disp_settex( uint8_t *buffer,
						uint32_t xsize,
						uint32_t ysize );

/*E map the buffer. */
int32_t my_disp_mapmem( uint8_t *buffer,
						size_t buf_size,
						uint32_t *buf_offset );

/*E unmap the buffer. */
int32_t my_disp_unmapmem( uint32_t buf_offset );


/*E flip the buffer for display. */
int my_disp_display( void );

/*E destroy display device. */
void my_disp_destroy( void );


#endif /*E __DISPLAY_H__ */
