#pragma once

#include <stdio.h>
#include <string_view>

class BZipFile {
    void* _bzip = nullptr;
    FILE* _f = nullptr;

public:
    void openWrite(std::string_view path);
    void write(const void* buf, int size);
    void write(std::string_view s);
    void flush();
    ~BZipFile();
};
