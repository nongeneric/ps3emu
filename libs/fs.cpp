#include "fs.h"

#include "../ps3emu/utils.h"
#include "../ps3emu/Process.h"
#include <stdexcept>
#include "string.h"
#include <string>
#include <array>
#include <algorithm>
#include <boost/log/trivial.hpp>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

using namespace boost::filesystem;

CellFsErrno sys_fs_open(const char* path,
                        uint32_t flags,
                        big_uint32_t* fd,
                        uint64_t mode,
                        const void* arg,
                        uint64_t size)
{
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_fs_open(%s, ...)", path);
    return 1;
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

template <typename T>
class FileMap {
    std::array<T, 255 - 3> _files;
    boost::mutex _m;
public:
    FileMap() {
        for (auto& f : _files)
            f = T();
    }
    
    int addFile(T file) {
        boost::unique_lock<boost::mutex> lock(_m);
        auto it = std::find(begin(_files), end(_files), T());
        if (it == end(_files))
            return -1;
        *it = file;
        return std::distance(begin(_files), it) + 3;
    }

    T& getFile(int descriptor) {
        boost::unique_lock<boost::mutex> lock(_m);
        assert(3 <= descriptor && descriptor <= 255);
        return _files[descriptor - 3];
    }

    void removeFile(T file) {
        boost::unique_lock<boost::mutex> lock(_m);
        auto it = std::find(begin(_files), end(_files), T());
        assert(it != end(_files));
        *it = T();
    }
};

struct DirInfo {
    DIR* dir;
    std::string path;
    bool operator==(DirInfo const& other) {
        return dir == other.dir;
    }
};

namespace {
    FileMap<FILE*> fileMap;
    FileMap<DirInfo> dirMap;
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
    auto hostPath = proc->contentManager()->toHost(path);
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsStat(%s (%s), ...)", path, hostPath);
    struct stat st;
    auto err = stat(hostPath.c_str(), &st);
    if (err)
        return toCellErrno(errno);
    copy(*sb, st);
    return err;
}

CellFsErrno cellFsFstat(int32_t fd, CellFsStat* sb, Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsStat(%d, ...)", fd);
    struct stat st;
    auto err = fstat(fileno(fileMap.getFile(fd)), &st);
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
    auto hostPath = proc->contentManager()->toHost(path);
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsOpen(%s (%s), %x, ...)", path, hostPath, flags);
    auto f = openFile(hostPath.c_str(), flags);
    if (!f) {
        return toCellErrno(errno);
    }
    *fd = fileMap.addFile(f);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsLseek(%d, %x, %d, ...)", fd, offset, whence);
    auto stdWhence = toStdWhence(whence);
    auto file = fileMap.getFile(fd);
    *pos = ftell(file);
    if (fseek(file, offset, stdWhence))
        return toCellErrno(errno);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsClose(int32_t fd) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsClose(%d)", fd);
    auto file = fileMap.getFile(fd);
    if (fclose(file))
        return toCellErrno(errno);
    fileMap.removeFile(file);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsRead(int32_t fd, ps3_uintptr_t buf, uint64_t nbytes, big_uint64_t* nread, MainMemory* mm) {
    std::vector<char> localBuf(nbytes);
    auto file = fileMap.getFile(fd);
    *nread = fread(&localBuf[0], 1, nbytes, file);
    mm->writeMemory(buf, &localBuf[0], *nread);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsWrite(int32_t fd, ps3_uintptr_t buf, uint64_t nbytes, big_uint64_t* nwrite, MainMemory* mm) {
    auto file = fileMap.getFile(fd);
    std::vector<char> localBuf(nbytes);
    mm->readMemory(buf, &localBuf[0], nbytes);
    *nwrite = fwrite(&localBuf[0], 1, nbytes, file);
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsMkdir(const char* path, uint32_t mode, Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsMkdir(%s, ...)", path);
    if (exists(path))
        return CELL_FS_EEXIST;
    auto res = create_directory(proc->contentManager()->toHost(path));
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
    auto host = proc->contentManager()->toHost(directory_path);
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsGetFreeSize(%s (%s), ...)", 
                                          directory_path,
                                          host);
    auto s = space(host);
    *free_block_count = s.available / *block_size;
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsFsync(int32_t fd) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsFsync(%d, ...)", fd);
    auto file = fileMap.getFile(fd);
    syncfs(fileno(file));
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsUnlink(const char* path, Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsUnlink(%s, ...)", path);
    remove(proc->contentManager()->toHost(path));
    return CELL_FS_SUCCEEDED;
}

#define CELL_FS_TYPE_UNKNOWN   0
#define CELL_FS_TYPE_DIRECTORY 1
#define CELL_FS_TYPE_REGULAR   2
#define CELL_FS_TYPE_SYMLINK   3

CellFsErrno cellFsOpendir(const char* path, big_int32_t* fd, Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsOpendir(%s, ...)", path);
    auto host = proc->contentManager()->toHost(path);
    auto dir = opendir(host.c_str());
    if (!dir)
        return toCellErrno(errno);
    *fd = dirMap.addFile({ dir, host });
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsReaddir(int32_t fd, CellFsDirent* dirent, big_uint64_t* nread) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsReaddir(%d, ...)", fd);
    auto& info = dirMap.getFile(fd);
    auto entry = readdir(info.dir);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsClosedir(%d, ...)", fd);
    dirMap.removeFile(dirMap.getFile(fd));
    return CELL_FS_SUCCEEDED;
}

CellFsErrno cellFsGetDirectoryEntries(int32_t fd, 
                                      CellFsDirectoryEntry* entries, 
                                      uint32_t entries_size, 
                                      uint32_t* data_count)
{
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsGetDirectoryEntries(%d, ...)", fd);
    auto& info = dirMap.getFile(fd);
    auto entry = readdir(info.dir);
    if (entry) {
        entries->entry_name.d_type = 
            entry->d_type == DT_REG ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
        entries->entry_name.d_namlen = strlen(entry->d_name);
        strcpy(entries->entry_name.d_name, entry->d_name);
        
        struct stat st;
        auto err = stat((info.path + "/" + std::string(entry->d_name)).c_str(), &st);
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
