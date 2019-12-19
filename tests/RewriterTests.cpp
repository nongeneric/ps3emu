#include "ps3emu/utils.h"
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/spu/SPUDasm.h"
#include "ps3emu/build-config.h"
#include "ps3tool-core/Rewriter.h"
#include "TestUtils.h"
#include <catch.hpp>
#include <filesystem>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

TEST_CASE("rewriter_simple") {
    test_interpreter_and_rewriter({testPath("rewriter_simple/a.elf")},
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

template <typename V>
std::function<std::string(uint32_t)> make_name(V& vec, bool ppu = true) {
    return [=](uint32_t cia) {
        for (auto& p : vec) {
            if (p.first == cia) {
                std::string name;
                auto instr = endian_reverse(p.second);
                if (ppu) {
                    ppu_dasm<DasmMode::Name>(&instr, cia, &name);
                    return "ppu_" + name;
                } else {
                    SPUDasm<DasmMode::Name>(&instr, cia, &name);
                    return "spu_" + name;
                }
            }
        }
        throw std::runtime_error(ssnprintf("bad cia: %x", cia));
    };
}

template <typename V>
std::function<bool(uint32_t)> make_validate(V& vec, bool ppu = true) {
    return [=](uint32_t cia) {
        for (auto& p : vec) {
            if (p.first == cia) {
                try {
                    std::string str;
                    auto instr = endian_reverse(p.second);
                    if (ppu) {
                        ppu_dasm<DasmMode::Print>(&instr, cia, &str);
                    } else {
                        SPUDasm<DasmMode::Print>(&instr, cia, &str);
                    }
                    return true;
                } catch (...) {
                    return false;
                }
            }   
        }
        throw std::runtime_error(ssnprintf("bad cia: %x", cia));
    };
}

template <typename V>
std::function<uint32_t(uint32_t)> make_read(V& vec) {
    return [=](uint32_t cia) {
        for (auto& p : vec) {
            if (p.first == cia)
                return p.second;
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
    
    auto blocks = discoverBasicBlocks(0x22ac4,
                                      0x22ae4 - 0x22ac4,
                                      std::set<uint32_t>({0x22ac4u}),
                                      log,
                                      make_analyze(instrs),
                                      make_validate(instrs));
    
    REQUIRE(blocks.size() == 1);
    auto block = blocks.front();
    REQUIRE(block.start == 0x22ac4);
    REQUIRE(block.len == 0x22ae4 - 0x22ac4);
}

TEST_CASE("rewriter_block_discovery_3") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x7034, 0x4C00012C }, /* isync                 */
        { 0x7038, 0xEBDD0000 }, /* ld        r30, 0(r29) */
        { 0x703C, 0x4E800020 }, /* blr                   */
    };
    std::stringstream log;
    
    auto blocks = discoverBasicBlocks(0x7034,
                                      0x7040 - 0x7034,
                                      std::set<uint32_t>({0x7034}),
                                      log,
                                      make_analyze(instrs),
                                      make_validate(instrs));
    
    REQUIRE(blocks.size() == 1);
    auto block = blocks[0];
    REQUIRE(block.start == 0x7034);
    REQUIRE(block.len == 0x7040 - 0x7034);
}

TEST_CASE("spu_rewriter_discover_elfs") {
    auto vec = read_all_bytes(testPath("spurs_task_queue/a.elf"));
    auto infos = discoverEmbeddedSpuElfs(vec);
    REQUIRE( infos.size() == 2 );
    REQUIRE( infos[0].startOffset == 0x12880 );
    REQUIRE( ((intptr_t)infos[0].header - (intptr_t)&vec[0]) == 0x12880 );
    REQUIRE( infos[1].startOffset == 0x13e00 );
    REQUIRE( ((intptr_t)infos[1].header - (intptr_t)&vec[0]) == 0x13e00 );
    
    vec = read_all_bytes(testPath("spurs_minimal_pm/hello_work_unit.elf"));
    infos = discoverEmbeddedSpuElfs(vec);
    REQUIRE( infos.size() == 1 );
    REQUIRE( infos[0].startOffset == 0 );
    REQUIRE( ((intptr_t)infos[0].header - (intptr_t)&vec[0]) == 0 );
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
    
    auto blocks = discoverBasicBlocks(0x0340,
                                      0x03c4 - 0x0340,
                                      std::set<uint32_t>({0x0340}),
                                      log,
                                      make_analyze(instrs, false),
                                      make_validate(instrs, false));
    
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

TEST_CASE("spu_rewriter_simple") {
    test_interpreter_and_rewriter({testPath("spurs_task_hello/a.elf")},
        "SPU: Hello world!\n"
    );
}

TEST_CASE("spu_rewriter_block_discovery_2") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x0340, 0x24004080 }, /* stqd           lr, arg_10(sp)      */
        { 0x0344, 0x24FFC0D0 }, /* stqd           r80, var_10(sp)     */
        { 0x0348, 0x4020007F }, /* nop            r127                */
        { 0x034C, 0x24FF80D1 }, /* stqd           r81, var_20(sp)     */
    };
    std::stringstream log;
    
    auto blocks = discoverBasicBlocks(0x0340,
                                      12,
                                      std::set<uint32_t>({0x0344}),
                                      log,
                                      make_analyze(instrs, false),
                                      make_validate(instrs, false));
    
    REQUIRE(blocks.size() == 2);
    auto block = blocks[0];
    REQUIRE(block.start == 0x0340);
    REQUIRE(block.len == 4);
    block = blocks[1];
    REQUIRE(block.start == 0x0344);
    REQUIRE(block.len == 8);
}

TEST_CASE("rewriter_block_discovery_optimize_leads_dont_remove_targets") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x0838, 0x2F9F0000 }, /* cmpwi     cr7, r31, 0   */
        { 0x083C, 0x38600001 }, /* li        r3, 1         */
        { 0x0840, 0x419C0034 }, /* blt       cr7, loc_874  */

        { 0x0844, 0x38600001 }, /* li        r3, 1         */
        { 0x0848, 0x38600001 }, /* li        r3, 1         */
        { 0x084c, 0x38600001 }, /* li        r3, 1         */
        { 0x0850, 0x7FC4F378 }, /* mr        r4, r30       */
        { 0x0854, 0x78630020 }, /* clrldi    r3, r3, 32    */
        { 0x0858, 0x38600001 }, /* li        r3, 1         */
        { 0x085C, 0x7C7F1B78 }, /* mr        r31, r3       */
        { 0x0860, 0x38600001 }, /* li        r3, 1         */
        { 0x0864, 0x38600001 }, /* li        r3, 1         */
        { 0x0868, 0x38600001 }, /* li        r3, 1         */
        { 0x086C, 0x38800127 }, /* li        r4, 0x127     */
        { 0x0870, 0x38600001 }, /* li        r3, 1         */
                            
        { 0x0874, 0xE8010090 }, /* ld        r0, 0x90(r1)  */
        { 0x0878, 0x7FE307B4 }, /* extsw     r3, r31       */
        { 0x087C, 0xEBC10070 }, /* ld        r30, 0x70(r1) */
        { 0x0880, 0xEBE10078 }, /* ld        r31, 0x78(r1) */
    };
    std::stringstream log;
    
    auto blocks = discoverBasicBlocks(0x0838,
                                      0x0884 - 0x0838,
                                      std::set<uint32_t>({0x0838}),
                                      log,
                                      make_analyze(instrs, true),
                                      make_validate(instrs, true));
    
    REQUIRE(blocks.size() == 3);
    auto block = blocks[0];
    REQUIRE(block.start == 0x0838);
    REQUIRE(block.len == 12);
    block = blocks[1];
    REQUIRE(block.start == 0x0844);
    REQUIRE(block.len == 0x0874 - 0x0844);
    block = blocks[2];
    REQUIRE(block.start == 0x0874);
    REQUIRE(block.len == 16);
}

TEST_CASE("rewriter_block_discovery_optimize_leads_dont_remove_bl_ret") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x022F24, 0x48005215 }, /* bl        ._Geterrno   */
        { 0x022F28, 0x60000000 }, /* nop                    */
    };
    std::stringstream log;
    
    auto blocks = discoverBasicBlocks(0x022F24,
                                      8,
                                      std::set<uint32_t>({0x022F24}),
                                      log,
                                      make_analyze(instrs, true),
                                      make_validate(instrs, true));
    
    REQUIRE(blocks.size() == 2);
    auto block = blocks[0];
    REQUIRE(block.start == 0x022F24);
    REQUIRE(block.len == 4);
    block = blocks[1];
    REQUIRE(block.start == 0x022F28);
    REQUIRE(block.len == 4);
}

TEST_CASE("rewriter_basic_block_address_propagation") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x3160, 0x4080320D }, /* il             r13, 0x64        */
        { 0x3164, 0x1C19018C }, /* ai             r12, r3, 0x64    */
        { 0x3168, 0x4218C808 }, /* ila            r8, sub_3190     */
        { 0x316C, 0x3883418B }, /* lqx            r11, r3, r13     */
        { 0x3170, 0x42318009 }, /* ila            r9, loc_6300     */
        { 0x3174, 0x42342802 }, /* ila            r2, loc_6850     */
        { 0x3178, 0x3B83058A }, /* rotqby         r10, r11, r12    */
        { 0x317C, 0x7C004505 }, /* ceqi           r5, r10, 1       */
        { 0x3180, 0x7C000506 }, /* ceqi           r6, r10, 0       */
        { 0x3184, 0x80E24405 }, /* selb           r7, r8, r9, r5   */
        { 0x3188, 0x80808386 }, /* selb           r4, r7, r2, r6   */
        { 0x318C, 0x35000200 }, /* bi             r4               */
    };
    
    auto labels = discoverIndirectLocations(
        0x3160, 0x3190 - 0x3160, make_analyze(instrs, false), make_name(instrs, false));
    REQUIRE((labels == std::set<uint32_t>{0x3190, 0x6300, 0x6850}));
}

TEST_CASE("rewriter_jump_table_detection") {
    std::vector<std::pair<uint32_t, uint32_t>> instrs {
        { 0x3218, 0x42191E37 }, /* ila            r55, off_323C    */
        { 0x321C, 0x340DA938 }, /* lqd            r56, 0x360(r82)  */
        { 0x3220, 0x0F609C36 }, /* shli           r54, r56, 2      */
        { 0x3224, 0x5C015C33 }, /* clgti          r51, r56, 5      */
        { 0x3228, 0x180DDB35 }, /* a              r53, r54, r55    */
        { 0x322C, 0x388D9BB4 }, /* lqx            r52, r55, r54    */
        { 0x3230, 0x3B8D5A02 }, /* rotqby         r2, r52, r53     */
        { 0x3234, 0x21000433 }, /* brnz           r51, loc_3254    */
        { 0x3238, 0x35000100 }, /* bi             r2               */
        { 0x323C, 0x000032C0 }, /* .int sub_32C0                   */
        { 0x3240, 0x000032D0 }, /* .int sub_32D0                   */
        { 0x3244, 0x000032E0 }, /* .int sub_32E0                   */
        { 0x3248, 0x00003308 }, /* .int sub_3308                   */
        { 0x324C, 0x00003318 }, /* .int sub_3318                   */
        { 0x3250, 0x00010440 }, /* .int 10440                      */
        { 0x3254, 0x00000001 }, /* .int 1                          */
        { 0x3258, 0x00000000 }, /* .int 0                          */
    };
    
    auto[table, len] = discoverJumpTable(0x3218,
                                         0x325C - 0x3218,
                                         0x1000,
                                         0x5000,
                                         make_analyze(instrs, false),
                                         make_name(instrs, false),
                                         make_read(instrs));
    REQUIRE(table == 0x323C);
    REQUIRE(len == 20);
}
