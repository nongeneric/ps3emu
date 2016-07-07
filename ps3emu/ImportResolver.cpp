#include "ImportResolver.h"

#include "ELFLoader.h"
#include "MainMemory.h"

void ImportEntry::resolveNcall(uint32_t index, bool stub) {
    _mm->store<4>(_stub, _descrVa);
    _mm->store<4>(_descrVa, _descrVa + 4);
    encodeNCall(_mm, _descrVa + 4, index);
    _resolution = stub ? ImportResolution::ncallStub : ImportResolution::ncall;
}

void ImportEntry::resolve(ps3_uintptr_t exportStubVa) {
    _mm->store<4>(_stub, exportStubVa);
    _resolution = ImportResolution::prx;
}

uint32_t ImportEntry::fnid() {
    return _fnid;
}

ps3_uintptr_t ImportEntry::stub() {
    return _stub;
}

ImportResolution ImportEntry::resolution() {
    return _resolution;
}

ImportEntry::ImportEntry(MainMemory* mm,
                         uint32_t fnid,
                         ps3_uintptr_t stub,
                         ps3_uintptr_t descrVa,
                         prx_symbol_type_t type)
    : _mm(mm),
      _fnid(fnid),
      _stub(stub),
      _descrVa(descrVa),
      _resolution(ImportResolution::unresolved),
      _type(type) {}
      
ImportedLibrary::ImportedLibrary(MainMemory* mm,
                                 ELFLoader* elf,
                                 prx_info* info,
                                 ps3_uintptr_t descrVa)
    : _mm(mm), _elf(elf), _descrVa(descrVa) {
    readString(_mm, info->prx_name, _name);
    _mm->setMemory(descrVa, 0, 8 * info->count, true);
    for (auto i = 0u; i < info->count; ++i) {
        auto fnid = _mm->load<4>(info->prx_fnids + sizeof(uint32_t) * i);
        uint32_t stub = info->prx_stubs + sizeof(uint32_t) * i;
        _entries.push_back({_mm, fnid, stub, _descrVa, prx_symbol_type_t::function});
        _descrVa += 8;
    }
}

ImportedLibrary::ImportedLibrary(MainMemory* mm,
                                 ELFLoader* elf,
                                 prx_import_t* info,
                                 ps3_uintptr_t descrVa,
                                 ps3_uintptr_t imageBase,
                                 ps3_uintptr_t ph1rva,
                                 ps3_uintptr_t ph1va)
    : _mm(mm), _elf(elf), _descrVa(descrVa) {
    readString(_mm, info->name + imageBase, _name);
    _mm->setMemory(descrVa, 0, 8 * info->functions, true);
    for (auto i = 0; i < info->functions; ++i) {
        auto fnid = _mm->load<4>(imageBase + info->fnids + sizeof(uint32_t) * i);
        uint32_t stub = ph1va + info->fstubs - ph1rva + sizeof(uint32_t) * i;
        _entries.push_back({_mm, fnid, stub, _descrVa, prx_symbol_type_t::function});
        _descrVa += 8;
    }
    for (auto i = 0; i < info->variables; ++i) {
        auto vnid = _mm->load<4>(imageBase + info->vnids + sizeof(uint32_t) * i);
        uint32_t stub = ph1va + info->vstubs - ph1rva + sizeof(uint32_t) * i;
        _entries.push_back({_mm, vnid, stub, 0, prx_symbol_type_t::variable});
    }
    for (auto i = 0; i < info->functions; ++i) {
        auto vnid = _mm->load<4>(imageBase + info->tls_vnids + sizeof(uint32_t) * i);
        uint32_t stub = ph1va + info->tls_vstubs - ph1rva + sizeof(uint32_t) * i;
        _entries.push_back({_mm, vnid, stub, 0, prx_symbol_type_t::tls_variable});
    }
}

ELFLoader* ImportedLibrary::elf() {
    return _elf;
}

std::string ImportedLibrary::name() {
    return _name;
}

std::vector<ImportEntry>& ImportedLibrary::entries() {
    return _entries;
}

ps3_uintptr_t ImportedLibrary::lastDescrVa() {
    return _descrVa;
}

ImportResolver::ImportResolver(MainMemory* mm)
    : _mm(mm), _descrVa(FunctionDescriptorsVa) {}

void ImportResolver::populate(ELFLoader* elf, prx_info* infos, size_t count) {
    for (auto i = 0u; i < count; ++i) {
        _libraries.push_back({_mm, elf, &infos[i], _descrVa});
        _descrVa = _libraries.back().lastDescrVa();
    }
}

void ImportResolver::populate(ELFLoader* elf,
                              prx_import_t* infos,
                              size_t count,
                              ps3_uintptr_t imageBase,
                              ps3_uintptr_t ph1rva,
                              ps3_uintptr_t ph1va) {
    for (auto i = 0u; i < count; ++i) {
        _libraries.push_back({_mm, elf, &infos[i], _descrVa, imageBase, ph1rva, ph1va});
        _descrVa = _libraries.back().lastDescrVa();
    }
}

std::vector<ImportedLibrary>& ImportResolver::libraries() {
    return _libraries;
}

prx_symbol_type_t ImportEntry::type() {
    return _type;
}
