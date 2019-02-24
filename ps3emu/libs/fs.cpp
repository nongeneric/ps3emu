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
#include <filesystem>
#include <boost/thread.hpp>

using namespace std::filesystem;
class contentManager;

namespace {
    struct DirInfo {
        DIR* dir;
        std::string path;
        bool operator==(DirInfo const& other) {
            return dir == other.dir;
        }
    };

    ThreadSafeIDMap<int32_t, FILE*, 20> fileMap;
    ThreadSafeIDMap<int32_t, std::shared_ptr<DirInfo>, 3> dirMap;
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
    bool e = exists(path);
    bool c = flags & CELL_FS_O_CREAT;
    bool t = flags & CELL_FS_O_TRUNC;
    bool rw = flags & CELL_FS_O_RDWR;
    if (!e && !c)
        return nullptr;
    if (e && (flags & CELL_FS_O_EXCL))
        return nullptr;
    auto mode = t || !e ? (rw ? "w+" : "w") : (rw ? "r+" : "r");
    auto f = fopen(path, mode);
    if (!f) {
        ERROR(libs) << ssnprintf("openFile failed: %s", strerror(errno));
        exit(1);
    }
    if (flags & CELL_FS_O_APPEND) {
        fseek(f, 0, SEEK_END);
    }
    return f;
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

CellFsErrno sys_fs_open(cstring_ptr_t path,
                        uint32_t flags,
                        big_int32_t* fd,
                        uint64_t mode,
                        uint32_t arg,
                        uint64_t size) {
    std::filesystem::path hostPath = g_state.content->toHost(path.str);
    if (hostPath.extension() == ".sdat")
        hostPath += ".decrypted";
    auto f = openFile(hostPath.c_str(), flags);
    if (f) {
        *fd = fileMap.create(f);
        INFO(libs) << ssnprintf("sys_fs_open(%s (%s), %x, ...) %d", path.str, hostPath.string(), flags, (uint32_t)*fd);
        return CELL_OK;
    }
    INFO(libs) << ssnprintf("sys_fs_open(%s (%s), %x, ...) FAILED", path.str, hostPath.string(), flags);
    return toCellErrno(errno);
}

CellFsErrno sys_fs_lseek(int32_t fd,
                         int64_t offset,
                         int32_t whence,
                         big_uint64_t* pos) {
    INFO(libs) << ssnprintf("sys_fs_lseek(%d, %x, %d, ...)", fd, offset, whence);
    auto stdWhence = toStdWhence(whence);
    auto file = fileMap.get(fd);
    if (fseek(file, offset, stdWhence))
        return toCellErrno(errno);
    *pos = ftell(file);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_read(int32_t fd,
                        ps3_uintptr_t buf,
                        uint64_t nbytes,
                        big_uint64_t* nread) {
    std::vector<char> localBuf(nbytes);
    auto file = fileMap.get(fd);
    auto bytesRead = fread(&localBuf[0], 1, nbytes, file);
    if (nread) {
         *nread = bytesRead;
    }
    g_state.mm->writeMemory(buf, &localBuf[0], bytesRead);
    INFO(libs) << ssnprintf("sys_fs_read(%x, %x, %d) : %d", fd, buf, nbytes, bytesRead);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_write(int32_t fd,
                        ps3_uintptr_t buf,
                        uint64_t nbytes,
                        big_uint64_t* nwrite) {
    auto file = fileMap.get(fd);
    std::vector<char> localBuf(nbytes);
    g_state.mm->readMemory(buf, &localBuf[0], nbytes);
    auto bytesWritten = fwrite(&localBuf[0], 1, nbytes, file);
    if (nwrite) {
        *nwrite = bytesWritten;
    }
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_close(int32_t fd) {
    INFO(libs) << ssnprintf("sys_fs_close(%d)", fd);
    auto file = fileMap.get(fd);
    if (fclose(file))
        return toCellErrno(errno);
    fileMap.destroy(fd);
    return CELL_FS_SUCCEEDED;
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

CellFsErrno sys_fs_fstat(int32_t fd, CellFsStat* sb) {
    INFO(libs) << ssnprintf("sys_fs_fstat(%d, ...)", fd);
    struct stat st;
    auto err = fstat(fileno(fileMap.get(fd)), &st);
    if (err) {
        auto cellError = toCellErrno(errno);
        WARNING(libs) << ssnprintf("err: %d", cellError);
        return cellError;
    }
    copy(*sb, st);
    return err;
}

CellFsErrno sys_fs_disk_free(cstring_ptr_t directory_path,
                             big_uint64_t* capacity,
                             big_uint64_t* free) {
    auto host = path(g_state.content->toHost(directory_path.str));
    INFO(libs) << ssnprintf(
        "sys_fs_disk_free(%s (%s), ...)", directory_path.str, host.string());
    while (!exists(host)) {
        host = host.parent_path();
    }
    auto s = space(host);
    *capacity = s.capacity;
    *free = s.available;
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_fsync(int32_t fd) {
    INFO(libs) << ssnprintf("sys_fs_fsync(%d, ...)", fd);
    auto file = fileMap.get(fd);
    syncfs(fileno(file));
    return CELL_FS_SUCCEEDED;
}

#define CELL_FS_TYPE_UNKNOWN   0
#define CELL_FS_TYPE_DIRECTORY 1
#define CELL_FS_TYPE_REGULAR   2
#define CELL_FS_TYPE_SYMLINK   3

CellFsErrno getDirectoryEntries(int32_t fd,
                                CellFsDirectoryEntry* entries,
                                uint32_t entries_size,
                                uint32_t* data_count)
{
    INFO(libs) << ssnprintf("getDirectoryEntries(%d, ...)", fd);
    auto info = dirMap.get(fd);
    auto entry = readdir(info->dir);
    if (entry) {
        entries->entry_name.d_type = 
            entry->d_type == DT_REG ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
        entries->entry_name.d_namlen = strlen(entry->d_name);
        strcpy(entries->entry_name.d_name, entry->d_name);
        
        struct stat st;
        auto err = stat((info->path + "/" + std::string(entry->d_name)).c_str(), &st);
        if (err) {
            auto cellError = toCellErrno(errno);
            WARNING(libs) << ssnprintf("err: %d", cellError);
            return cellError;
        }
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

FILE* searchFileMap(int32_t fd) {
    return fileMap.get(fd);
}

struct fcntl_direntries_t {
    big_uint32_t zero;
    big_uint32_t written;
    big_uint32_t entries;
    big_uint32_t capacity;
};

struct fcntl_free_space_t {
    big_uint64_t unk0;
    big_uint64_t unk1;
    big_uint64_t unk3;
    big_uint32_t unk4;
    big_uint32_t blocksize;
    big_uint64_t freespace;
};

CellFsErrno sys_fs_fcntl(int32_t fd, uint32_t cmd, uint32_t data, uint32_t size) {
    if (fd == -1 && cmd == 0xe0000017) {
        g_state.mm->store32(data + 0x20, 0, g_state.granule);
        return CELL_OK;
    }
    if (fd != -1 && cmd == 0xc0000008) {
        g_state.mm->store32(data + 0x20, 0, g_state.granule);
        return CELL_OK;
    }
    if (cmd == 0xe0000012) {
        fcntl_direntries_t config;
        g_state.mm->readMemory(data, &config, size);
        EMU_ASSERT(config.zero == 0);
        EMU_ASSERT(size == sizeof(fcntl_direntries_t));
        uint32_t written;
        auto entries = (CellFsDirectoryEntry*)g_state.mm->getMemoryPointer(
            config.entries, config.capacity * sizeof(CellFsDirectoryEntry));
        auto res = getDirectoryEntries(fd, entries, config.capacity, &written);
        config.written = written;
        g_state.mm->writeMemory(data, &config, size);
        return res;
    }
    if (cmd == 0xc0000002) {
        fcntl_free_space_t config;
        EMU_ASSERT(size == sizeof(fcntl_free_space_t));
        g_state.mm->readMemory(data, &config, size);
        big_uint64_t capacity, free;
        sys_fs_disk_free({"/dev_hdd0/../"}, &capacity, &free);
        config.blocksize = 0x200;
        config.freespace = free / config.blocksize;
        g_state.mm->writeMemory(data, &config, size);
        return CELL_OK;
    }
    WARNING(libs) << ssnprintf("unknown sys_fs_fcntl(%x, %x, %x, %x)", fd, cmd, data, size);
    return CELL_OK;
}

CellFsErrno sys_fs_truncate(cstring_ptr_t path, uint64_t size) {
    auto hostPath = g_state.content->toHost(path.str);
    if (truncate(hostPath.c_str(), size))
        return toCellErrno(errno);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_rename(cstring_ptr_t src, cstring_ptr_t dest) {
    auto hostSrc = g_state.content->toHost(src.str);
    auto hostDest = g_state.content->toHost(dest.str.c_str());
    if (rename(hostSrc.c_str(), hostDest.c_str()))
        return toCellErrno(errno);
    return CELL_OK;
}

CellFsErrno sys_fs_stat(cstring_ptr_t path, CellFsStat *sb) {
    auto hostPath = g_state.content->toHost(path.str);
    INFO(libs) << ssnprintf("sys_fs_stat(%s (%s), ...)", path.str, hostPath);
    struct stat st;
    auto err = stat(hostPath.c_str(), &st);
    if (err) {
        auto cellError = toCellErrno(errno);
        WARNING(libs) << ssnprintf("err: %d", cellError);
        return cellError;
    }
    copy(*sb, st);
    return err;
}

CellFsErrno sys_fs_mkdir(cstring_ptr_t path, CellFsMode mode) {
    INFO(libs) << ssnprintf("sys_fs_mkdir(%s, ...)", path.str);
    auto hostPath = g_state.content->toHost(path.str);
    if (exists(hostPath))
        return CELL_FS_EEXIST;
    auto res = create_directory(hostPath);
    assert(res);
    (void)res;
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_ftruncate(int32_t fd, uint64_t size) {
    auto file = fileMap.get(fd);
    if (ftruncate(fileno(file), size))
        return toCellErrno(errno);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_opendir(cstring_ptr_t path, big_int32_t *fd) {
    auto host = g_state.content->toHost(path.str);
    INFO(libs) << ssnprintf("sys_fs_opendir(%s, ...) %s", path.str, host);
    auto dir = opendir(host.c_str());
    if (!dir) {
        auto cellError = toCellErrno(errno);
        WARNING(libs) << ssnprintf("err: %d", cellError);
        return cellError;
    }
    auto dirInfo = std::shared_ptr<DirInfo>(new DirInfo{dir, host});
    *fd = dirMap.create(dirInfo);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_readdir(int32_t fd, CellFsDirent *dirent, big_uint64_t *nread) {
    INFO(libs) << ssnprintf("sys_fs_readdir(%d, ...)", fd);
    auto info = dirMap.get(fd);
    auto entry = readdir(info->dir);
    if (entry) {
        dirent->d_type = entry->d_type == DT_REG ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
        dirent->d_namlen = strlen(entry->d_name);
        strcpy(dirent->d_name, entry->d_name);
        *nread = sizeof(CellFsDirent);
    } else {
        *nread = 0;
    }
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_closedir(int32_t fd) {
    INFO(libs) << ssnprintf("sys_fs_closedir(%d, ...)", fd);
    dirMap.destroy(fd);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_unlink(cstring_ptr_t path) {
    INFO(libs) << ssnprintf("sys_fs_unlink(%s, ...)", path.str);
    remove(g_state.content->toHost(path.str));
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_rmdir(cstring_ptr_t path) {
    INFO(libs) << ssnprintf("sys_fs_rmdir(%s, ...)", path.str);
    remove(g_state.content->toHost(path.str));
    return CELL_FS_SUCCEEDED;
}

CellFsErrno sys_fs_fget_block_size(int32_t fd,
                                   big_uint64_t* sector_size,
                                   big_uint64_t* block_size,
                                   big_uint64_t* unk1,
                                   big_uint32_t* unk2)
{
    *sector_size = 512;
    *block_size = 512;
    *unk1 = 0;
    *unk2 = 0x42;
    return CELL_OK;
}

CellFsErrno sys_fs_get_block_size(cstring_ptr_t path,
                                  big_uint64_t* sector_size,
                                  big_uint64_t* block_size,
                                  big_uint64_t* unk1)
{
    big_uint32_t unk2;
    return sys_fs_fget_block_size(0, sector_size, block_size, unk1, &unk2);
}

CellFsErrno sys_fs_lsn_lock(int32_t fd) {
    return CELL_OK;
}

CellFsErrno sys_fs_lsn_unlock(int32_t fd) {
    return CELL_OK;
}

CellFsErrno sys_fs_lsn_get_cda_size(int32_t fd, big_uint64_t* unk) {
    *unk = 0xffffffff00000000ull;
    return CELL_FS_EFSSPECIFIC;
}
