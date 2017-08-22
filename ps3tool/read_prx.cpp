#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/utils.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/exports/splicer.h"
#include <boost/endian/arithmetic.hpp>
#include <iostream>
#include <fstream>

using namespace boost::endian;

std::string printFnid(uint32_t fnid) {
    auto name = fnidToName(fnid);
    if (name)
        return *name;
    return ssnprintf("fnid_%08X");
}

void HandleReadPrx(ReadPrxCommand const& command) {
    MainMemory mm;
    g_state.mm = &mm;
    ELFLoader loader;
    loader.load(command.elf);
    auto imageBase = command.prx ? 0x10000 : 0;
    loader.map([](auto va, auto size, auto index) {}, imageBase, {}, nullptr, false);
    auto [imports, nimports] = loader.imports();
    auto [exports, nexports] = loader.exports();
    auto module = loader.module();
    auto header = loader.header();
    auto elf = (uint8_t*)header;
    auto pheader = reinterpret_cast<Elf64_be_Phdr*>(elf + header->e_phoff);
    
    std::cout << "from idaapi import *\n"
                << "\n"
                << "def make_func(ea, name):\n"
                << "    del_func(ea)\n"
                << "    add_func(ea, -1)\n"
                << "    set_name(ea, name)\n\n";
    
    if (module) {
        auto tocOrigin = module->toc - 0x8000;
        auto tocSize = pheader[1].p_vaddr + pheader[1].p_filesz - tocOrigin;
        std::cout << ssnprintf("# module_info (%08x)\n", pheader->p_paddr)
                << ssnprintf("#   attributes %04x\n", module->attributes)
                << ssnprintf("#   minor version %x\n", module->minor_version)
                << ssnprintf("#   major version %x\n", module->major_version)
                << ssnprintf("#   name %s\n", module->name)
                << ssnprintf("#   toc %08x (origin %08x, size %08x)\n", module->toc, tocOrigin, tocSize)
                << ssnprintf("#   exports start %08x\n", module->exports_start)
                << ssnprintf("#   exports end %08x\n", module->exports_end)
                << ssnprintf("#   imports start %08x\n", module->imports_start)
                << ssnprintf("#   imports end %08x\n", module->imports_end);
    }
                
    std::cout << "\n# exports\n";
    for (auto i = 0; i < nexports; ++i) {
        auto& e = exports[i];
        std::string libname = "unknown";
        if (e.name) {
            readString(g_state.mm, e.name, libname);
        }
        auto fnids = (big_uint32_t*)g_state.mm->getMemoryPointer(e.fnid_table, 1);
        auto stubs = (big_uint32_t*)g_state.mm->getMemoryPointer(e.stub_table, 1);
        std::cout << ssnprintf("\n# functions: %d, variables: %d, tls_variables: %d\n\n",
                               e.functions,
                               e.variables,
                               e.tls_variables);
        for (auto i = 0; i < e.functions + e.variables + e.tls_variables; ++i) {
            auto descr =
                reinterpret_cast<fdescr*>(g_state.mm->getMemoryPointer(stubs[i], 1));
                std::cout << ssnprintf("# exported symbol\n")
                          << ssnprintf("#   lib      %s\n", libname)
                          << ssnprintf("#   name     %s\n", printFnid(fnids[i]))
                          << ssnprintf("#   fnid     %08x\n", fnids[i] - imageBase)
                          << ssnprintf("#   stub     %08x\n", stubs[i] - imageBase);
                std::cout << ssnprintf(
                    "make_func(0x%08x, \"%s_%s\")\n\n", descr->va - imageBase, libname, printFnid(fnids[i]));
        }
    }
    
    std::cout << "\n# imports\n";
    for (auto i = 0; i < nimports; ++i) {
        auto& import = imports[i];
        std::string libname;
        readString(g_state.mm, import.name, libname);
        auto fnids = (big_uint32_t*)g_state.mm->getMemoryPointer(import.fnids, 1);
        auto fstubs = (big_uint32_t*)g_state.mm->getMemoryPointer(import.fstubs, 1);
        std::cout << ssnprintf(
            "# fnid table: %08x, stub table: %08x\n", import.fnids, import.fstubs);
        for (auto i = 0u; i < import.functions; ++i) {
            std::cout << ssnprintf("# imported function\n")
                      << ssnprintf("#   lib      %s\n", libname)
                      << ssnprintf("#   name     %s\n", printFnid(fnids[i]))
                      << ssnprintf("#   fstub    %08x\n", fstubs[i] - imageBase)
                      << ssnprintf("#   fnid     %08x\n", fnids[i] - imageBase)
                      << ssnprintf("#   fstubVa  %08x\n", import.fstubs + 4 * i - imageBase);
            std::cout << ssnprintf("make_func(0x%08x, \"%s_stub_%s\")\n\n",
                                   fstubs[i] - imageBase,
                                   libname,
                                   printFnid(fnids[i]));
        }
    }
    
    std::cout << "\n# relocations\n\n";
    auto rel = (Elf64_be_Rela*)&elf[pheader[2].p_offset];
    auto endRel = rel + pheader[2].p_filesz / sizeof(Elf64_be_Rela);
    for (; rel != endRel; ++rel) {
        assert(((uint64_t)rel->r_offset >> 32) == 0);
        assert(((uint64_t)rel->r_addend >> 32) == 0);
        uint64_t info = rel->r_info;
        std::cout << ssnprintf("# offset: %08x\n", rel->r_offset)
                  << ssnprintf("# sym: %08x; type: %08x; addend: %08x\n\n",
                               ELF64_R_SYM(info), ELF64_R_TYPE(info), (uint32_t)rel->r_addend);
    }
}
