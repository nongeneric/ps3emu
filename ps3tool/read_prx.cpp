#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/utils.h"
#include <boost/endian/arithmetic.hpp>
#include <iostream>
#include <fstream>

using namespace boost::endian;

struct module_info_t {
    big_uint16_t attributes;
    big_uint16_t version;
    char name[28];
    big_uint32_t toc;
    big_uint32_t exports_start;
    big_uint32_t exports_end;
    big_uint32_t imports_start;
    big_uint32_t imports_end;
};

static_assert(sizeof(module_info_t) == 0x34, "");

struct prx_export_t {
    char size;
    char padding;
    big_uint16_t version;
    big_uint16_t attributes;
    big_uint16_t functions;
    big_uint16_t variables;
    big_uint16_t tls_variables;
    char hash;
    char tls_hash;
    char reserved[2];
    big_uint32_t name;
    big_uint32_t fnid_table;
    big_uint32_t stub_table;
};

static_assert(sizeof(prx_export_t) == 0x1c, "");

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
    auto nexports = (module->exports_end - module->exports_start) / sizeof(prx_export_t);
    std::cout << ssnprintf("prx name: %s\n", module->name);
    for (auto i = 0u; i < nexports; ++i) {
        auto& e = exports[i];
        if (e.name) {
            std::cout << ssnprintf(
                "exported library: %s (%d functions)\n",
                &elf[pheader->p_offset + e.name], e.functions);
            auto fnids = reinterpret_cast<big_uint32_t*>(&elf[pheader->p_offset + e.fnid_table]);
            auto stubs = reinterpret_cast<big_uint32_t*>(&elf[pheader->p_offset + e.stub_table]);
            for (auto i = 0u; i < e.functions; ++i) {
                auto descr = reinterpret_cast<fdescr*>(&elf[pheader->p_offset + stubs[i]]);
                std::cout << ssnprintf("\tfnid_%08X, descr: %08x, va: %08x\n", fnids[i], stubs[i], descr->va);
            }
        }
    }
}
