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

    std::cout << "from idaapi import *\n"
                << "\n"
                << "def make_func(ea, name):\n"
                << "    del_func(ea)\n"
                << "    add_func(ea, -1)\n"
                << "    set_name(ea, name)\n\n";
    
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
                
    std::cout << "\n# exports\n";
    for (auto i = 0u; i < nexports; ++i) {
        auto& e = exports[i];
        auto libname = e.name ? &elf[pheader->p_offset + e.name] : "unknown";
        auto fnids = reinterpret_cast<big_uint32_t*>(
            &elf[pheader->p_offset + e.fnid_table]);
        auto stubs = reinterpret_cast<big_uint32_t*>(
            &elf[pheader->p_offset + e.stub_table]);
        std::cout << ssnprintf("\n# functions: %d, variables: %d, tls_variables: %d\n\n",
                               e.functions,
                               e.variables,
                               e.tls_variables);
        for (auto i = 0; i < e.functions + e.variables + e.tls_variables; ++i) {
            auto descr =
                reinterpret_cast<fdescr*>(&elf[pheader->p_offset + stubs[i]]);
                std::cout << ssnprintf("# exported symbol\n")
                      << ssnprintf("#   lib      %s\n", libname)
                      << ssnprintf("#   name     fnid_%08X\n", fnids[i])
                      << ssnprintf("#   fnid     %08X\n", fnids[i])
                      << ssnprintf("#   stub     %08x\n", stubs[i]);
                std::cout << ssnprintf(
                    "make_func(0x%08x, \"%s_fnid_%08X_%d\")\n\n", descr->va, libname, fnids[i], i);
        }
    }
    
    std::cout << "\n# imports\n";
    for (auto i = 0u; i < nimports; ++i) {
        auto& import = imports[i];
        auto libname = &elf[pheader->p_offset + import.name];
        auto fnids = (big_uint32_t*)&elf[pheader->p_offset + import.fnids];
        auto fstubs = (big_uint32_t*)&elf[pheader->p_offset + import.fstubs];
        std::cout << ssnprintf(
            "# fnid table: %08x, stub table: %08x\n", import.fnids, import.fstubs);
        for (auto i = 0u; i < import.functions; ++i) {
            std::cout << ssnprintf("# imported function\n")
                      << ssnprintf("#   lib      %s\n", libname)
                      << ssnprintf("#   name     fnid_%08X\n", fnids[i])
                      << ssnprintf("#   fnid     %08X\n", fnids[i])
                      << ssnprintf("#   fstub    %08x\n", fstubs[i])
                      << ssnprintf("#   fstubVa  %08x\n", import.fstubs + 4 * i);
            std::cout << ssnprintf("make_func(0x%08x, \"%s_stub_fnid_%08X_%d\")\n\n",
                                   fstubs[i],
                                   libname,
                                   fnids[i],
                                   i);
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
