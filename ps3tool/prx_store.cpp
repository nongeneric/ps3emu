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
        auto stolen = elf.map(g_state.mm, [&](auto va, auto size, auto) {
            imageBase = ::align(va + size, 1 << 10);
        }, imageBase, "");
        
        info->size = imageBase - info->imageBase;
    }
    
    g_state.config->save();
}

void rewritePrxStore() {
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
        
        rewrite.elf = prxPath.string();
        rewrite.cpp = prxPath.string() + ".cpp";
        rewrite.trace = false;
        rewrite.imageBase = prxInfo.imageBase;        
        HandleRewrite(rewrite);
    }
}

void compilePrxStore() {
    for (auto& prxInfo : g_state.config->sysPrxInfos) {
        if (!prxInfo.loadx86)
            continue;
        path prxStorePath = g_state.config->prxStorePath;
        auto prxPath = prxStorePath / "sys" / "external" / prxInfo.name;
        CompileInfo info;
        info.so = prxPath.string() + ".x86.so";
        info.cpp = prxPath.string() + ".cpp";
        info.debug = false;
        info.trace = false;
        info.log = false;
        std::cout << ssnprintf("compiling %s", prxInfo.name) << std::endl;
        compile(info);
    }
}

void HandlePrxStore(PrxStoreCommand const& command) {
    MainMemory mm;
    g_state.mm = &mm;
    auto internalAlloc = std::make_unique<InternalMemoryManager>(
        EmuInternalArea, EmuInternalAreaSize);
    auto heapAlloc = std::make_unique<InternalMemoryManager>(HeapArea, HeapAreaSize);
    g_state.memalloc = internalAlloc.get();
    g_state.heapalloc = heapAlloc.get();
    
    if (command.map) {
        mapPrxStore();
    }
    
    if (command.rewrite) {
        rewritePrxStore();
    }
    
    if (command.compile) {
        compilePrxStore();
    }
}
