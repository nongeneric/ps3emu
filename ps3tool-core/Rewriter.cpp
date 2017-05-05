#include "Rewriter.h"

#include "ps3emu/utils.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include <boost/optional.hpp>
#include <set>
#include <algorithm>

struct BasicBlockContext {
    uint32_t start;
    uint32_t length;
    uint32_t imageBase;
    std::set<uint32_t> allLeads;
    std::set<uint32_t> breaks;
    std::set<uint32_t> visited;
    std::ostream* log;
    std::function<InstructionInfo(uint32_t)> analyze;
    std::vector<uint32_t>* branches;
    
    void update(uint32_t newLead) {
        std::stack<uint32_t> leads;
        leads.push(newLead);
    
        while (!leads.empty()) {
            auto cia = leads.top();
            leads.pop();
            INFO(libs) << ssnprintf("analyzing basic block %x(%x)\n", cia, cia - imageBase);
            if (allLeads.find(cia) != end(allLeads))
                continue;
            allLeads.insert(cia);
    
            for (;;) {
                if (!subset(cia, 4u, start, length)) {
                    if (branches) {
                        branches->push_back(cia);
                    }
                    break;
                }
    
                visited.insert(cia);
                auto info = analyze(cia);
                auto logLead = [&](uint32_t target) {
                    INFO(libs) << ssnprintf("new lead %x(%x) -> %x(%x)\n",
                                      cia,
                                      cia - imageBase,
                                      target,
                                      target - imageBase);
                };
                if (info.target) {
                    leads.push(*info.target);
                    logLead(*info.target);
                }
                if (info.flow) {
                    if (info.passthrough) {
                        leads.push(cia + 4);
                        logLead(cia + 4);
                    }
                    breaks.insert(cia + 4);
                    INFO(libs) << ssnprintf(
                        "new break %x(%x)\n", cia + 4, cia + 4 - imageBase);
                    break;
                }
                cia += 4;
            }
        }
    }
};

std::vector<BasicBlock> discoverBasicBlocks(
    uint32_t start,
    uint32_t length,
    uint32_t imageBase,
    std::set<uint32_t> leads,
    std::ostream& log,
    analyze_t analyze,
    std::vector<uint32_t>* branches)
{
    BasicBlockContext context;
    context.start = start;
    context.length = length;
    context.imageBase = imageBase;
    context.log = &log;
    context.analyze = analyze;
    context.branches = branches;

    for (auto& lead : leads) {
        if (!subset(lead, 4u, start, length))
            throw std::runtime_error("a lead is outside the segment");
    }
    
    while (!leads.empty()) {
        auto lead = *begin(leads);
        context.update(lead);
        std::set<uint32_t> remaining;
        std::set_difference(begin(leads),
                            end(leads),
                            begin(context.visited),
                            end(context.visited),
                            std::inserter(remaining, begin(remaining)));
        leads = remaining;
    }
   
    boost::optional<BasicBlock> block;
    std::vector<BasicBlock> blocks;
    for (auto cia = start; cia < start + length; cia += 4) {
        bool isLead = context.allLeads.find(cia) != end(context.allLeads);
        bool isBreak = context.breaks.find(cia) != end(context.breaks);
        
        if (isLead || isBreak) {
            if (block) {
                blocks.push_back(*block);
            }
            if (isLead) {
                block = BasicBlock();
                block->start = cia;
            } else {
                block = boost::none;
            }
        }
        
        if (block) {
            block->len += 4;
        }
    }
    
    if (block) {
        blocks.push_back(*block);
    }
    
    return blocks;
}

uint32_t vaToOffset(const Elf32_be_Ehdr* header, uint32_t va) {
    auto phs = (Elf32_be_Phdr*)((char*)header + header->e_phoff);
    for (auto i = 0; i < header->e_phnum; ++i) {
        auto vaStart = phs[i].p_vaddr; 
        auto vaEnd = vaStart + phs[i].p_memsz;
        if (vaStart <= va && va < vaEnd) {
            if (va - vaStart > phs[i].p_filesz)
                throw std::runtime_error("out of range");
            return va - vaStart + phs[i].p_offset;
        }
    }
    throw std::runtime_error("out of range");
}

std::vector<EmbeddedElfInfo> discoverEmbeddedSpuElfs(std::vector<uint8_t> const& elf) {
    std::vector<uint8_t> magic { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3 };
    std::vector<EmbeddedElfInfo> elfs;
    auto it = begin(elf);
    for (;;) {
        it = std::search(it, end(elf), begin(magic), end(magic));
        if (it == end(elf))
            break;
        auto elfit = it++;
        auto header = (const Elf32_be_Ehdr*)&*elfit;
        if (header->e_ident[EI_CLASS] != ELFCLASS32)
            continue;
        if (sizeof(Elf32_be_Ehdr) != header->e_ehsize)
            continue;
        if (sizeof(Elf32_be_Phdr) != header->e_phentsize)
            continue;
        if (sizeof(Elf32_be_Shdr) != header->e_shentsize)
            continue;
        uint32_t end = 0;
        auto phs = (Elf32_be_Phdr*)((char*)header + header->e_phoff);
        for (auto i = 0; i < header->e_phnum; ++i) {
            end = std::max(end, phs[i].p_offset + phs[i].p_filesz);
        }
        if (header->e_shnum) {
            auto shs = (Elf32_be_Shdr*)((char*)header + header->e_shoff);
            for (auto i = 0; i < header->e_shnum; ++i) {
                end = std::max(end, shs[i].sh_addr + shs[i].sh_size);
            }
        }
        bool isJob = false;
        if (header->e_entry == 0x10) {
            auto ep = (big_uint32_t*)((char*)header + vaToOffset(header, header->e_entry));
            isJob = ep[0] == 0x44012850 && ep[1] == 0x32000080 && ep[2] == 0x44012850 && ep[4] == 0x62696E32;
        }
        elfs.push_back({(uint32_t)std::distance(begin(elf), elfit), end, isJob, header});
    }
    return elfs;
}
