#pragma once

#include <string>
#include <vector>
#include <stdint.h>

struct ShaderDasmCommand {
    std::string binary;
    bool naked;
    bool vertex;
};

struct UnsceCommand {
    std::string sce;
    std::string data;
    std::string elf;
};

struct RestoreElfCommand {
    std::string elf;
    std::string dump;
    std::string output;
};

struct ReadPrxCommand {
    std::string elf;
    bool prx;
    bool writeIdaScript;
};

struct ParseSpursTraceCommand {
    std::string dump;
};

struct RewriteCommand {
    std::string elf;
    std::string cpp;
    uint32_t imageBase;
    bool isSpu = false;
    bool noFexcept = false;
};

struct PrxStoreCommand {
    bool map;
    bool rewrite;
    bool verbose;
};

struct RsxDasmCommand {
    std::string bin;
};

struct FindSpuElfsCommand {
    std::string elf;
};

struct DumpInstrDbCommand { };

struct PrintGcmVizTraceCommand {
    std::string trace;
    int frame;
    int command;
};

struct TraceVizCommand {
    std::string good;
    std::string bad;
    std::string dump;
    uint32_t offset;
    bool spu;
};

struct DasmCommand {
    std::string elf;
};

struct UnpackTrpCommand {
    std::string trp;
    std::string output;
};

struct SplitLogCommand {
    std::string log;
    std::string output;
};

void HandleUnsce(UnsceCommand const& command);
void HandleShaderDasm(ShaderDasmCommand const& command);
void HandleRestoreElf(RestoreElfCommand const& command);
void HandleReadPrx(ReadPrxCommand const& command);
void HandleParseSpursTrace(ParseSpursTraceCommand const& command);
void HandleRewrite(RewriteCommand const& command);
void HandlePrxStore(PrxStoreCommand const& command);
void HandleRsxDasm(RsxDasmCommand const& command);
void HandleFindSpuElfs(FindSpuElfsCommand const& command);
void HandleDumpInstrDb(DumpInstrDbCommand const& command);
void HandlePrintGcmVizTrace(PrintGcmVizTraceCommand const& command);
void HandleTraceViz(TraceVizCommand const& command);
void HandleDasm(DasmCommand const& command);
void HandleUnpackTrp(UnpackTrpCommand const& command);
void HandleSplitLog(SplitLogCommand const& command);
