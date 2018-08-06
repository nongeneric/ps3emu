#pragma once

#include <string_view>
#include <memory>

class PPUThread;
class SPUThread;
class BZipFile;

class TraceFile {
    std::unique_ptr<BZipFile> _file;
    int _traced = 0;
    void nextFlush();

public:
    TraceFile(std::string_view path);
    ~TraceFile();
    void append(SPUThread* th);
    void append(PPUThread* th);
    void append(std::string_view line);
};
