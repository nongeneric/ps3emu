#include "ps3emu/utils.h"
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/spu/SPUDasm.h"
#include "ps3tool-core/Rewriter.h"
#include "TestUtils.h"
#include <catch/catch.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

TEST_CASE("rewriter_simple") {
    std::string output;
    auto res = rewrite(
        "./binaries/rewriter_simple/a.elf",
        "/tmp/x86.cpp",
        "--entries 1022c 10314 1039c 10418 10484 10518 1053c 105ac 10654"
        "--ignored 1045c 104c8",
        output);
    REQUIRE(res);
    res = compile({"/tmp/x86.cpp", "/tmp/x86.so", false, false});
    REQUIRE(res);
    
    output = startWaitGetOutput({"./binaries/rewriter_simple/a.elf"}, {"--x86", "/tmp/x86.so"});
    REQUIRE( output ==
        "test1 ep: 1022c\n"
        "test1 res: 1416\n"
        "test2 ep: 10314 1039c\n"
        "test2 res: -5\n"
        "test3 ep: 10418 1045c(ignore)\n"
        "test3 res: 382\n"
        "test4 ep: 10484 104c8(ignore) 10518\n"
        "test4 res: 98\n"
        "test5 ep: 1053c\n"
        "test5 res: 2008\n"
        "test6 ep: 105ac\n"
        "test6 res: 17\n"
        "test0 ep: 10654\n"
    );
}

template <typename V>
std::function<InstructionInfo(uint32_t)> make_analyze(V& vec, bool ppu = true) {
    return [=](uint32_t cia) {
        for (auto& p : vec) {
            if (p.first == cia)
                return ppu ? analyze(p.second, cia) : analyzeSpu(p.second, cia);
        }
        throw std::runtime_error(ssnprintf("bad cia: %x", cia));
    };
}

TEST_CASE("rewriter_block_discovery_1") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x22AC4, 0x39800000 }, /* li        r12, 0          */
        { 0x22AC8, 0x658C0003 }, /* oris      r12, r12, 3     */
        { 0x22ACC, 0x818C0154 }, /* lwz       r12, 0x154(r12) */
        { 0x22AD0, 0xF8410028 }, /* std       r2, arg_28(r1)  */
        { 0x22AD4, 0x800C0000 }, /* lwz       r0, 0(r12)      */
        { 0x22AD8, 0x804C0004 }, /* lwz       r2, 4(r12)      */
        { 0x22ADC, 0x7C0903A6 }, /* mtctr     r0              */
        { 0x22AE0, 0x4E800420 }, /* bctr                      */
    };
    std::stringstream log;
    
    auto blocks = discoverBasicBlocks(
        0x22ac4, 0x22ae4, 0, std::stack<uint32_t>({0x22ac4u}), log, make_analyze(instrs));
    
    REQUIRE(blocks.size() == 1);
    auto block = blocks.front();
    REQUIRE(block.start == 0x22ac4);
    REQUIRE(block.len == 0x22ae4 - 0x22ac4);
}

TEST_CASE("rewriter_block_discovery_2") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x10B14, 0xF821FF81 }, /* stdu      r1, -0x80(r1)        */
        { 0x10B18, 0x7C0802A6 }, /* mflr      r0                   */
        { 0x10B1C, 0xFBE10078 }, /* std       r31, 0x80+var_8(r1)  */
        { 0x10B20, 0xF8010090 }, /* std       r0, 0x80+arg_10(r1)  */
        { 0x10B24, 0x7C3F0B78 }, /* mr        r31, r1              */
        { 0x10B28, 0x7C601B78 }, /* mr        r0, r3               */
        { 0x10B2C, 0x901F00B0 }, /* stw       r0, 0xB0(r31)        */
        { 0x10B30, 0x801F00B0 }, /* lwz       r0, 0xB0(r31)        */
        { 0x10B34, 0x78000020 }, /* clrldi    r0, r0, 32           */
        { 0x10B38, 0x7C030378 }, /* mr        r3, r0               */
        { 0x10B3C, 0x3C800033 }, /* lis       r4, 0x33             */
        { 0x10B40, 0x4800D181 }, /* bl                             */
        
        { 0x10B44, 0xE8410028 }, /* ld        r2, 0x80+var_58(r1)  */
        { 0x10B48, 0x7C601B78 }, /* mr        r0, r3               */
        { 0x10B4C, 0x7C0007B4 }, /* extsw     r0, r0               */
        { 0x10B50, 0x7C030378 }, /* mr        r3, r0               */
        { 0x10B54, 0xE9610000 }, /* ld        r11, 0x80+var_80(r1) */
        { 0x10B58, 0xE80B0010 }, /* ld        r0, 0x10(r11)        */
        { 0x10B5C, 0x7C0803A6 }, /* mtlr      r0                   */
        { 0x10B60, 0xEBEBFFF8 }, /* ld        r31, -8(r11)         */
        { 0x10B64, 0x7D615B78 }, /* mr        r1, r11              */
        { 0x10B68, 0x4E800020 }, /* blr                            */

        { 0x1DCC0, 0x39800000 }, /* li        r12, 0               */
        { 0x1DCC4, 0x658C0002 }, /* oris      r12, r12, 2          */
        { 0x1DCC8, 0x818C0020 }, /* lwz       r12, 0x20(r12)       */
        { 0x1DCCC, 0xF8410028 }, /* std       r2, arg_28(r1)       */
        { 0x1DCD0, 0x800C0000 }, /* lwz       r0, 0(r12)           */
        { 0x1DCD4, 0x804C0004 }, /* lwz       r2, 4(r12)           */
        { 0x1DCD8, 0x7C0903A6 }, /* mtctr     r0                   */
        { 0x1DCDC, 0x4E800420 }, /* bctr                           */
    };
    std::stringstream log;
    
    auto blocks = discoverBasicBlocks(
        0x10B14, 0x1DCE0, 0, std::stack<uint32_t>({0x10B14}), log, make_analyze(instrs));
    
    REQUIRE(blocks.size() == 3);
    auto block = blocks[0];
    REQUIRE(block.start == 0x10B14);
    REQUIRE(block.len == 0x10B44 - 0x10B14);
    block = blocks[1];
    REQUIRE(block.start == 0x10B44);
    REQUIRE(block.len == 0x10B6C - 0x10B44);
    block = blocks[2];
    REQUIRE(block.start == 0x1DCC0);
    REQUIRE(block.len == 0x1DCE0 - 0x1DCC0);
}

TEST_CASE("rewriter_block_discovery_3") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x7034, 0x4C00012C }, /* isync                 */
        { 0x7038, 0xEBDD0000 }, /* ld        r30, 0(r29) */
        { 0x703C, 0x4E800020 }, /* blr                   */
    };
    std::stringstream log;
    
    auto blocks = discoverBasicBlocks(
        0x7034, 0x7040, 0, std::stack<uint32_t>({0x7034}), log, make_analyze(instrs));
    
    REQUIRE(blocks.size() == 1);
    auto block = blocks[0];
    REQUIRE(block.start == 0x7034);
    REQUIRE(block.len == 0x7040 - 0x7034);
}

TEST_CASE("spu_rewriter_discover_elfs") {
    auto vec = read_all_bytes("./binaries/spurs_task_queue/a.elf");
    auto infos = discoverEmbeddedSpuElfs(vec);
    REQUIRE( infos.size() == 2 );
    REQUIRE( infos[0].startOffset == 0x12880 );
    REQUIRE( ((intptr_t)infos[0].start - (intptr_t)&vec[0]) == 0x12880 );
    REQUIRE( infos[1].startOffset == 0x13e00 );
    REQUIRE( ((intptr_t)infos[1].start - (intptr_t)&vec[0]) == 0x13e00 );
    
    vec = read_all_bytes("./binaries/spurs_minimal_pm/hello_work_unit.elf");
    infos = discoverEmbeddedSpuElfs(vec);
    REQUIRE( infos.size() == 1 );
    REQUIRE( infos[0].startOffset == 0 );
    REQUIRE( ((intptr_t)infos[0].start - (intptr_t)&vec[0]) == 0 );
}

TEST_CASE("spu_rewriter_block_discovery_1") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x0340, 0x24004080 }, /* stqd           lr, arg_10(sp)      */
        { 0x0344, 0x24FFC0D0 }, /* stqd           r80, var_10(sp)     */
        { 0x0348, 0x4020007F }, /* nop            r127                */
        { 0x034C, 0x24FF80D1 }, /* stqd           r81, var_20(sp)     */
        { 0x0350, 0x081F81FE }, /* sf             r126, r3, r126      */
        { 0x0354, 0x3F838102 }, /* rotqbyi        r2, r2, 0xE         */
        { 0x0358, 0x24FEC081 }, /* stqd           sp, var_50(sp)      */
        { 0x035C, 0x1CEC0081 }, /* ai             sp, sp, -0x50       */
        { 0x0360, 0x22000902 }, /* brhz           r2, loc_3A8         */

        { 0x0364, 0x4202E050 }, /* ila            r80, 0x5C0          */
        { 0x0368, 0x427FFFD1 }, /* ila            r81, 0xFFFF         */
        { 0x036C, 0x181FA850 }, /* a              r80, r80, r126      */

        { 0x0370, 0x1DFFC10B }, /* ahi            r11, r2, -1         */
        { 0x0374, 0x00200000 }, /* lnop                               */
        { 0x0378, 0x1822E88A }, /* and            r10, r81, r11       */
        { 0x037C, 0x3FE08589 }, /* shlqbyi        r9, r11, 2          */
        { 0x0380, 0x0F608508 }, /* shli           r8, r10, 2          */
        { 0x0384, 0x23804589 }, /* stqr           r9, atexit_count    */
        { 0x0388, 0x18140407 }, /* a              r7, r8, r80         */
        { 0x038C, 0x38940406 }, /* lqx            r6, r8, r80         */
        { 0x0390, 0x3B81C305 }, /* rotqby         r5, r6, r7          */
        { 0x0394, 0x35200280 }, /* bisl           lr, r5              */
        
        { 0x0398, 0x33804304 }, /* lqr            r4, atexit_count    */
        { 0x039C, 0x3F838202 }, /* rotqbyi        r2, r4, 0xE         */
        { 0x03A0, 0x4020007F }, /* nop            r127                */
        { 0x03A4, 0x237FF982 }, /* brhnz          r2, loc_370         */

        { 0x03A8, 0x40800003 }, /* il             r3, 0               */
        { 0x03AC, 0x34018080 }, /* lqd            lr, 0x50+arg_10(sp) */
        { 0x03B0, 0x1C140081 }, /* ai             sp, sp, 0x50        */
        { 0x03B4, 0x34FFC0D0 }, /* lqd            r80, var_10(sp)     */
        { 0x03B8, 0x34FF80D1 }, /* lqd            r81, var_20(sp)     */
        { 0x03BC, 0x34FF40FE }, /* lqd            r126, var_30(sp)    */
        { 0x03C0, 0x35000000 }, /* bi             lr                  */
    };
    std::stringstream log;
    
    auto blocks = discoverBasicBlocks(
        0x0340, 0x03c4, 0, std::stack<uint32_t>({0x0340}), log, make_analyze(instrs, false));
    
    REQUIRE(blocks.size() == 5);
    auto block = blocks[0];
    REQUIRE(block.start == 0x0340);
    REQUIRE(block.len == 0x0364 - 0x0340);
    block = blocks[1];
    REQUIRE(block.start == 0x0364);
    REQUIRE(block.len == 0x0370 - 0x0364);
    block = blocks[2];
    REQUIRE(block.start == 0x0370);
    REQUIRE(block.len == 0x0398 - 0x0370);
    block = blocks[3];
    REQUIRE(block.start == 0x0398);
    REQUIRE(block.len == 0x03A8 - 0x0398);
    block = blocks[4];
    REQUIRE(block.start == 0x03A8);
    REQUIRE(block.len == 0x03c4 - 0x03A8);
}
