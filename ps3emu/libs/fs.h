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

CellFsErrno sys_fs_open(cstring_ptr_t path,
                        uint32_t flags,
                        big_int32_t *fd,
                        uint64_t mode,
                        uint32_t arg,
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
CellFsErrno sys_fs_fstat(int32_t fd, CellFsStat* sb);
CellFsErrno sys_fs_close(int32_t fd);
CellFsErrno sys_fs_fcntl(int32_t fd, uint32_t cmd, uint32_t data, uint32_t size);
CellFsErrno sys_fs_truncate(cstring_ptr_t path, uint64_t size);
CellFsErrno sys_fs_ftruncate(int32_t fd, uint64_t size);
CellFsErrno sys_fs_rename(cstring_ptr_t src, cstring_ptr_t dest);
CellFsErrno sys_fs_stat(cstring_ptr_t path, CellFsStat *sb);
CellFsErrno sys_fs_mkdir(cstring_ptr_t path, CellFsMode mode);
CellFsErrno sys_fs_opendir(cstring_ptr_t path, big_int32_t *fd);
CellFsErrno sys_fs_readdir(int32_t fd, CellFsDirent *dirent, big_uint64_t *nread);
CellFsErrno sys_fs_closedir(int32_t fd);
CellFsErrno sys_fs_unlink(cstring_ptr_t path);
CellFsErrno sys_fs_rmdir(cstring_ptr_t path);
CellFsErrno sys_fs_fget_block_size(int32_t fd,
                                   big_uint64_t* sector_size,
                                   big_uint64_t* block_size,
                                   big_uint64_t* unk1,
                                   big_uint32_t* unk2);
CellFsErrno sys_fs_get_block_size(cstring_ptr_t path,
                                  big_uint64_t* sector_size,
                                  big_uint64_t* block_size,
                                  big_uint64_t* unk1);
CellFsErrno sys_fs_lsn_lock(int32_t fd);
CellFsErrno sys_fs_lsn_unlock(int32_t fd);
CellFsErrno sys_fs_lsn_get_cda_size(int32_t fd, big_uint64_t* unk);
CellFsErrno sys_fs_fsync(int32_t fd);
CellFsErrno sys_fs_disk_free(cstring_ptr_t directory_path,
                             big_uint64_t* capacity,
                             big_uint64_t* free);

FILE* searchFileMap(int32_t fd);
