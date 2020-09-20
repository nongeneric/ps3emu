#include "TraceFile.h"

#include "ps3emu/utils/BZipFile.h"
#include "ps3emu/ppu/PPUThread.h"
#include "ps3emu/spu/SPUThread.h"
#include "ps3emu/spu/SPUDasm.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/state.h"
#include "ps3emu/MainMemory.h"

TraceFile::TraceFile(std::string_view path) {
    _file = std::make_unique<BZipFile>();
    _file->openWrite(path);
}

TraceFile::~TraceFile() { }

void TraceFile::nextFlush() {
    if ((_traced % 20000) == 0) {
        _file->flush();
    }
    _traced++;
}

void TraceFile::append(SPUThread* th) {
    auto cia = th->getNip();
    _file->write(sformat("pc:{:08x};", cia));
    for (auto i = 0u; i < 128; ++i) {
        auto v = th->r(i);
        _file->write(sformat(
            "r{:03}:{:08x}{:08x}{:08x}{:08x};", i, v.w<0>(), v.w<1>(), v.w<2>(), v.w<3>()));
    }
    auto instr = th->ptr(cia);
    std::string str;
    SPUDasm<DasmMode::Print>(instr, cia, &str);
    _file->write(sformat(" #{}\n", str));
    nextFlush();
}

void TraceFile::append(PPUThread* th) {
    auto cia = th->getNIP();
    uint32_t instr;
    g_state.mm->readMemory(cia, &instr, sizeof instr);
    std::string str;
    ppu_dasm<DasmMode::Print>(&instr, cia, &str);

    _file->write(sformat("pc:{:08x};", cia));
    for (auto i = 0u; i < 32; ++i) {
        auto r = th->getGPR(i);
        _file->write(
            sformat("r{}:{:08x}{:08x};", i, (uint32_t)(r >> 32), (uint32_t)r));
    }
    _file->write(
        sformat("r{}:{:08x}{:08x};", 32, 0, (uint32_t)th->getLR()));
    for (auto i = 0u; i < 32; ++i) {
        auto r = th->r(i);
        _file->write(sformat("v{}:{:08x}{:08x}{:08x}{:08x};",
                               i,
                               (uint32_t)r.w(0),
                               (uint32_t)r.w(1),
                               (uint32_t)r.w(2),
                               (uint32_t)r.w(3)));
    }
    _file->write(sformat(" #{}\n", str.c_str()));
    nextFlush();
}

void TraceFile::append(std::string_view line) {
    _file->write(line);
}
