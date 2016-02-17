/*
 *   SCE CONFIDENTIAL                                      
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001 
 *   Copyright (C) 2007 Sony Computer Entertainment Inc.    
 *   All Rights Reserved.                                  
 */

/*E
 * File: raw_spu_printf.ppu.c
 * Description
 *   A PPU program to handle SPU printf of a Raw SPU. 
 *
 *   SPU's printf does not actually output a string to a console.  It only
 *   put the argument list in a stack in its local storage, and pass its 
 *   address to PPU as an event, which is actually sent via PPU interrupt MB
 *   expecting that PPU handles it properly.  Therefore, the PPU-side must 
 *   has a system to constantly check an event from SPUs.  This program 
 *   realizes it by creating a PPU interrupt thread which wakes up when
 *   PPU MB interrupt is caught. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/spu_initialize.h>
#include <sys/raw_spu.h>
#include <sys/spu_utility.h>
#include <sys/ppu_thread.h>
#include <sys/interrupt.h>
#include <sys/paths.h>
#include <sys/process.h>
#include <spu_printf.h>
#include <sysutil/sysutil_sysparam.h>  
#include <sys/timer.h>
#include <sys/synchronization.h>
#include "AsyncCopy.h"
#include "assert.h"
#include "string"

SYS_PROCESS_PARAM(1001, 0x10000)

#define EIEIO                 __asm__ volatile ("eieio");
#define INT_STAT_MAILBOX      0x01UL
#define SPU_PROG  (SYS_APP_HOME "/hello.spu.self")
#define PPU_STACK_SIZE 4096

#define MAX_PHYSICAL_SPU       6 
#define MAX_RAW_SPU            1


#define DMA_LSA       MFC_LSA      
#define DMA_EAH       MFC_EAH      
#define DMA_EAL       MFC_EAL      
#define DMA_Size_Tag  MFC_Size_Tag 
#define DMA_Class_CMD MFC_Class_CMD
#define DMA_CMDStatus MFC_CMDStatus
#define DMA_QStatus   MFC_QStatus  
#define PU_MB         SPU_Out_MBox
#define SPU_MB        SPU_In_MBox
#define MB_Stat       SPU_MBox_Status

#define mmio_write_prob_reg sys_raw_spu_mmio_write
#define mmio_read_prob_reg sys_raw_spu_mmio_read
#define GLboolean int

sys_raw_spu_t id;

#define _JS_ASYNCCOPY_NOTIFY_MEMORY		0x00010000
#define _JS_ASYNCCOPY_NOTIFY_MAILBOX	0x00020000
#define _JS_ASYNCCOPY_GPU_SRC			0x00040000
#define _JS_ASYNCCOPY_GPU_DST			0x00080000
#define _JS_ASYNCCOPY_BARRIER			0x00100000
#define _JS_ASYNCCOPY_ACTIVE			0x80000000

unsigned int _jsAsyncCopyQ;		// LS address of command array
unsigned int _jsAsyncIOIF;		// LS address of IOIF window array

#define JS_LIKELY(COND) __builtin_expect((COND),1)
#define JS_UNLIKELY(COND) __builtin_expect((COND),0)

GLboolean _jsIsInStack( const char *ptr )
{
    // Text if the pointer is in the stack. This is done by comparing the upper bits
    // because protection is done at the 256MB segment granularity.
    unsigned long register stackPtr asm( "sp" );
    return ( stackPtr >> 28 ) == (( unsigned long )ptr ) >> 28;
}

static inline unsigned long _jsPad( unsigned long x, unsigned long pad )
    {
        return ( x + pad - 1 ) / pad*pad;
    }

static const char* slowRangeStart = NULL;
static size_t slowRangeSize = 0;

typedef unsigned long long jsEA;
typedef unsigned long jsIntPtr;
#define JS_PTR_TO_EA(ptr) ((jsEA)(jsIntPtr)(ptr))
#define JS_EA_TO_PTR(ea) ((void *)(jsIntPtr)(ea))
//
//static void _jsVMXCopyExtra( void* dst, const void* src, size_t size )
//{
//    /*printf("vmx extra: 0x%p 0x%p %d\n",dst,src,size);*/
//    assert(( JS_PTR_TO_EA( src )&15 ) == 0 );
//    assert(( JS_PTR_TO_EA( dst )&15 ) == 0 );
//
//    vector float* dptr = ( vector float* )dst;
//    vector float* sptr = ( vector float* )src;
//    const int n = size / 16;
//    for ( int i = 0; i < n; ++i )
//        vec_st( vec_ld( 0, sptr++ ), 0, dptr++ );
//
//    if ( JS_UNLIKELY( size & 15 ) )
//        memcpy( dptr, sptr, size & 15 );
//}


void _jsAsyncCopySetSlowRange( const void* start, size_t size, unsigned long long* ioif )
{
    slowRangeStart = ( char * )start;
    slowRangeSize = size;

    // copy IOIF windows
    //  There are 4 independent windows into a 64 MB block of GPU memory:
    //   ioif[0]	block 0 window 0
    //   ioif[1]	block 0 window 1
    //   ioif[2]	block 0 window 2
    //   ioif[3]	block 0 window 3
    //   ioif[4]	block 1 window 0
    //   ioif[5]	block 1 window 1
    //   ioif[6]	block 1 window 2
    //   ioif[7]	block 1 window 3
    //   ...
    //
    //  The number of blocks needed is determined by the upper bound of
    //  available GPU memory.
    const int nBlocks = _jsPad( size, 1 << 26 ) >> 26;
    const int nWindows = 4 * nBlocks;
    unsigned long long* ls = ( unsigned long long* )( LS_BASE_ADDR( id ) + _jsAsyncIOIF );
    for ( int i = 0; i < nWindows; ++i )
        ls[i] = ioif[i];
}

static void _jsAsyncCopyExec(
    char*			dst,
    const char*		src,
    size_t			size,
    int				notifyValue,
    int*			notifyAddress,
    unsigned int	options )
{
    // Test if source or destination is in the stack. Since 0.8.5, SPU access to the stack silently fails (DMA never terminates)
    if ( JS_UNLIKELY(( id < 0 ) || _jsIsInStack( src ) || _jsIsInStack( dst ) ) )
    {
        // use the CPU if the SPU is not running
        //_jsVMXCopy( dst, src, size );
		printf("stack!\n");

        // notify if requested
        if ( options & _JS_ASYNCCOPY_NOTIFY_MEMORY )
            *notifyAddress = notifyValue;
        return;
    }

    // ordering between data write and spu commands
asm volatile( "sync"::: "memory" );

    // check for source slow range
    //  XXX DMA from GPU larger than 16 bytes hangs.
    if ( JS_UNLIKELY(( src + size > slowRangeStart ) & ( src < slowRangeStart + slowRangeSize ) ) )
        options |= _JS_ASYNCCOPY_GPU_SRC;

    // check for destination slow range
    //  Using multiple IOIF windows is faster.
    //  Can't use cache prealloc.
    if (( dst + size > slowRangeStart ) & ( dst < slowRangeStart + slowRangeSize ) )
    {
        options |= _JS_ASYNCCOPY_GPU_DST;
        //  	dst = _JS_NV_UNMAP_GPU_ADDRESS(dst);  RS XXX need to add generic wrapper for this as AsysCopy should be Rasterizer independant
    }

    static unsigned int qIndex = 0;

    // prepare command struct
    static union U
    {
        jsMemcpyEntry entry;
        vector float v[2];
    }
    u __attribute__(( aligned( 16 ) ) );
    u.entry.src				= ( unsigned long long )( unsigned long )src;
    u.entry.dst				= ( unsigned long long )( unsigned long )dst;
    u.entry.size			= size;
    u.entry.notifyValue		= notifyValue;
    u.entry.notifyAddress	= ( unsigned long long )( unsigned long )notifyAddress;

    // copy command into mapped memory with VMX
    assert( sizeof( jsMemcpyEntry ) == 32 );
    union U* ls = ( union U* )( LS_BASE_ADDR( id ) +
                                            _jsAsyncCopyQ + qIndex * sizeof( jsMemcpyEntry ) );
    ls->v[0] = u.v[0];
    ls->v[1] = u.v[1];

    // wait for mailbox slot
    while (( mmio_read_prob_reg( id, MB_Stat ) & 0xff00 ) == 0 )
{
        sys_timer_usleep( 10 );
    }

    // ordering between command writes and spu kick
asm volatile( "eieio"::: "memory" );

    // notify SPU of new entry
    mmio_write_prob_reg( id, SPU_MB, qIndex | options );

    qIndex = ( qIndex + 1 ) % _JS_ASYNCCOPY_QSIZE;
}

void _jsAsyncCopyNotify( void* dst, const void* src, size_t size, int notifyValue, int* notifyAddress )
{
    assert( dst != NULL );
    assert( src != NULL );
    assert( notifyAddress != NULL );

    _jsAsyncCopyExec(( char * )dst, ( const char * )src, size, notifyValue, notifyAddress, _JS_ASYNCCOPY_NOTIFY_MEMORY );
}


#define _JS_ASYNCCOPY_NOTIFY_MEMORY		0x00010000
#define _JS_ASYNCCOPY_NOTIFY_MAILBOX	0x00020000
#define _JS_ASYNCCOPY_GPU_SRC			0x00040000
#define _JS_ASYNCCOPY_GPU_DST			0x00080000
#define _JS_ASYNCCOPY_BARRIER			0x00100000
#define _JS_ASYNCCOPY_ACTIVE			0x80000000

#define _JS_ASYNCCOPY_BARRIER_TOKEN 0xdeadbeef

void _jsAsyncCopy( void* dst, const void* src, size_t size )
{
    assert( dst != NULL );
    assert( src != NULL );

    _jsAsyncCopyExec(( char * )dst, ( const char * )src, size, 0, NULL, 0 );
}

void _jsAsyncCopyFinish( void )
{
    _jsAsyncCopyExec(
        NULL, NULL, 0,
        _JS_ASYNCCOPY_BARRIER_TOKEN, NULL,
        _JS_ASYNCCOPY_BARRIER | _JS_ASYNCCOPY_NOTIFY_MAILBOX );

    // wait for mailbox return
    do
    {
        do
        {
            asm volatile( "eieio" );
        }
        while ( ~mmio_read_prob_reg( id, MB_Stat ) & 0x1 );
    }
    while ( mmio_read_prob_reg( id, PU_MB ) != _JS_ASYNCCOPY_BARRIER_TOKEN );
}


int main(void)
{
	sys_mutex_attribute_t attr;
	sys_mutex_attribute_initialize(attr);

	int ret;
	
	sys_spu_image_t spu_img;


	if (ret != CELL_OK) {
		printf("systemUtilityInit() failed %x\n", ret);
		exit(ret);
	}

	/*E
	 * Initialize SPUs
	 */
	printf("Initializing SPUs\n", 0);
	ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);
	if (ret) {
		printf("sys_spu_initialize failed %x\n", ret);
		exit(ret);
	}

	/*E
	 * Execute a series of system calls to load a program to a Raw SPU.
	 */
	ret = sys_raw_spu_create(&id, NULL);
	if (ret) {
		printf("sys_raw_spu_create failed %x\n", ret);
		exit(ret);
	}
	printf("sys_raw_spu_create succeeded. raw_spu number is %d\n", id);


	ret = sys_spu_image_open(&spu_img, SPU_PROG);
	if (ret != CELL_OK) {
		printf("sys_spu_image_open failed %x\n", ret);
		exit(1);
	}

	ret = sys_raw_spu_image_load(id, &spu_img);
	if (ret) {
		printf("sys_raw_spu_load failed %x\n", ret);
		exit(1);
	}

    asm volatile( "eieio"::: "memory" );
    mmio_write_prob_reg( id, SPU_RunCntl, 1 );

	// init async copy
    
	printf("waiting for _jsAsyncCopyQ\n");
    do {
		asm volatile( "eieio"::: "memory" );
    }
    while ( ~mmio_read_prob_reg( id, MB_Stat ) & 0x1 );
	
	printf("reading for _jsAsyncCopyQ\n");
    _jsAsyncCopyQ = mmio_read_prob_reg( id, PU_MB );


	printf("waiting for _jsAsyncIOIF\n");
    // get IOIF window array address
    do {
		asm volatile( "eieio"::: "memory" );
    }
    while ( ~mmio_read_prob_reg( id, MB_Stat ) & 0x1 );
	printf("reading for _jsAsyncIOIF\n");
    _jsAsyncIOIF = mmio_read_prob_reg( id, PU_MB );

	//

	char* sourceBuffer = new char[128];
	char* destBuffer = new char[150];
	int* notify = new int();
	memset(sourceBuffer, 0xcd, 128);
	memset(destBuffer, 0x11, 128);

	printf("starting copy\n");
	_jsAsyncCopy(destBuffer, sourceBuffer, 128);

	printf("waiting for completion\n");
	_jsAsyncCopyFinish();

	if (memcmp(sourceBuffer, destBuffer, 128)) {
		printf("failure!\n");
	} else {
		printf("success!\n");
	}

	if ((ret = sys_raw_spu_destroy(id)) != CELL_OK) {
		printf("sys_raw_spu_destroy failed %x\n", ret);
		exit(ret);
	}

	ret = sys_spu_image_close(&spu_img);
	if (ret != CELL_OK) {
		printf("sys_spu_image_close failed %x\n", ret);
		exit(ret);
	}

	return 0;
}

