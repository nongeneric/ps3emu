#include "ps3tool.h"

#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/utils.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3tool-core/NinjaScript.h"
#include <boost/endian/arithmetic.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <algorithm>
#include <fstream>

using namespace boost::filesystem;
using namespace boost::endian;
using namespace boost::algorithm;

void mapPrxStore() {
    path prxStorePath = g_state.config->prxStorePath;
    std::vector<path> prxPaths;
    directory_iterator end_it;
    for (directory_iterator it(prxStorePath / "sys" / "external"); it != end_it; ++it) {
        if (!ends_with(it->path().string(), ".elf"))
            continue;
        prxPaths.push_back(it->path());
    }
    
    std::sort(begin(prxPaths), end(prxPaths), [&](auto l, auto r) {
        auto known = {
            "liblv2.sprx.elf",
            "libsysmodule.sprx.elf",
            "libsysutil.sprx.elf",
            "libgcm_sys.sprx.elf",
            "libaudio.sprx.elf",
            "libio.sprx.elf",
            "libsre.sprx.elf",
            "libfs.sprx.elf",
            "libsysutil_np_trophy.sprx.elf"
        };
        auto lit = std::find_if(begin(known), end(known), [&](auto k) {
            return ends_with(l.string(), k);
        });
        auto rit = std::find_if(begin(known), end(known), [&](auto k) {
            return ends_with(r.string(), k);
        });
        auto litn = lit == end(known) ? -1 : std::distance(begin(known), lit);
        auto ritn = rit == end(known) ? -1 : std::distance(begin(known), rit);
        if (litn != -1 && ritn != -1)
            return litn < ritn;
        if (litn != -1 && ritn == -1)
            return true;
        if (litn == -1 && ritn != -1)
            return false;
        return l < r;
    });
    
    RewriterStore rewriterStore;
    auto imageBase = 10 << 20;
    for (auto& prxPath : prxPaths) {
        auto& prxInfos = g_state.config->sysPrxInfos;
        auto name = prxPath.filename().string();
        auto it = std::find_if(begin(prxInfos), end(prxInfos), [&] (auto& info) {
            return info.name == name;
        });
        SysPrxInfo* info;
        if (it != end(prxInfos)) {
            info = &*it;
        } else {
            prxInfos.push_back({});
            info = &prxInfos.back();
            info->name = name;
        }
        info->imageBase = imageBase;
        
        ELFLoader elf;
        elf.load(prxPath.string());
        auto stolen = elf.map([&](auto va, auto size, auto) {
            imageBase = ::align(va + size, 1 << 10);
        }, imageBase, {}, &rewriterStore);
        
        info->size = imageBase - info->imageBase;
    }
    
    g_state.config->save();
}

std::string printEntriesString(std::vector<uint32_t> const& entries) {
    if (entries.empty())
        return "";
    std::string entriesString = "--entries";
    for (auto entry : entries) {
        entriesString += ssnprintf(" %x", entry);
    }
    return entriesString;
}

void rewritePrxStore() {
    path prxStorePath = g_state.config->prxStorePath;
    auto& prxInfos = g_state.config->sysPrxInfos;
    
    NinjaScript rewriteScript;
    auto ps3tool = ps3toolPath();
    rewriteScript.rule("rewrite-ppu", ps3tool + " rewrite --elf $in --cpp $in --image-base $imagebase $entries $entries-file");
    rewriteScript.rule("rewrite-spu", ps3tool + " rewrite --spu --elf $in --cpp $in.spu --image-base $imagebase $entries $entries-file");
    rewriteScript.variable("entries", "");
    rewriteScript.variable("entries-file", "");
    
    NinjaScript buildScript;
    buildScript.variable("trace", "");
    
    for (auto& prxInfo : prxInfos) {
        auto prxPath = (prxStorePath / "sys" / "external" / prxInfo.name).string();
        assert(exists(prxPath));
        
        std::ifstream f(prxPath);
        assert(f.is_open());
        std::vector<char> elf(file_size(prxPath));
        f.read(elf.data(), elf.size());

        auto header = reinterpret_cast<Elf64_be_Ehdr*>(&elf[0]);
        auto pheader = reinterpret_cast<Elf64_be_Phdr*>(&elf[header->e_phoff]);
        auto module = reinterpret_cast<module_info_t*>(&elf[pheader->p_paddr]);
        auto exports = reinterpret_cast<prx_export_t*>(&elf[pheader->p_offset + module->exports_start]);
        auto nexports = (module->exports_end - module->exports_start) / sizeof(prx_export_t);
        
        std::vector<uint32_t> ppuEntries;
        for (auto i = 0u; i < nexports; ++i) {
            auto& e = exports[i];
            auto stubs = reinterpret_cast<big_uint32_t*>(
                &elf[pheader->p_offset + e.stub_table]);
            for (auto i = 0; i < e.functions; ++i) {
                auto descr = reinterpret_cast<fdescr*>(&elf[pheader->p_offset + stubs[i]]);
                ppuEntries.push_back(descr->va);
            }
        }
        
        auto imagebaseVar = std::make_tuple("imagebase", ssnprintf("%x", prxInfo.imageBase));
        
        if (prxInfo.loadx86) {
            std::vector<std::tuple<std::string, std::string>> vars;
            vars.push_back(imagebaseVar);
            auto entriesPath = prxPath + ".entries";
            if (exists(entriesPath)) {
                vars.push_back({"entries-file", "--entries-file " + entriesPath});
            }
            vars.push_back({"entries", printEntriesString(ppuEntries)});
            auto out = prxPath + ".ninja";
            rewriteScript.statement("rewrite-ppu", prxPath, out, vars);
            buildScript.subninja(out);
            buildScript.defaultStatement(prxInfo.name + ".x86.so");
        }
        
        if (prxInfo.loadx86spu) {
            auto out = prxPath + ".spu.ninja";
            rewriteScript.statement("rewrite-spu", prxPath, out, {imagebaseVar});
            buildScript.subninja(out);
            buildScript.defaultStatement(prxInfo.name + ".spu.x86.so");
        }
    }
    
    auto relative = prxStorePath / "sys" / "external";
    write_all_text(rewriteScript.dump(), (relative / "rewrite.ninja").string());
    write_all_text(buildScript.dump(), (relative / "build.ninja").string());
    
    write_all_lines({
        "#!/bin/bash",
        "set -e",
        "ninja-build -f rewrite.ninja -t clean",
        "ninja-build -f rewrite.ninja -j 5",
        "ninja-build -f build.ninja -t clean",
        "ninja-build -f build.ninja"
    }, (relative / "build.sh").string());
}

void HandlePrxStore(PrxStoreCommand const& command) {
    MainMemory mm;
    g_state.mm = &mm;
    auto internalAlloc = std::make_unique<InternalMemoryManager>(
        EmuInternalArea, EmuInternalAreaSize, "");
    auto heapAlloc = std::make_unique<InternalMemoryManager>(HeapArea, HeapAreaSize, "");
    g_state.memalloc = internalAlloc.get();
    g_state.heapalloc = heapAlloc.get();
    
    if (command.map) {
        mapPrxStore();
    }
    
    if (command.rewrite) {
        rewritePrxStore();
    }
}
