#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/utils.h"
#include <boost/endian/arithmetic.hpp>
#include <iostream>
#include <fstream>

using namespace boost::endian;

void HandleReadPrx(ReadPrxCommand const& command) {
    std::ifstream f(command.elf);
    if (!f.is_open()) {
        throw std::runtime_error("can't read elf");
    }
    f.seekg(0, std::ios_base::end);
    auto file_size = f.tellg();
    std::vector<char> elf(file_size);
    f.seekg(0, std::ios_base::beg);
    f.read(elf.data(), elf.size());

    auto header = reinterpret_cast<Elf64_be_Ehdr*>(&elf[0]);
    auto pheader = reinterpret_cast<Elf64_be_Phdr*>(&elf[header->e_phoff]);
    auto module = reinterpret_cast<module_info_t*>(&elf[pheader->p_paddr]);
    auto exports = reinterpret_cast<prx_export_t*>(&elf[pheader->p_offset + module->exports_start]);
    auto imports = reinterpret_cast<prx_import_t*>(&elf[pheader->p_offset + module->imports_start]);
    auto nexports = (module->exports_end - module->exports_start) / sizeof(prx_export_t);
    auto nimports = (module->imports_end - module->imports_start) / sizeof(prx_import_t);

    std::cout << ssnprintf("# %s\n", module->name);
    if (command.writeIdaScript) {
        std::cout << "from idaapi import *\n"
                  << "\n"
                  << "def make_func(ea, name):\n"
                  << "    del_func(ea)\n"
                  << "    add_func(ea, -1)\n"
                  << "    set_name(ea, name)\n\n";
    }
    std::cout << "\n# exports\n";
    for (auto i = 0u; i < nexports; ++i) {
        auto& e = exports[i];
        auto libname = e.name ? &elf[pheader->p_offset + e.name] : "unknown";
        auto fnids = reinterpret_cast<big_uint32_t*>(
            &elf[pheader->p_offset + e.fnid_table]);
        auto stubs = reinterpret_cast<big_uint32_t*>(
            &elf[pheader->p_offset + e.stub_table]);
        for (auto i = 0; i < e.functions + e.variables + e.tls_variables; ++i) {
            auto descr =
                reinterpret_cast<fdescr*>(&elf[pheader->p_offset + stubs[i]]);
            if (command.writeIdaScript) {
                std::cout
                    << ssnprintf("make_func(0x%08x, \"%s_fnid_%08X_%d\") # %08x\n",
                                    descr->va,
                                    libname,
                                    fnids[i],
                                    i,
                                    fnids[i]);
            } else {
                std::cout << ssnprintf("%s, fnid_%08X, %08x, %08x\n",
                                        libname,
                                        fnids[i],
                                        stubs[i],
                                        descr->va);
            }
        }
    }
    std::cout << "\n# imports\n";
    for (auto i = 0u; i < nimports; ++i) {
        auto& import = imports[i];
        auto libname = &elf[pheader->p_offset + import.name];
        auto fnids = reinterpret_cast<big_uint32_t*>(
            &elf[pheader->p_offset + import.fnid_table]);
        auto stubs =
            reinterpret_cast<big_uint32_t*>(&elf[pheader->p_offset + import.stubs]);
        for (auto i = 0u; i < import.functions; ++i) {
            if (command.writeIdaScript) {
                std::cout << ssnprintf("make_func(0x%08x, \"%s_stub_fnid_%08X_%d\") # %08x\n",
                                       stubs[i],
                                       libname,
                                       fnids[i],
                                       i,
                                       fnids[i]);
            } else {
                std::cout << ssnprintf("%s, fnid_%08X\n", libname, fnids[i]);
            }
        }
    }
}
