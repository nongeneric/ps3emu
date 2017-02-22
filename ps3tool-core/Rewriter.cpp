#include "Rewriter.h"

#include "ps3emu/utils.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include <boost/optional.hpp>
#include <set>

std::vector<BasicBlock> discoverBasicBlocks(
    uint32_t start,
    uint32_t length,
    uint32_t imageBase,
    std::stack<uint32_t> leads,
    std::ostream& log,
    std::function<InstructionInfo(uint32_t)> analyze)
{
    std::set<uint32_t> allLeads;
    std::set<uint32_t> breaks;

    auto leadsCopy = leads;
    while (!leadsCopy.empty()) {
        auto lead = leadsCopy.top();
        leadsCopy.pop();
        if (!subset(lead, 4u, start, length))
            throw std::runtime_error("a lead is outside the segment");
    }
    
    
    while (!leads.empty()) {
        auto cia = leads.top();
        leads.pop();
        log << ssnprintf("analyzing basic block %x(%x)\n", cia, cia - imageBase);
        if (allLeads.find(cia) != end(allLeads))
            continue;
        allLeads.insert(cia);
        
        for (;;) {
            if (!subset(cia, 4u, start, length))
                break;
            
            auto info = analyze(cia);
            auto logLead = [&] (uint32_t target) {
                log << ssnprintf("new lead %x(%x) -> %x(%x)\n",
                                 cia,
                                 cia - imageBase,
                                 target,
                                 target - imageBase);
            };
            if (info.target) {
                leads.push(info.target);
                logLead(info.target);
            }
            if (info.flow) {
                if (info.passthrough) {
                    leads.push(cia + 4);
                    logLead(cia + 4);
                }
                breaks.insert(cia + 4);
                log << ssnprintf("new break %x(%x)\n",
                                 cia + 4,
                                 cia + 4 - imageBase);
                break;
            }
            cia += 4;
        }
    }
   
    boost::optional<BasicBlock> block;
    std::vector<BasicBlock> blocks;
    for (auto cia = start; cia < start + length; cia += 4) {
        bool isLead = allLeads.find(cia) != end(allLeads);
        bool isBreak = breaks.find(cia) != end(breaks);
        
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
    return blocks;
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
        elfs.push_back({(uint32_t)std::distance(begin(elf), elfit), end, header});
    }
    return elfs;
}
