#include "ps3tool.h"

#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/utils.h"
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/InternalMemoryManager.h"
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

void rewritePrxStore(bool verbose) {
    path prxStorePath = g_state.config->prxStorePath;
    auto& prxInfos = g_state.config->sysPrxInfos;
    for (auto& prxInfo : prxInfos) {
        auto prxPath = prxStorePath / "sys" / "external" / prxInfo.name;
        assert(exists(prxPath));
        
        std::ifstream f(prxPath.string());
        assert(f.is_open());
        std::vector<char> elf(file_size(prxPath));
        f.read(elf.data(), elf.size());

        auto header = reinterpret_cast<Elf64_be_Ehdr*>(&elf[0]);
        auto pheader = reinterpret_cast<Elf64_be_Phdr*>(&elf[header->e_phoff]);
        auto module = reinterpret_cast<module_info_t*>(&elf[pheader->p_paddr]);
        auto exports = reinterpret_cast<prx_export_t*>(&elf[pheader->p_offset + module->exports_start]);
        auto nexports = (module->exports_end - module->exports_start) / sizeof(prx_export_t);
        
        RewriteCommand rewrite;
        for (auto i = 0u; i < nexports; ++i) {
            auto& e = exports[i];
            auto stubs = reinterpret_cast<big_uint32_t*>(
                &elf[pheader->p_offset + e.stub_table]);
            for (auto i = 0; i < e.functions; ++i) {
                auto descr = reinterpret_cast<fdescr*>(&elf[pheader->p_offset + stubs[i]]);
                rewrite.entryPoints.push_back(descr->va);
            }
        }
        
        auto funcList = prxPath.string() + ".entries";
        if (exists(funcList)) {
            std::ifstream f(funcList);
            assert(f.is_open());
            std::string line;
            while (std::getline(f, line)) {
                rewrite.entryPoints.push_back(std::stoi(line, 0, 16));
            }
        }
        
//         if (basename(prxPath) != "liblv2.sprx")
//             continue;
        
        rewrite.elf = prxPath.string();
        rewrite.cpp = prxPath.string() + ".cpp";
        rewrite.isSpu = false;
        rewrite.imageBase = prxInfo.imageBase;        
        
        if (verbose) {
            std::cout << ssnprintf("rewriting %s -> %s\n", rewrite.elf, rewrite.cpp);
        }
        
        HandleRewrite(rewrite);
        
        if (!prxInfo.loadx86spu)
            continue;
        
        rewrite.cpp = prxPath.string() + ".spu.cpp";
        rewrite.isSpu = true;
        rewrite.entryPoints.clear();
        rewrite.ignoredEntryPoints.clear();
        
        if (verbose) {
            std::cout << ssnprintf("rewriting [SPU] %s -> %s\n", rewrite.elf, rewrite.cpp);
        }
        
        HandleRewrite(rewrite);
    }
}

void compilePrxStore(bool verbose) {
    for (auto& prxInfo : g_state.config->sysPrxInfos) {
        if (!prxInfo.loadx86)
            continue;
        path prxStorePath = g_state.config->prxStorePath;
        auto prxPath = prxStorePath / "sys" / "external" / prxInfo.name;
        CompileInfo info;
        info.so = prxPath.string() + ".x86.so";
        info.cpp = prxPath.string() + ".cpp";
        info.debug = prxInfo.x86trace;
        info.trace = prxInfo.x86trace;
        std::cout << ssnprintf("compiling %s", prxInfo.name) << std::endl;
        auto line = compile(info);
        
        if (verbose) {
            std::cout << line << std::endl;
        }
        
        std::string output;
        auto res = exec(line, output);
        if (!res)
            std::cout << "compilation failed" << std::endl;
        
        if (!prxInfo.loadx86spu)
            continue;
        
        info.so = prxPath.string() + ".x86spu.so";
        info.cpp = prxPath.string() + ".spu.cpp";
        std::cout << ssnprintf("compiling spu %s", prxInfo.name) << std::endl;
        line = compile(info);
        
        if (verbose) {
            std::cout << line << std::endl;
        }
        
        res = exec(line, output);
        if (!res)
            std::cout << "compilation failed\n";
    }
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
        rewritePrxStore(command.verbose);
    }
    
    if (command.compile) {
        compilePrxStore(command.verbose);
    }
}
