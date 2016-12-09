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
    std::function<uint32_t(uint32_t)> readInstr)
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
            auto info = analyze(readInstr(cia), cia);
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
