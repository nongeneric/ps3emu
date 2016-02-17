/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *                Copyright (C) 2010 Sony Computer Entertainment Inc.
 *                                               All Rights Reserved.
 */

#ifndef _JS_ASYNCCOPY_H_
#define _JS_ASYNCCOPY_H_

// memcpy command
//  There is an array of these structs on the SPU.  A client fills one of
//  the array elements then sends the index to the SPU mailbox.
//
//
typedef struct
{
    unsigned long long	src;
    unsigned long long	dst;
    unsigned int		size;

    unsigned int		notifyValue;
    unsigned long long	notifyAddress;
}
jsMemcpyEntry;

// command array size
//  This is the number of elements in the command array.  It must be at
//  least one more than the SPU mailbox queue length (4) plus the number
//  of simultaneous running commands (currently 2).
#define _JS_ASYNCCOPY_QSIZE 16

// command options
//  A new command is signaled by sending a mailbox message with the command
//  index in the low order byte.  The message may also contain option bits
//  in the upper bytes.  The topmost bit is reserved for internal use to tag
//  the command as active.
#define _JS_ASYNCCOPY_NOTIFY_MEMORY		0x00010000
#define _JS_ASYNCCOPY_NOTIFY_MAILBOX	0x00020000
#define _JS_ASYNCCOPY_GPU_SRC			0x00040000
#define _JS_ASYNCCOPY_GPU_DST			0x00080000
#define _JS_ASYNCCOPY_BARRIER			0x00100000
#define _JS_ASYNCCOPY_ACTIVE			0x80000000

#endif // _JS_ASYNCCOPY_H_

