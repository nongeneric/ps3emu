#include "BZipFile.h"

#include <bzlib.h>
#include <stdexcept>

void BZipFile::openWrite(std::string_view path) {
    _f = fopen(path.data(), "w");
    if (!_f)
        throw std::runtime_error("can't open file");
    int error;
    _bzip = BZ2_bzWriteOpen(&error, _f, 9, 0, 0);
    if (error != BZ_OK)
        throw std::runtime_error("can't bzip file");
}

void BZipFile::write(const void* buf, int size) {
    int error;
    BZ2_bzWrite(&error, _bzip, const_cast<void*>(buf), size);
    if (error != BZ_OK)
        throw std::runtime_error("bzip write error");
}

void BZipFile::write(std::string_view s) {
    write(s.data(), s.size());
}

void BZipFile::flush() {
    BZ2_bzflush(_bzip);
}

BZipFile::~BZipFile() {
    if (!_f || !_bzip)
        return;
    int error;
    BZ2_bzWriteClose(&error, _bzip, 0, nullptr, nullptr);
    fclose(_f);
}
