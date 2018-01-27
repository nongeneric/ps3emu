/*
 *     SCE CONFIDENTIAL
 *     PlayStation(R)3 Programmer Tool Runtime Library 475.001
 *     Copyright (C) 2006 Sony Computer Entertainment Inc.
 *     All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/process.h>
#include <sys/paths.h>
#include <cell/cell_fs.h>
#include <cell/sysmodule.h>
#include "../../include/common.h"

static int error_flag = 0;

#define API_ERRORX(x) {if(x!=CELL_FS_SUCCEEDED){printf("error=0x%x\n",x);error_flag=1;goto cleanup;}}

#define TEST_OFFSET     (13)
#define TEST_SIZE       (1024*1024*2+3)
#define TEST_FILE_SIZE  (1024*1024*3)
#define TEST_WAIT       (1024*4)
#define TEST_BLOCK_SIZE (1024*4)
#define TEST_BUF_SIZE   (1024*4*3)

static int xxx = 0;
static void xxx_callback(int fd, uint64_t size)
{
    //printf("Callback is called !! fd = ??, size = %llu\n", size);
    xxx = 1;
}

SYS_PROCESS_PARAM(1001, 0x10000)
int
main(void)
{
    CellFsErrno err;
    int fd;
    char *buf = NULL;
    uint64_t pos;
    uint64_t sector_size;
    uint64_t block_size;
    CellFsRingBuffer ringbuf;
    uint64_t rsize;
    uint64_t read;
    uint64_t wsize;

    /* 
     * loading prx for libfs
     */ 
    int ret;
    ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
    if (ret) {
        printf("cellSysmoduleLoadModule() error 0x%x !\n", ret);
        sys_process_exit(1);
    }

    if (!isMounted(MOUNT_POINT)) {
        sys_process_exit(1);
    }

    /*
     * cellFsMkdir()
     */
    printf("cellFsMkdir in\n");
    err = cellFsMkdir(MOUNT_POINT"/sample", CELL_FS_DEFAULT_CREATE_MODE_1);
    printf("cellFsMkdir: ret = %d\n", err);
    API_ERROR(err);

    /*
     * cellFsOpen()
     */
    printf("cellFsOpen in\n");
    err = cellFsOpen(MOUNT_POINT"/sample/Data0",
                     CELL_FS_O_RDWR|CELL_FS_O_CREAT, &fd, NULL, 0);
    printf("cellFsOpen: err = %d\n", err);
    API_ERROR(err);
    //printf("cellFsOpen: fd = %d\n", fd);

    /*
     * cellFsFtruncate()
     */
    print_fstat(fd);
    printf("cellFsFtruncate in\n");
    err = cellFsFtruncate(fd, TEST_FILE_SIZE);
    printf("cellFsFtruncate: err = %d\n", err);
    API_ERROR(err);
    print_fstat(fd);

    /*
     * cellFsFGetBlockSize()
     */
    printf("cellFsFGetBlockSize in\n");
    err = cellFsFGetBlockSize(fd, &sector_size, &block_size);
    printf("cellFsFGetBlockSize: ret = %d\n", err);
    API_ERROR(err);
    printf("cellFsFGetBlockSize: sector_size = %llu\n", sector_size);
    printf("cellFsFGetBlockSize: block_size  = %llu\n", block_size);

    buf = (char *)malloc(TEST_SIZE);
    if (!buf) {
        API_ERRORX(CELL_FS_ENOMEM);
    }

    /* COPY_TEST */
    {
    /*
     * prepare CellFsRingBuffer [for copy mode]
     */
    ringbuf.ringbuf_size  = TEST_BUF_SIZE;
    ringbuf.block_size    = TEST_BLOCK_SIZE;
    ringbuf.copy          = CELL_FS_ST_COPY;
    ringbuf.transfer_rate = 20; // dummy

    printf("***** COPY MODE TEST START *****\n");

    /*
     * prepare data in file
     */
    memset((char *)buf, 'P', TEST_SIZE);
    err = cellFsLseek(fd, TEST_OFFSET, CELL_FS_SEEK_SET, &pos);
    API_ERRORX(err);
    err = cellFsWrite(fd, buf, TEST_SIZE, &wsize);
    if ((err != CELL_FS_SUCCEEDED) || (wsize != TEST_SIZE)) {
        API_ERRORX(err);
    }

    /*
     * Test copy-mode stream read(and verify)
     */
    read = 0;
    printf("cellFsStReadInit in\n");
    err = cellFsStReadInit(fd, &ringbuf);
    printf("cellFsStReadInit: ret = %d\n", err);
    API_ERRORX(err);
    printf("cellFsStReadStart in\n");
    err = cellFsStReadStart(fd, TEST_OFFSET, TEST_SIZE);
    printf("cellFsStReadStart: ret = %d\n", err);
    API_ERRORX(err);
    while (1) {
        memset(buf, 'Z', TEST_WAIT); // reset to Z
        //printf("cellFsStRead in\n");
        err = cellFsStRead(fd, buf, TEST_WAIT, &rsize);
        //printf("cellFsStRead: ret     = %d\n", err);
        if ((err != (int)CELL_FS_SUCCEEDED) && (err != (int)CELL_FS_ERANGE)) {
            API_ERRORX(err);
        }
        for (int i = 0; i < (int)rsize; i++) {
            if (buf[i] != 'P') {
                printf("cellFsStRead Verify Error rsize = %llu\n", rsize);
                printf("cellFsStRead Verify Error buf[%d] = 0x%x\n", i, buf[i]);
                goto cleanup;
            }
        }
        read+=rsize;
        if (err == (int)CELL_FS_ERANGE) {
            printf ("cellFsStRead reached ERANGE !!\n");
            break;
        }
    }
    printf("cellFsStReadStop in\n");
    err = cellFsStReadStop(fd);
    printf("cellFsStReadStop: ret = %d\n", err);
    API_ERRORX(err);
    printf("cellFsStReadFinish in\n");
    err = cellFsStReadFinish(fd);
    printf("cellFsStReadFinish: ret = %d\n", err);
    API_ERRORX(err);
    printf("read = %llu\n", read);

    printf("***** COPY MODE TEST DONE *****\n");
    }

    char *addr;
    /* COPYLESS_TEST */
    {
    /*
     * prepare CellFsRingBuffer [for copyless mode]
     */
    ringbuf.ringbuf_size  = TEST_BUF_SIZE;
    ringbuf.block_size    = TEST_BLOCK_SIZE;
    ringbuf.copy          = CELL_FS_ST_COPYLESS;
    ringbuf.transfer_rate = 20; // dummy

    printf("***** COPYLESS MODE TEST START *****\n");

    /*
     * prepare data in file
     */
    memset((char *)buf, 'X', TEST_SIZE);
    err = cellFsLseek(fd, TEST_OFFSET, CELL_FS_SEEK_SET, &pos);
    API_ERRORX(err);
    err = cellFsWrite(fd, buf, TEST_SIZE, &wsize);
    if ((err != CELL_FS_SUCCEEDED) || (wsize != TEST_SIZE)) {
        API_ERRORX(err);
    }

    /*
     * Test copyless-mode stream read(and verify)
     */
    read = 0;
    printf("cellFsStReadInit in\n");
    err = cellFsStReadInit(fd, &ringbuf);
    printf("cellFsStReadInit: ret = %d\n", err);
    API_ERRORX(err);
    printf("cellFsStReadStart in\n");
    err = cellFsStReadStart(fd, TEST_OFFSET, TEST_SIZE);
    printf("cellFsStReadStart: ret = %d\n", err);
    API_ERRORX(err);
    while (1) {
        //printf("cellFsStReadWait in\n");
        err = cellFsStReadWait(fd, TEST_WAIT);
        //printf("cellFsStReadWait: ret = %d\n", err);
        API_ERRORX(err);
        //printf("cellFsStReadGetCurrentAddr in\n");
        err = cellFsStReadGetCurrentAddr(fd, &addr, &rsize);
        if ((err != (int)CELL_FS_SUCCEEDED) && (err != (int)CELL_FS_ERANGE)) {
            API_ERRORX(err);
        }
        if (err == (int)CELL_FS_ERANGE) {
            printf ("cellFsStReadGetCurrentAddr reached ERANGE !!\n");
            break;
        }
        for (int i = 0; i < (int)rsize; i++) {
            if (addr[i] != 'X') {
                printf("cellFsStReadGetCurrentAddr Verify Error addr[%d] = 0x%x\n",
                             i, addr[i]);
                goto cleanup;
            }
        }
        //printf("cellFsStReadPutCurrentAddr in\n");
        err = cellFsStReadPutCurrentAddr(fd, addr, rsize);
        //printf("cellFsStReadPutCurrentAddr: ret = %d\n", err);
        API_ERRORX(err);
        read+=rsize;
    }
    printf("cellFsStReadStop in\n");
    err = cellFsStReadStop(fd);
    printf("cellFsStReadStop: ret = %d\n", err);
    API_ERRORX(err);
    printf("cellFsStReadFinish in\n");
    err = cellFsStReadFinish(fd);
    printf("cellFsStReadFinish: ret = %d\n", err);
    API_ERRORX(err);
    printf("read = %llu\n", read);

    printf("***** COPYLESS MODE TEST DONE *****\n");
    }

    /* COPYLESS_TEST_WITH_CALLBACK */
    {
    /*
     * prepare CellFsRingBuffer [for copyless mode]
     */
    ringbuf.ringbuf_size  = TEST_BUF_SIZE;
    ringbuf.block_size    = TEST_BLOCK_SIZE;
    ringbuf.copy          = CELL_FS_ST_COPYLESS;
    ringbuf.transfer_rate = 20; // dummy

    printf("***** COPYLESS MODE TEST WITH CALLBACK START *****\n");

    /*
     * prepare data in file
     */
    memset((char *)buf, 'Z', TEST_SIZE);
    err = cellFsLseek(fd, TEST_OFFSET, CELL_FS_SEEK_SET, &pos);
    API_ERRORX(err);
    err = cellFsWrite(fd, buf, TEST_SIZE, &wsize);
    if ((err != CELL_FS_SUCCEEDED) || (wsize != TEST_SIZE)) {
        API_ERRORX(err);
    }

    /*
     * Test copyless-mode stream read(and verify)
     */
    read = 0;
    printf("cellFsStReadInit in\n");
    err = cellFsStReadInit(fd, &ringbuf);
    printf("cellFsStReadInit: ret = %d\n", err);
    API_ERRORX(err);
    printf("cellFsStReadStart in\n");
    err = cellFsStReadStart(fd, TEST_OFFSET, TEST_SIZE);
    printf("cellFsStReadStart: ret = %d\n", err);
    API_ERRORX(err);
    while (1) {
        //printf("cellFsStReadWaitCallback in\n");
        err = cellFsStReadWaitCallback(fd, TEST_WAIT, xxx_callback);
        //printf("cellFsStReadWaitCallback ret = 0x%x\n", err);
        API_ERRORX(err);
        while(!xxx) {
            sys_timer_usleep(10);
        }
        xxx = 0;
        //printf("cellFsStReadGetCurrentAddr in\n");
        err = cellFsStReadGetCurrentAddr(fd, &addr, &rsize);
        if ((err != (int)CELL_FS_SUCCEEDED) && (err != (int)CELL_FS_ERANGE)) {
            API_ERRORX(err);
        }
        if (err == (int)CELL_FS_ERANGE) {
            printf ("cellFsStReadGetCurrentAddr reached ERANGE !!\n");
            break;
        }
        for (int i = 0; i < (int)rsize; i++) {
            if (addr[i] != 'Z') {
                printf("cellFsStReadGetCurrentAddr Verify Error addr[%d] = 0x%x\n",
                             i, addr[i]);
                goto cleanup;
            }
        }
        //printf("cellFsStReadPutCurrentAddr in\n");
        err = cellFsStReadPutCurrentAddr(fd, addr, rsize);
        //printf("cellFsStReadPutCurrentAddr: ret = %d\n", err);
        API_ERRORX(err);
        read+=rsize;
    }
    printf("cellFsStReadStop in\n");
    err = cellFsStReadStop(fd);
    printf("cellFsStReadStop: ret = %d\n", err);
    API_ERRORX(err);
    printf("cellFsStReadFinish in\n");
    err = cellFsStReadFinish(fd);
    printf("cellFsStReadFinish: ret = %d\n", err);
    API_ERRORX(err);
    printf("read = %llu\n", read);

    printf("***** COPYLESS MODE WITH CALLBACK TEST DONE *****\n");
    }

cleanup:
    if (buf) free(buf);
    cellFsStReadStop(fd);
    cellFsStReadFinish(fd);
    /*
     * cellFsClose()
     */
    printf("cellFsClose in\n");
    err = cellFsClose(fd);
    printf("cellFsClose: ret = %d\n", err);
    API_ERROR(err);

    /*
     * cleanup
     */
    printf("cleanup:cellFsUnlink in\n");
    err = cellFsUnlink(MOUNT_POINT"/sample/Data0");
    printf("cleanup:cellFsUnlink: ret = %d\n", err);
    API_ERROR(err);
    printf("cleanup:cellFsRmdir in\n");
    err = cellFsRmdir(MOUNT_POINT"/sample");
    printf("cleanup:cellFsRmdir: ret = %d\n", err);
    API_ERROR(err);
    
    if (error_flag) {
        printf("stream failed\n");
    } else {
        printf("stream succeeded\n");
    }

    sys_process_exit(0);
}
