#include "ps3tool.h"

#include "ps3emu/Config.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/HeapMemoryAlloc.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/build-config.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/state.h"
#include "ps3emu/utils.h"
#include "ps3tool-core/NinjaScript.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/align.hpp>
#include <boost/endian/arithmetic.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace std::filesystem;
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
    auto imageBase = 38 << 20;
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
            imageBase = boost::alignment::align_up(va + size, 1 << 10);
        }, imageBase, {}, &rewriterStore, true);

        info->size = imageBase - info->imageBase;
    }

    g_state.config->save();
}

std::string printEntriesString(std::vector<uint32_t> const& entries) {
    if (entries.empty())
        return "";
    std::string entriesString = "--entries";
    for (auto entry : entries) {
        entriesString += sformat(" {:x}", entry);
    }
    return entriesString;
}

void rewritePrxStore() {
    path prxStorePath = g_state.config->prxStorePath;
    auto& prxInfos = g_state.config->sysPrxInfos;

    NinjaScript rewriteScript;
    auto ps3tool = ps3toolPath();
    rewriteScript.rule("rewrite-ppu", ps3tool + " rewrite --no-fexcept --elf $in --cpp $cpp --image-base $imagebase");
    rewriteScript.rule("rewrite-spu", ps3tool + " rewrite --spu --elf $in --cpp $cpp --image-base $imagebase");

    NinjaScript buildScript;
    buildScript.variable("trace", "");

    auto external = prxStorePath / "sys" / "external";
    auto buildPath = external / g_buildName;
    create_directories(buildPath);

    for (auto& prxInfo : prxInfos) {
        auto prxPath = (external / prxInfo.name).string();
        assert(exists(prxPath));

        std::tuple imagebaseVar("imagebase", sformat("{:x}", prxInfo.imageBase));

        if (prxInfo.loadx86) {
            auto out = buildPath / (prxInfo.name + ".ninja");
            std::tuple cppVar{"cpp", (buildPath / (prxInfo.name)).string()};
            rewriteScript.statement("rewrite-ppu", prxPath, out, {imagebaseVar, cppVar});
            buildScript.subninja(out);
        }

        if (prxInfo.loadx86spu) {
            auto out = buildPath / (prxInfo.name + ".spu.ninja");
            std::tuple cppVar{"cpp", (buildPath / (prxInfo.name + ".spu")).string()};
            rewriteScript.statement("rewrite-spu", prxPath, out, {imagebaseVar, cppVar});
            buildScript.subninja(out);
        }
    }

    write_all_text(rewriteScript.dump(), (buildPath / "rewrite.ninja").string());
    write_all_text(buildScript.dump(), (buildPath / "build.ninja").string());

    write_all_lines({
        "#!/bin/bash",
        "set -e",
        "ninja-build -f rewrite.ninja -t clean",
        "ninja-build -f rewrite.ninja -j 3",
        "ninja-build -f build.ninja -t clean",
        "ninja-build -f build.ninja"
    }, (buildPath / "build.sh").string());
}

void HandlePrxStore(PrxStoreCommand const& command) {
    MainMemory mm;
    g_state.init();
    g_state.mm = &mm;
    auto internalAlloc = std::make_unique<InternalMemoryManager>(
        EmuInternalArea, EmuInternalAreaSize, "");
    auto heapAlloc = std::make_unique<HeapMemoryAlloc>();
    g_state.memalloc = internalAlloc.get();
    g_state.heapalloc = heapAlloc.get();

    if (command.map) {
        mapPrxStore();
    }

    if (command.rewrite) {
        rewritePrxStore();
    }
}
