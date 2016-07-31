#include "fs.h"

#include "../utils.h"
#include "../Process.h"
#include "../IDMap.h"
#include "../ContentManager.h"
#include "../state.h"
#include <stdexcept>
#include "string.h"
#include <string>
#include <array>
#include <algorithm>
#include "../log.h"
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

struct contentManager;
using namespace boost::filesystem;
class contentManager;

CellFsErrno sys_fs_open(const char* path,
                        uint32_t flags,
                        big_int32_t* fd,
                        uint64_t mode,
                        const void* arg,
                        uint64_t size,
                        Process* proc) {
    return cellFsOpen(path, flags, fd, mode, 0, proc);
}

CellFsErrno sys_fs_lseek(int32_t fd,
                         int64_t offset,
                         int32_t whence,
                         big_uint64_t* pos) {
    return cellFsLseek(fd, offset, whence, pos);
}

CellFsErrno sys_fs_read(int32_t fd,
                        ps3_uintptr_t buf,
                        uint64_t nbytes,
                        big_uint64_t* nread,
                        MainMemory* mm) {
    return cellFsRead(fd, buf, nbytes, nread, mm);
}

CellFsErrno sys_fs_close(int32_t fd) {
    return cellFsClose(fd);
}

CellFsErrno toCellErrno(int err) {
    switch (err) {
        case EACCES: return CELL_FS_EACCES;
        case EFAULT: return CELL_FS_EFAULT;
        case ENAMETOOLONG: return CELL_FS_ENAMETOOLONG;
        case ENOENT: return CELL_FS_ENOENT;
        case ENOMEM: return CELL_FS_ENOMEM;
        case ENOTDIR: return CELL_FS_ENOTDIR;
    }
    return CELL_FS_SUCCEEDED;
}

namespace {
    struct DirInfo {
        DIR* dir;
        std::string path;
        bool operator==(DirInfo const& other) {
            return dir == other.dir;
        }
    };

    ThreadSafeIDMap<int32_t, FILE*, 20> fileMap;
    ThreadSafeIDMap<int32_t, std::shared_ptr<DirInfo>> dirMap;
}

void copy(CellFsStat& sb, struct stat& st) {
    sb.st_mode = st.st_mode;
    sb.st_uid = st.st_uid;
    sb.st_gid = st.st_gid;
    sb.cell_st_atime = st.st_atime;
    sb.cell_st_mtime = st.st_mtime;
    sb.cell_st_ctime = st.st_ctime;
    sb.st_size = st.st_size;
    sb.st_blksize = 512;
}

CellFsErrno cellFsStat(const char* path, CellFsStat* sb, Process* proc) {
    auto hostPath = g_state.content->toHost(path);
    LOG << ssnprintf("cellFsStat(%s (%s), ...)", path, hostPath);
    struct stat st;
    auto err = stat(hostPath.c_str(), &st);
    if (err)
        return toCellErrno(errno);
    copy(*sb, st);
    return err;
}

CellFsErrno cellFsFstat(int32_t fd, CellFsStat* sb, Process* proc) {
    LOG << ssnprintf("cellFsStat(%d, ...)", fd);
    struct stat st;
    auto err = fstat(fileno(fileMap.get(fd)), &st);
    if (err)
        return toCellErrno(errno);
    copy(*sb, st);
    return err;
}

#define CELL_FS_O_CREAT         000100
#define CELL_FS_O_EXCL          000200
#define CELL_FS_O_TRUNC         001000
#define CELL_FS_O_APPEND        002000
#define CELL_FS_O_ACCMODE       000003
#define CELL_FS_O_RDONLY 000000
#define CELL_FS_O_RDWR   000002
#define CELL_FS_O_WRONLY 000001
#define CELL_FS_O_MSELF         010000

FILE* openFile(const char* path, int flags) {
    assert((flags & CELL_FS_O_MSELF) == 0);
    FILE* f = nullptr;
    bool e = exists(path);
    bool c = flags & CELL_FS_O_CREAT;
    bool t = flags & CELL_FS_O_TRUNC;
    if (!e && !c)
        return nullptr;
    if (e && (flags & CELL_FS_O_EXCL))
        return nullptr;
    if (t || !e) {
        f = fopen(path, "w+");
    } else {
        f = fopen(path, "r+");
    }
    if (flags & CELL_FS_O_APPEND) {
        fseek(f, 0, SEEK_END);
    }
    return f;
}

CellFsErrno cellFsOpen(const char* path, int32_t flags, big_int32_t* fd, uint64_t, uint64_t, Process* proc) {
    auto hostPath = g_state.content->toHost(path);
    LOG << ssnprintf("cellFsOpen(%s (%s), %x, ...)", path, hostPath, flags);
    auto f = openFile(hostPath.c_str(), flags);
    if (!f) {
        return toCellErrno(errno);
    }
    *fd = fileMap.create(f);
    return CELL_OK;
}

#define CELL_FS_SEEK_SET 0
#define CELL_FS_SEEK_CUR 1
#define CELL_FS_SEEK_END 2

int toStdWhence(int cellWhence) {
    switch (cellWhence) {
        case CELL_FS_SEEK_SET: return SEEK_SET;
        case CELL_FS_SEEK_CUR: return SEEK_CUR;
        case CELL_FS_SEEK_END: return SEEK_END;
    }
    throw std::runtime_error("illegal seek");
}

CellFsErrno cellFsLseek(int32_t fd, int64_t offset, int32_t whence, big_uint64_t* pos) {
    LOG << ssnprintf("cellFsLseek(%d, %x, %d, ...)", fd, offset, whence);
    auto stdWhence = toStdWhence(whence);
    auto file = fileMap.get(fd);
    if (fseek(file, offset, stdWhence))
        return toCellErrno(errno);
    *pos = ftell(file);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsClose(int32_t fd) {
    LOG << ssnprintf("cellFsClose(%d)", fd);
    auto file = fileMap.get(fd);
    if (fclose(file))
        return toCellErrno(errno);
    fileMap.destroy(fd);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsRead(int32_t fd, ps3_uintptr_t buf, uint64_t nbytes, big_uint64_t* nread, MainMemory* mm) {
    std::vector<char> localBuf(nbytes);
    auto file = fileMap.get(fd);
    *nread = fread(&localBuf[0], 1, nbytes, file);
    mm->writeMemory(buf, &localBuf[0], *nread);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsWrite(int32_t fd, ps3_uintptr_t buf, uint64_t nbytes, big_uint64_t* nwrite, MainMemory* mm) {
    auto file = fileMap.get(fd);
    std::vector<char> localBuf(nbytes);
    mm->readMemory(buf, &localBuf[0], nbytes);
    *nwrite = fwrite(&localBuf[0], 1, nbytes, file);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsMkdir(const char* path, uint32_t mode, Process* proc) {
    LOG << ssnprintf("cellFsMkdir(%s, ...)", path);
    if (exists(path))
        return CELL_FS_EEXIST;
    auto res = create_directory(g_state.content->toHost(path));
    assert(res);
    (void)res;
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsGetFreeSize(const char* directory_path,
                              big_uint32_t* block_size,
                              big_uint64_t* free_block_count,
                              Process* proc)
{
    *block_size = 4096;
    auto host = g_state.content->toHost(directory_path);
    LOG << ssnprintf("cellFsGetFreeSize(%s (%s), ...)", 
                                          directory_path,
                                          host);
    auto s = space(host);
    *free_block_count = s.available / *block_size;
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsFsync(int32_t fd) {
    LOG << ssnprintf("cellFsFsync(%d, ...)", fd);
    auto file = fileMap.get(fd);
    syncfs(fileno(file));
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsUnlink(const char* path, Process* proc) {
    LOG << ssnprintf("cellFsUnlink(%s, ...)", path);
    remove(g_state.content->toHost(path));
    return CELL_FS_SUCCEEDED;
}

#define CELL_FS_TYPE_UNKNOWN   0
#define CELL_FS_TYPE_DIRECTORY 1
#define CELL_FS_TYPE_REGULAR   2
#define CELL_FS_TYPE_SYMLINK   3

CellFsErrno cellFsOpendir(const char* path, big_int32_t* fd, Process* proc) {
    LOG << ssnprintf("cellFsOpendir(%s, ...)", path);
    auto host = g_state.content->toHost(path);
    auto dir = opendir(host.c_str());
    if (!dir)
        return toCellErrno(errno);
    auto dirInfo = std::shared_ptr<DirInfo>(new DirInfo{dir, host});
    *fd = dirMap.create(dirInfo);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsReaddir(int32_t fd, CellFsDirent* dirent, big_uint64_t* nread) {
    LOG << ssnprintf("cellFsReaddir(%d, ...)", fd);
    auto info = dirMap.get(fd);
    auto entry = readdir(info->dir);
    if (entry) {
        dirent->d_type = entry->d_type == DT_REG ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
        dirent->d_namlen = strlen(entry->d_name);
        strcpy(dirent->d_name, entry->d_name);
        *nread = 1;
    } else {
        *nread = 0;
    }
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsClosedir(int32_t fd) {
    LOG << ssnprintf("cellFsClosedir(%d, ...)", fd);
    dirMap.destroy(fd);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsGetDirectoryEntries(int32_t fd, 
                                      CellFsDirectoryEntry* entries, 
                                      uint32_t entries_size, 
                                      uint32_t* data_count)
{
    LOG << ssnprintf("cellFsGetDirectoryEntries(%d, ...)", fd);
    auto info = dirMap.get(fd);
    auto entry = readdir(info->dir);
    if (entry) {
        entries->entry_name.d_type = 
            entry->d_type == DT_REG ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
        entries->entry_name.d_namlen = strlen(entry->d_name);
        strcpy(entries->entry_name.d_name, entry->d_name);
        
        struct stat st;
        auto err = stat((info->path + "/" + std::string(entry->d_name)).c_str(), &st);
        if (err)
            return toCellErrno(errno);
        copy(entries->attribute, st);
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            entries->attribute.st_size = 4096;
        } else if (entry->d_type != DT_REG) {
            entries->attribute.st_size = 0;
        }
        
        *data_count = 1;
    } else {
        *data_count = 0;
    }
    return CELL_FS_SUCCEEDED;
}
