#include "fs.h"

#include "../ps3emu/utils.h"
#include "../ps3emu/PPU.h"
#include "../ps3emu/ELFLoader.h"
#include <stdexcept>
#include "string.h"
#include <string>
#include <array>
#include <algorithm>
#include <boost/log/trivial.hpp>
#include <sys/stat.h>
#include <stdio.h>
#include <boost/filesystem.hpp>

enum class MountPoint {
    GameData,
    SystemCache,
    MemoryStick,
    Usb,
    Bluray,
    HostHome,
    HostAbsolute
};

MountPoint splitPathImpl(const char* path, const char** point, const char** relative) {
#define check(str, type) \
    if (memcmp(str, path, strlen(str)) == 0) { \
        *point = str; \
        *relative = path + strlen(str) + 1; \
        return type; \
    }
    check("/dev_hdd0", MountPoint::GameData);
    check("/dev_hdd1", MountPoint::SystemCache);
    check("/dev_ms", MountPoint::MemoryStick);
    check("/dev_usb", MountPoint::Usb);
    check("/dev_bdvd", MountPoint::Bluray);
    check("/app_home", MountPoint::HostHome);
    check("/host_root", MountPoint::HostAbsolute);
#undef check
    throw std::runtime_error("illegal mount point");
}

std::string toHostPath(const char* path, const char* exePath) {
    const char* point;
    const char* relative;
    auto type = splitPathImpl(path, &point, &relative);
    if (type == MountPoint::HostAbsolute) {
        relative += 3;
    }
    boost::filesystem::path exe(exePath);
    return absolute(exe.parent_path() / point / relative).string();
}

CellFsErrno sys_fs_open_impl(const char* path,
                             uint32_t flags,
                             big_uint32_t* fd,
                             uint64_t mode,
                             const void* arg,
                             uint64_t size)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
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

CellFsErrno cellFsStat(const char* path, CellFsStat* sb, PPU* ppu) {
    auto hostPath = toHostPath(path, ppu->getELFLoader()->loadedFilePath().c_str());
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsStat(%s (%s), ...)", path, hostPath);
    struct stat st;
    auto err = stat(hostPath.c_str(), &st);
    if (err != CELL_FS_SUCCEEDED)
        return err;
    sb->st_mode = st.st_mode;
    sb->st_uid = st.st_uid;
    sb->st_gid = st.st_gid;
    sb->cell_st_atime = st.st_atime;
    sb->cell_st_mtime = st.st_mtime;
    sb->cell_st_ctime = st.st_ctime;
    sb->st_size = st.st_size;
    sb->st_blksize = st.st_blksize;
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

const char* flagsToMode(int flags) {
    assert((flags & CELL_FS_O_MSELF) == 0);
    assert((flags & CELL_FS_O_EXCL) == 0);
    if (flags == CELL_FS_O_RDONLY)
        return "r";
    if (flags == CELL_FS_O_RDWR)
        return "r+";
    if (flags == CELL_FS_O_WRONLY)
        return "w";
    if (((flags & CELL_FS_O_WRONLY) || (flags & CELL_FS_O_RDWR)) && (flags & CELL_FS_O_APPEND))
        return "a+";
    throw std::runtime_error("unsupported mode");
}

class FileMap {
    std::array<FILE*, 255 - 3> _files;
public:
    FileMap() {
        for (auto& f : _files)
            f = nullptr;
    }
    
    int addFile(FILE* file) {
        auto it = std::find(begin(_files), end(_files), nullptr);
        if (it == end(_files))
            return -1;
        *it = file;
        return std::distance(begin(_files), it) + 3;
    }

    FILE* getFile(int descriptor) {
        assert(3 <= descriptor && descriptor <= 255);
        return _files[descriptor - 3];
    }

    void removeFile(FILE* file) {
        auto it = std::find(begin(_files), end(_files), nullptr);
        assert(it != end(_files));
        *it = nullptr;
    }
};

static FileMap fileMap;

CellFsErrno cellFsOpen(const char* path, int32_t flags, big_int32_t* fd, uint64_t, uint64_t, PPU* ppu) {
    const char* mode = flagsToMode(flags);
    auto hostPath = toHostPath(path, ppu->getELFLoader()->loadedFilePath().c_str());
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellFsOpen(%s (%s), %x, ...)", path, hostPath, flags);
    auto f = fopen(hostPath.c_str(), mode);
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

CellFsErrno cellFsRead(int32_t fd, ps3_uintptr_t buf, uint64_t nbytes, big_uint64_t* nread, PPU* ppu) {
    std::unique_ptr<char[]> localBuf(new char[nbytes]);
    auto file = fileMap.getFile(fd);
    *nread = fread(localBuf.get(), 1, nbytes, file);
    ppu->writeMemory(buf, localBuf.get(), *nread);
    return CELL_FS_SUCCEEDED;
}
