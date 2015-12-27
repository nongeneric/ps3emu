#pragma once

#include "sys_defs.h"
#include "../ps3emu/constants.h"

typedef big_int32_t CellFsMode;
typedef int32_t CellFsErrno;

constexpr CellFsErrno CELL_FS_OK = CELL_OK;
constexpr CellFsErrno CELL_FS_SUCCEEDED = CELL_FS_OK;
constexpr CellFsErrno CELL_FS_ENOTMOUNTED = -2147418054;
constexpr CellFsErrno CELL_FS_ENOENT = -2147418106;
constexpr CellFsErrno CELL_FS_EIO = -2147418069;
constexpr CellFsErrno CELL_FS_ENOMEM = -2147418108;
constexpr CellFsErrno CELL_FS_ENOTDIR = -2147418066;
constexpr CellFsErrno CELL_FS_ENAMETOOLONG = -2147418060;
constexpr CellFsErrno CELL_FS_EFSSPECIFIC = -2147418056;
constexpr CellFsErrno CELL_FS_EFAULT = -2147418099;
constexpr CellFsErrno CELL_FS_EACCES = -2147418071;

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

class Process;
class PPUThread;
CellFsErrno sys_fs_open_impl(const char *path,
                             uint32_t flags,
                             big_uint32_t *fd,
                             uint64_t mode,
                             const void *arg,
                             uint64_t size);
CellFsErrno cellFsStat(const char* path, CellFsStat* sb, Process* proc);
CellFsErrno cellFsOpen(const char *path,
                       int32_t flags,
                       big_int32_t *fd,
                       uint64_t arg,
                       uint64_t size,
                       Process* proc);
CellFsErrno cellFsLseek(int32_t fd, int64_t offset, int32_t whence, big_uint64_t *pos);
CellFsErrno cellFsClose(int32_t fd);
CellFsErrno cellFsRead(int32_t fd, ps3_uintptr_t buf, uint64_t nbytes, big_uint64_t *nread, PPUThread* thread);
