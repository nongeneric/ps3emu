#pragma once

#include <string>
#include <vector>
#include <stdint.h>

struct ShaderDasmCommand {
    std::string binary;
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
    bool writeIdaScript;
};

struct ParseSpursTraceCommand {
    std::string dump;
};

struct RewriteCommand {
    std::string elf;
    std::string cpp;
    uint32_t imageBase;
    std::vector<uint32_t> entryPoints;
    std::vector<uint32_t> ignoredEntryPoints;
};

struct PrxStoreCommand {
    bool map;
    bool rewrite;
    bool compile;
};

void HandleUnsce(UnsceCommand const& command);
void HandleShaderDasm(ShaderDasmCommand const& command);
void HandleRestoreElf(RestoreElfCommand const& command);
void HandleReadPrx(ReadPrxCommand const& command);
void HandleParseSpursTrace(ParseSpursTraceCommand const& command);
void HandleRewrite(RewriteCommand const& command);
void HandlePrxStore(PrxStoreCommand const& command);
