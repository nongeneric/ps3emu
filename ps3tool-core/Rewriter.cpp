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

    while (!leads.empty()) {
        auto cia = leads.top();
        leads.pop();
        log << ssnprintf("analyzing basic block %x(%x)\n", cia, cia - imageBase);
        if (allLeads.find(cia) != end(allLeads))
            continue;
        allLeads.insert(cia);
        
        for (;;) {
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
        elfs.push_back({(uint32_t)std::distance(begin(elf), elfit), header});
    }
    return elfs;
}
