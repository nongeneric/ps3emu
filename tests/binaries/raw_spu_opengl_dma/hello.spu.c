/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *                Copyright (C) 2010 Sony Computer Entertainment Inc.
 *                                               All Rights Reserved.
 */

 
/*
 * 
 */

#include <stdlib.h>

#ifdef OS_VERSION_0_3_0
#define MFC_WrTagMask mfc_wr_tag_mask
#define SPU_WrOutMbox mfc_wr_mailbox
#define SPU_RdInMbox mfc_rd_mailbox
#endif

#include "AsyncCopy.h"

#define _JS_NJOBS 2		// number of simultaneous copies (affects _JS_ASYNCCOPY_QSIZE)
#define _JS_NBUFFERS 2	// number of buffers per copy
#define _JS_BSIZE 16384	// buffer size (max 16384)
#define _JS_WSIZE 1024	// window buffer size
#define _JS_NWINDOWS 2	// number of IOIF windows to use

// command requests
//  Each struct represents a memcpy call.  The PU DMAs the command into a
//  slot and puts the index of the slot into the SPU mailbox.  Some
//  interlock must be used to keep the PU from overwriting a previous
//  command before it is safe to do so.  The mechanism that does this is
//  that the PU must stall when the SPU mailbox is full.  The queue must be
//  sized large enough so that the number of active commands plus the number
//  in the queue plus the entry the PU is adding will all fit.
volatile jsMemcpyEntry entry[_JS_ASYNCCOPY_QSIZE] __attribute__(( aligned( 128 ) ) );

// temporary buffers
char buffer[_JS_NJOBS][_JS_NBUFFERS][_JS_BSIZE] __attribute__(( aligned( 128 ) ) );

// IOIF window addresses
//  There are 4 windows for each of 4 64MB blocks in GPU memory:
//
//	0-3		1st 64MB
//	4-7		2nd 64MB
//	8-11	3rd 64MB
//	12-15	4th 64MB
//
//  ioif[0], ioif[1], ioif[2], and ioif[3] are pointers to the same 64MB of
//  memory on the GPU.  Similarly, ioif[4], ioif[5], ioif[6], and ioif[7]
//  all point to the next 64MB, and so on.
//
//	DMA using multiple windows can be faster than to a single window.  This
//	requires sizing the DMA so we actually get parallelism.
unsigned long long ioif[32] __attribute__(( aligned( 128 ) ) );
static inline unsigned long long _jsMapGPUAddress( unsigned long long addr, int wid )
{
    // compute window index
    //  - determine which 64MB block the address is in
    //  - multiply the block by 4 to get the index of window 0
    //  - add the window id (0-3)
    const int index = (( addr >> 24 ) & 0x1c ) + wid;

    // lower bits are offset from the window base address
    const unsigned long long offset = addr & 0x3ffffff;

    return ioif[index] + offset;
}

// DMA tag usage
//  For each job:
//   _JS_NBUFFERS tags (1 for each buffer)
//  4 tags for notification, one for 0x0, 0x4, 0x8, and 0xc address

// busy buffer bitfield (1 busy, 0 idle)
//  This helper function gets the DMA status of each buffer used in a
//  particular job, returned as a bitfield _JS_NBUFFERS wide in the low
//  order bits.  If the returned value is 0, DMA has been completed for this
//  job.
static inline unsigned int getJobDMAStatus( int job )
{
    const int mask = ( 1 << _JS_NBUFFERS ) - 1;
    const int jobShift = job * _JS_NBUFFERS;
    spu_writech( MFC_WrTagMask, mask << jobShift );

    return ( spu_mfcstat( 0 ) >> jobShift ) ^ mask;
}

// buffer DMA tag
//  This helper function provides the correct DMA tag for a job and buffer.
static inline int getBufferTag( int job, int buffer )
{
    return buffer + job*_JS_NBUFFERS;
}

int main()
{
	//__builtin_snpause();
    // send address of command buffer to PU
    spu_writech( SPU_WrOutMbox, ( unsigned int )entry );

    // send address of IOIF window array to PU
    spu_writech( SPU_WrOutMbox, ( unsigned int )ioif );

    // map jobs to command entries
    int jobMsg[_JS_NJOBS] = {0};

    while ( 1 )
    {
        // service all jobs
        for ( int job = 0; job < _JS_NJOBS; ++job )
        {
            // two cases, job active or not
            if ( jobMsg[job] )
            {
                // check if DMA is idle
                if ( getJobDMAStatus( job ) )
                    continue;

                const int id = jobMsg[job] & 0xff;
                if ( jobMsg[job] & _JS_ASYNCCOPY_NOTIFY_MEMORY )
                {
                    const unsigned long long notifyAddress = entry[id].notifyAddress;

                    // match alignment
                    //  Source and destination DMA addresses must match in
                    //  the 4 LSBs.  The notification value is moved to a
                    //  position that matches the notification address.
                    static int notify[4] __attribute__(( aligned( 16 ) ) );
                    const int index = ( notifyAddress & 0xf ) >> 2;

                    // select DMA tag
                    //  We must wait for any previous notification at this
                    //  index to be sent.
                    const int tag = index + _JS_NJOBS * _JS_NBUFFERS;
                    spu_writech( MFC_WrTagMask, 1 << tag );
                    spu_mfcstat( 2 );	// 0 polls, 1 waits for next, 2 waits for all

                    // DMA notification
                    notify[index] = entry[id].notifyValue;
                    spu_mfcdma64(
                        &notify[index],
                        notifyAddress >> 32,
                        notifyAddress & 0xffffffffULL,
                        sizeof( int ),
                        tag,
                        0x20 );
                }

                if ( jobMsg[job] & _JS_ASYNCCOPY_NOTIFY_MAILBOX )
                {
                    // mailbox notification
                    spu_writech( SPU_WrOutMbox, entry[id].notifyValue );
                }

                entry[id].size = 0;
                jobMsg[job] = 0;
            }
            else // job not active
            {
                // poll for new job
                //  The PU puts a command index in the mailbox.  The index
                //  is in the low byte, while options are passed in the
                //  upper bytes.
                if ( spu_readchcnt( SPU_RdInMbox ) == 0 )
                    continue;
                jobMsg[job] = spu_readch( SPU_RdInMbox ) | _JS_ASYNCCOPY_ACTIVE;
                const int id = jobMsg[job] & 0xff;

                // fence is actually a barrier
                if ( jobMsg[job] & _JS_ASYNCCOPY_BARRIER )
                {
                    // wait for all DMA
                    const unsigned int mask = ( 1 << ( _JS_NJOBS * _JS_NBUFFERS ) ) - 1;
                    spu_writech( MFC_WrTagMask, mask );
                    spu_mfcstat( 2 );
                }

                // queue DMA commands
                unsigned int bufferId = 0;
                unsigned long long src = entry[id].src;
                unsigned long long dst = entry[id].dst;
                unsigned int size = entry[id].size;
                while ( size )
                {
                    const int bufSize = size < _JS_BSIZE ? size : _JS_BSIZE;
                    const int tag = getBufferTag( job, bufferId );

                    // read
                    if ( jobMsg[job] & _JS_ASYNCCOPY_GPU_SRC )
                    {
                        // transfer 16 bytes at a time
                        for ( int offset = 0; offset < bufSize; offset += 16 )
                        {
                            spu_mfcdma64(
                                buffer[job][bufferId] + offset,
                                src >> 32,
                                ( src & 0xffffffffULL ),
                                16,
                                tag,
                                0x42 );	// read with fence
                            src += 16;
                        }
                    }
                    else
                    {
                        spu_mfcdma64(
                            buffer[job][bufferId],
                            src >> 32,
                            src & 0xffffffffULL,
                            bufSize,
                            tag,
                            0x42 );	// read with fence
                        src += bufSize;
                    }

                    // write
                    if ( jobMsg[job] & _JS_ASYNCCOPY_GPU_DST )
                    {
                        // transfer in IOIF windows
                        unsigned int wid = 0;
                        void* writeSrc = buffer[job][bufferId];
                        unsigned int writeSize = bufSize;
                        unsigned int cmd = 0x21;	// write with barrier
                        while ( writeSize )
                        {
                            unsigned int windowSize = _JS_WSIZE;

                            // check if 64MB boundary would be crossed
                            if (( dst ^( dst + windowSize ) ) & 0x4000000 )
                                windowSize = _JS_WSIZE - ( dst & ( _JS_WSIZE - 1 ) );

                            // don't write past the end
                            if ( windowSize > writeSize )
                                windowSize = writeSize;

                            const unsigned long long writeDst = _jsMapGPUAddress( dst, wid );
                            spu_mfcdma64(
                                writeSrc,
                                writeDst >> 32,
                                writeDst & 0xffffffffULL,
                                windowSize,
                                tag,
                                cmd );
                            cmd = 0x20;	// only first DMA needs barrier

                            writeSrc += windowSize;
                            dst += windowSize;
                            writeSize -= windowSize;

                            wid = ( wid + 1 ) & ( _JS_NWINDOWS - 1 );
                        }
                    }
                    else
                    {
                        spu_mfcdma64(
                            buffer[job][bufferId],
                            dst >> 32,
                            dst & 0xffffffffULL,
                            bufSize,
                            tag,
                            0x22 );	// write with fence
                        dst += bufSize;
                    }

                    size -= bufSize;
                    bufferId = ( bufferId + 1 ) % _JS_NBUFFERS;
                }
            }
        } // job loop
    } // loop forever

    return 0;
}
