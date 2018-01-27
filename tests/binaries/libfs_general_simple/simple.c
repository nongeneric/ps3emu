/*
 *     SCE CONFIDENTIAL
 *     PlayStation(R)3 Programmer Tool Runtime Library 475.001
 *     Copyright (C) 2006 Sony Computer Entertainment Inc.
 *     All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include <sys/process.h>
#include <sys/paths.h>
#include <cell/cell_fs.h>
#include <cell/sysmodule.h>
#define USE_PRINT_STAT
#include "../../include/common.h"

SYS_PROCESS_PARAM(1001, 0x10000)
int
main(void)
{
    CellFsErrno err;
    int fd, dir;
    CellFsDirent dent;
    char w[100], r[100];
    uint64_t sw, pos, sr, rd;

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
     * test cellFsMkdir()
     */
    printf("cellFsMkdir in\n");
    err = cellFsMkdir(MOUNT_POINT"/sample", CELL_FS_DEFAULT_CREATE_MODE_1);
    printf("cellFsMkdir: ret = %d\n", err);
    API_ERROR(err);

    /*
     * test cellFsOpen()
     */
    printf("cellFsOpen in\n");
    err = cellFsOpen(MOUNT_POINT"/sample/My_TEST_File",
                     CELL_FS_O_RDWR|CELL_FS_O_CREAT, &fd, NULL, 0);
    printf("cellFsOpen: err = %d\n", err);
    API_ERROR(err);
    //printf("cellFsOpen: fd = %d\n", fd);

    /*
     * test cellFsStat(), cellFsFstat()
     */
    print_stat(MOUNT_POINT"/sample/My_TEST_File");
    print_fstat(fd);

    /*
     * test cellFsWrite()
     */
    memset(w, 0, 100);
    printf("cellFsWrite in - 1\n");
    strcpy(w, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    err = cellFsWrite(fd, (const void *)w, (uint64_t)100, &sw);
    printf("cellFsWrite: err = %d\n", err);
    API_ERROR(err);
    printf("cellFsWrite: nwrite = %llu\n", sw);

    /*
     * test cellFsLseek(), cellFsRead()
     */
    printf("cellFsLseek-1 SEEK_SET in\n");
    err = cellFsLseek(fd, 0, CELL_FS_SEEK_SET, &pos);
    printf("cellFsLseek: pos = %llu\n", pos);
    printf("cellFsLseek: err = %d\n", err);
    API_ERROR(err);

    printf("cellFsRead in\n");
    err = cellFsRead(fd, (void *)r, (uint64_t)100, &sr);
    printf("cellFsRead: err     = %d\n", err);
    API_ERROR(err);
    printf("cellFsRead: nread = %llu\n", sr);
    printf("cellFsRead: r     = %s\n", r);

    printf("cellFsLseek-2 SEEK_END in\n");
    err = cellFsLseek(fd, -100, CELL_FS_SEEK_END, &pos);
    printf("cellFsLseek-2: pos = %llu\n", pos);
    printf("cellFsLseek-2: err = %d\n", err);
    API_ERROR(err);

    printf("cellFsRead in\n");
    err = cellFsRead(fd, (void *)r, (uint64_t)100, &sr);
    printf("cellFsRead: err     = %d\n", err);
    API_ERROR(err);
    printf("cellFsRead: nread = %llu\n", sr);
    printf("cellFsRead: r         = %s\n", r);

    printf("cellFsLseek-3 SEEK_CUR in\n");
    err = cellFsLseek(fd, -100, CELL_FS_SEEK_CUR, &pos);
    printf("cellFsLseek-3: pos = %llu\n", pos);
    printf("cellFsLseek-3: err = %d\n", err);
    API_ERROR(err);

    printf("cellFsRead in\n");
    err = cellFsRead(fd, (void *)r, (uint64_t)100, &sr);
    printf("cellFsRead: err   = %d\n", err);
    API_ERROR(err);
    printf("cellFsRead: nread = %llu\n", sr);
    printf("cellFsRead: r     = %s\n", r);

    /*
     * test cellFsFtruncate()
     */
    print_fstat(fd);
    printf("cellFsFtruncate in\n");
    err = cellFsFtruncate(fd, 500L);
    printf("cellFsFtruncate: err = %d\n", err);
    API_ERROR(err);
    print_fstat(fd);

    /*
     * test cellFsClose()
     */
    printf("cellFsClose in\n");
    err = cellFsClose(fd);
    printf("cellFsClose: ret = %d\n", err);
    API_ERROR(err);

    /*
     * test cellFsTruncate()
     */
    print_stat(MOUNT_POINT"/sample/My_TEST_File");
    printf("cellFsTruncate in\n");
    err = cellFsTruncate(MOUNT_POINT"/sample/My_TEST_File", 1000L);
    printf("cellFsTruncate: err = %d\n", err);
    API_ERROR(err);
    print_stat(MOUNT_POINT"/sample/My_TEST_File");

    /*
     * test cellFsRename()
     */
    printf("cellFsRename in\n");
    err = cellFsRename(MOUNT_POINT"/sample/My_TEST_File",
                       MOUNT_POINT"/sample/My_TEST_File_Renamed");
    printf("cellFsRename: ret = %d\n", err);
    API_ERROR(err);

    /*
     * test cellFsOpenDir()
     */
    printf("cellFsOpendir in\n");
    err = cellFsOpendir(MOUNT_POINT"/sample", &dir);
    printf("cellFsOpendir: err = %d\n", err);
    API_ERROR(err);
    //printf("cellFsOpendir: dir = %d\n", dir);

    /*
     * test cellFsReaddir()
     */
    while (1) {
        printf("cellFsReaddir in\n");
        err = cellFsReaddir(dir, &dent, &rd);
        API_ERROR(err);
        if (rd != 0) {
            printf("cellFsReaddir: nread = %llu\n", rd);
            printf("cellFsReaddir: err   = %d\n", err);
            printf("dent.d_type          = %d\n", dent.d_type);
            printf("dent.d_name          = %s\n", dent.d_name);
        } else {
            printf("cellFsReaddir: out\n");
            break;
        }
    } 

    /*
     * test cellFsClosedir()
     */
    printf("cellFsClosedir in\n");
    err = cellFsClosedir(dir);
    printf("cellFsClosedir: ret = %d\n", err);
    API_ERROR(err);

    /*
     * test cellFsUnlink()
     */
    printf("cellFsUnlink in\n");
    err = cellFsUnlink(MOUNT_POINT"/sample/My_TEST_File_Renamed");
    printf("cellFsUnlink: ret = %d\n", err);
    API_ERROR(err);

    /*
     * test cellFsRmdir()
     */
    printf("cellFsRmdir in\n");
    err = cellFsRmdir(MOUNT_POINT"/sample");
    printf("cellFsRmdir: ret = %d\n", err);
    API_ERROR(err);

    sys_process_exit(0);
}

