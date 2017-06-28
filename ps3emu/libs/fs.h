#pragma once

#include "sys_defs.h"
#include "../constants.h"
#include "../MainMemory.h"

typedef big_int32_t CellFsMode;
typedef int32_t CellFsErrno;

constexpr CellFsErrno CELL_FS_OK = CELL_OK;
constexpr CellFsErrno CELL_FS_SUCCEEDED = CELL_FS_OK;
constexpr CellFsErrno CELL_FS_ENOTMOUNTED = 0x8001003a;
constexpr CellFsErrno CELL_FS_ENOENT = 0x80010006;
constexpr CellFsErrno CELL_FS_EIO = 0x8001002b;
constexpr CellFsErrno CELL_FS_ENOMEM = 0x80010004;
constexpr CellFsErrno CELL_FS_ENOTDIR = 0x8001002e;
constexpr CellFsErrno CELL_FS_ENAMETOOLONG = 0x80010034;
constexpr CellFsErrno CELL_FS_EFSSPECIFIC = 0x80010038;
constexpr CellFsErrno CELL_FS_EFAULT = 0x8001000d;
constexpr CellFsErrno CELL_FS_EACCES = 0x80010029;
constexpr CellFsErrno CELL_FS_EEXIST = 0x80010014;

#define CELL_FS_MAX_FS_FILE_NAME_LENGTH (255)

struct CellFsStat {
    CellFsMode st_mode;
    big_int32_t st_uid;
    big_int32_t st_gid;
    cell_time_t cell_st_atime;
    cell_time_t cell_st_mtime;
    cell_time_t cell_st_ctime;
    big_uint64_t st_size;
    big_uint64_t st_blksize;
};

struct CellFsDirent {
    uint8_t d_type;
    uint8_t d_namlen;
    char d_name[CELL_FS_MAX_FS_FILE_NAME_LENGTH + 1];
};

struct CellFsDirectoryEntry {
    CellFsStat attribute;
    CellFsDirent entry_name;
};

CellFsErrno sys_fs_open(const char *path,
                        uint32_t flags,
                        big_int32_t *fd,
                        uint64_t mode,
                        const void *arg,
                        uint64_t size);
CellFsErrno sys_fs_lseek(int32_t fd, int64_t offset, int32_t whence, big_uint64_t *pos);
CellFsErrno sys_fs_read(int32_t fd,
                        ps3_uintptr_t buf,
                        uint64_t nbytes,
                        big_uint64_t* nread);
CellFsErrno sys_fs_write(int32_t fd,
                        ps3_uintptr_t buf,
                        uint64_t nbytes,
                        big_uint64_t* nwrite);
CellFsErrno sys_fs_fstat(int32_t fd, CellFsStat* sb, Process* proc);
CellFsErrno sys_fs_close(int32_t fd);
CellFsErrno cellFsStat(const char* path, CellFsStat* sb, Process* proc);
CellFsErrno cellFsFstat(int32_t fd, CellFsStat* sb, Process* proc);
CellFsErrno cellFsOpen(cstring_ptr_t path,
                       int32_t flags,
                       big_int32_t *fd,
                       uint64_t arg,
                       uint64_t size);
CellFsErrno cellFsSdataOpen(cstring_ptr_t path,
                       int32_t flags,
                       big_int32_t *fd,
                       uint64_t arg,
                       uint64_t size);
CellFsErrno cellFsLseek(int32_t fd, int64_t offset, int32_t whence, big_uint64_t *pos);
CellFsErrno cellFsClose(int32_t fd);
CellFsErrno cellFsRead(int32_t fd, ps3_uintptr_t buf, uint64_t nbytes, big_uint64_t *nread, MainMemory* mm);
CellFsErrno cellFsWrite(int32_t fd, ps3_uintptr_t buf, uint64_t nbytes, big_uint64_t *nwrite, MainMemory* mm);
CellFsErrno cellFsMkdir(const char* path, uint32_t mode, Process* proc);
CellFsErrno cellFsGetFreeSize(const char* directory_path,
                              big_uint32_t* block_size,
                              big_uint64_t* free_block_count,
                              Process* proc);
CellFsErrno cellFsFsync(int32_t fd);
CellFsErrno cellFsUnlink(const char* path, Process* proc);
CellFsErrno cellFsOpendir(const char *path, big_int32_t *fd, Process* proc);
CellFsErrno cellFsReaddir(int32_t fd, CellFsDirent *dir, big_uint64_t *nread);
CellFsErrno cellFsClosedir(int32_t fd);
CellFsErrno cellFsGetDirectoryEntries(int32_t fd,
                                      CellFsDirectoryEntry *entries,
                                      uint32_t entries_size,
                                      uint32_t *data_count);
FILE* searchFileMap(int32_t fd);
