#include <catch2/catch.hpp>

#include "TestUtils.h"
#include <boost/regex.hpp>
#include <filesystem>
#include <vector>

TEST_CASE("simple_printf") {
    test_interpreter_and_rewriter({testPath("simple_printf/a.elf")}, "some output\n");
}

TEST_CASE("bubblesort") {
    test_interpreter_and_rewriter(
        {testPath("bubblesort/a.elf"), "5", "17", "30", "-1", "20", "12", "100", "0"},
        "args: 5 17 30 -1 20 12 100 0 \nsorted: -1 0 5 12 17 20 30 100 \n");
}

TEST_CASE("md5") {
    auto output = startWaitGetOutput({testPath("md5/a.elf"), "-x"});
    REQUIRE( output ==
        "MD5 test suite results:\n\n"
        "d41d8cd98f00b204e9800998ecf8427e \"\"\n\n"
        "0cc175b9c0f1b6a831c399e269772661 \"a\"\n\n"
        "900150983cd24fb0d6963f7d28e17f72 \"abc\"\n\n"
        "f96b697d7cb7938d525a2f31aaf161d0 \"message digest\"\n\n"
        "c3fcd3d76192e4007dfb496cca67e13b \"abcdefghijklmnopqrstuvwxyz\"\n\n"
        "d174ab98d277d9f5a5611c2c9f419d9f \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\"\n\n"
        "57edf4a22be3c955ac49da2e2107b67a \"12345678901234567890123456789012345678901234567890123456789012345678901234567890\"\n\n"
    );
}

TEST_CASE("printf") {
    auto output = startWaitGetOutput({testPath("printf/a.elf"), "-x"});
    REQUIRE( output ==
        "    0\n"
        "123456789\n"
        "  -10\n"
        "-123456789\n"
        "0    \n"
        "123456789\n"
        "-10  \n"
        "-123456789\n"
        "000\n"
        "001\n"
        "123456789\n"
        "-10\n"
        "-123456789\n"
        "'   10'\n"
        "'10   '\n"
        "'00010'\n"
        "'  +10'\n"
        "'+10  '\n"
        "'10.3'\n"
        "'10.35'\n"
        "'   10.35'\n"
        "' 10.3456'\n"
        "'00010.35'\n"
        "'10.35   '\n"
        "'1012.35 '\n"
        "'101234567.35'\n"
        "'Hello'\n"
        "'     Hello'\n"
        "'Hello     '\n");
}

TEST_CASE("fcmpconv") {
    auto output = startWaitGetOutput({testPath("fcmpconv/a.elf")});
    REQUIRE( output ==
        "b > c: 0\n"
        "b < c: 1\n"
        "b == c: 0\n"
        "b > 0: 1\n"
        "b < 0: 0\n"
        "c > 0: 1\n"
        "c < 0: 0\n"
        "(int)3.14: 3\n"
        "(int)-6: -6\n"
        "(int)-6.6: -6\n"
        "(float)7: 7.000000\n"
        "(float)-100: -100.000000\n"
        "1.190000\n"
    );
}

TEST_CASE("matrixmul") {
    auto output = startWaitGetOutput({testPath("matrixmul/a.elf")});
    REQUIRE( output ==
        "mul3 = 1.851600e+01\n"
        "isnan(NAN)         = 1\n"
        "isnan(INFINITY)    = 0\n"
        "isnan(0.0)         = 0\n"
        "isnan(DBL_MIN/2.0) = 0\n"
        "isnan(0.0 / 0.0)   = 1\n"
        "isnan(Inf - Inf)   = 1\n"
        "isfinite(NAN)         = 0\n"
        "isfinite(INFINITY)    = 0\n"
        "isfinite(0.0)         = 1\n"
        "isfinite(DBL_MIN/2.0) = 1\n"
        "isfinite(1.0)         = 1\n"
        "isfinite(exp(800))    = 0\n"
        "isinf(NAN)         = 0\n"
        "isinf(INFINITY)    = 1\n"
        "isinf(0.0)         = 0\n"
        "isinf(DBL_MIN/2.0) = 0\n"
        "isinf(1.0)         = 0\n"
        "isinf(exp(800))    = 1\n"
        "isnormal(NAN)         = 0\n"
        "isnormal(INFINITY)    = 0\n"
        "isnormal(0.0)         = 0\n"
        "isnormal(1.0)         = 1\n"
        "isunordered(NAN,1.0) = 1\n"
        "isunordered(1.0,NAN) = 1\n"
        "isunordered(NAN,NAN) = 1\n"
        "isunordered(1.0,0.0) = 0\n"
        "mul4vec = 7.984118e+04\n"
        "a.b.c=\n"
        "11633689.00\n"
        "38058352.00\n"
        "-12969724.00\n"
        "7520124.50\n"
        "2673266.75\n"
        "-5750935.50\n"
        "24634854.00\n"
        "42614816.00\n"
        "-22973844.00\n"
        "1436.70\n"
        "3466.32\n"
        "2365965.00\n"
        "some nan\n"
        "-515707.59\n"
    );
}

TEST_CASE("dtoa") {
    auto output = startWaitGetOutput({testPath("dtoa/a.elf")});
    REQUIRE( output ==
        "3.13 = 3.13\n"
        "0.02380113 = 0.02380113\n"
        "3.23234 * 0.1292999 = 0.41794123876600003\n"
        "-493893848 = -493893848\n"
        "1.322828e300 = 1.322828e+300\n"
        "0.0000182919575748888 = 1.82919575748888e-5\n"
    );
}

TEST_CASE("float_printf") {
    auto output = startWaitGetOutput({testPath("float_printf/a.elf")});
    REQUIRE( output ==
        "18.516 = 1.851600e+01\n"
        "float = 4.179412e-01\n"
        "double = 4.179412e-01\n"
    );
}

TEST_CASE("gcm_context_size") {
    auto output = startWaitGetOutput({testPath("gcm_context_size/a.elf")});
    REQUIRE( output ==  "1bff\n" );
}

TEST_CASE("gcm_memory_mapping") {
    auto output = startWaitGetOutput({testPath("gcm_memory_mapping/a.elf")});
    REQUIRE( output ==
        "host_addr to offset: 0\n"
        "offset 0 to address == host_addr?: 1\n"
        "address 0xc0000005 to offset: 5\n"
    );
}

TEST_CASE("hello_simd") {
    auto output = startWaitGetOutput({testPath("hello_simd/a.elf")});
    REQUIRE( output ==
        "vector: 2,2,2,2\n"
        "x: 2\n"
        "y: 2\n"
        "z: 2\n"
        "w: 2\n"
        "vector: 0,1,0,0\n"
        "matrix: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1\n"
        "matrix: 2,0,0,0,0,2,0,0,0,0,2,0,0,0,0,2\n"
        "matrix: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1\n"
        "matrix: 2,0,0.99988,0.84704,0.46444,2,0,0,0,0,0.64666,0,0,0.86688,0,0.86688\n"
        "matrix: 1.14636,0.744333,0.752061,0.739199,0.938779,1.14636,0.4355,0.398784,0,0,0.0109291,0,0.408125,0.756513,0.138723,0.157507\n"
        "matrix: 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15\n"
        "matrix: 0,4,8,12,1,5,9,13,2,6,10,14,3,7,11,15\n"
        "matrix: 0.813116,-0.525972,-0.249392,0,-0.569181,-0.628632,-0.529958,0,0.121967,0.572866,-0.810524,0,-0.0406558,0.0646375,0.533075,1\n"
        "matrix: 1,0,0,0,0,0.267499,0.963558,0,0,-0.963558,0.267499,0,-10,23.5568,-27.2961,1\n"
        "matrix: -1,0,0,0,0,-0.267499,0.963558,0,0,-0.963558,-0.267499,0,10,20,30,-1\n"
        "matrix3: 1,0,0,0,0,0.267499,-0.963558,0,0\n"
        "matrix3: -0.904072,0,0.42738,0,0,1,0,0,-0.42738\n"
        "vector3: -0.42738,-0.871126,-0.241838\n"
        "point3: -6.33811,-12.9572,-1.15236\n"
        "vector3: 0.42738,0.871126,0.241838\n"
        "vector3: -0.492483,0,0.870322\n"
        "vector3: -0.75816,0.491059,-0.429015\n"
        "matrix: -0.492483,0,0.870322,0,-0.75816,0.491059,-0.429015,0,-0.42738,-0.871126,-0.241838,-0,0,0,0,1\n"
        "matrix: -0.492483,-0.75816,-0.42738,0,0,0.491059,-0.871126,0,0.870322,-0.429015,-0.241838,0,-2.11848,1.06309,-14.2749,1\n"
        "matrix: 0.492483,0,-0.870322,0,0.75816,-0.491059,0.429015,0,0.42738,0.871126,0.241838,0,-6.33811,-12.9572,-1.15236,-1\n"
        "vector3: -1.3,-1.3,-1.3\n"
        "vector: -1.3,-1.3,-1.3,-1.3\n"
        "matrix3: 1,0,0,0,-1.3,-1.3,-1.3,-1.3,0.8\n"
        "vector3: 0,0,0\n"
        "vector3: -0.963558,-0.963558,-0.963558\n"
        "vector3: 0.267499,0.267499,0.267499\n"
        "vector: 0,0.267499,0,0\n"
        "vector3: 0,0.267499,0\n"
        "vector3: 0,0.267499,-0.963558\n"
        "vector3: 0,0.963558,0\n"
        "vector3: 0,0.963558,0.267499\n"
        "matrix3: 1,0,0,0,0,0.267499,-0.963558,0,0\n"
        "vector: -0.963558,-0.963558,-0.963558,-0.963558\n"
        "sinf\n"
        "vector: 0,0,0,0\n"
        "vector: 80000000,80000000,80000000,80000000\n"
        "vector: -0.827606,-0.827606,-0.827606,-0.827606\n"
        "vector: -1.32761,-1.32761,-1.32761,-1.32761\n"
        "vector: ffffffff,ffffffff,ffffffff,ffffffff\n"
        "vector: 3,3,3,3\n"
        "vector: -1,-1,-1,-1\n"
        "vector: 0.270796,0.270796,0.270796,0.270796\n"
        "vector: 0.0198577,0.0198577,0.0198577,0.0198577\n"
        "vector: 0.00831777,0.00831777,0.00831777,0.00831777\n"
        "vector: 0,0,0,0\n"
        "vector: 0.963558,0.963558,0.963558,0.963558\n"
        "vector: 0,0,0,0\n"
        "vector: -0.963558,-0.963558,-0.963558,-0.963558\n"
    );
}

TEST_CASE("basic_large_cmdbuf") {
    auto output = startWaitGetOutput({testPath("basic_large_cmdbuf/a.elf")});
    REQUIRE( output ==
        "end - begin = 6ffc\n"
        "success\n"
    );
}

TEST_CASE("ppu_threads") {
    auto output = startWaitGetOutput({testPath("ppu_threads/a.elf")});
    REQUIRE( output == "exitstatus: 3; i: 4000\n" );
}

TEST_CASE("ppu_threads_tls") {
    auto output = startWaitGetOutput({testPath("ppu_threads_tls/a.elf")});
    REQUIRE( output ==
        "exitstatus: 125055; i: 4000\n"
        "primary thread tls_int: 500\n"
    );
}

TEST_CASE("ppu_threads_atomic_inc") {
    auto output = startWaitGetOutput({testPath("ppu_threads_atomic_inc/a.elf")});
    REQUIRE( output == "exitstatus: 1; i: 80000\n" );
}

TEST_CASE("ppu_threads_atomic_single_lwarx") {
    auto output = startWaitGetOutput({testPath("ppu_threads_atomic_single_lwarx/a.elf")});
    REQUIRE( output == "5, 3\n" );
}

TEST_CASE("ppu_cellgame") {
    auto output = startWaitGetOutput({testPath("ppu_cellgame/USRDIR/a.elf")});
    REQUIRE( output ==
        "title: GameUpdate Utility Sample\n"
        "gamedir: EMUGAME\n"
        "contentdir: /dev_hdd0\n"
        "usrdir: /dev_hdd0/USRDIR\n"
        "filesize: 4\n"
    );
}

TEST_CASE("ppu_cellSysutil") {
    auto output = startWaitGetOutput({testPath("ppu_cellSysutil/a.elf")});
    REQUIRE( output ==
        "CELL_SYSUTIL_SYSTEMPARAM_ID_LANG = 1\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN = 1\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT = 1\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT = 1\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE = 180\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME = 0\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL = 9\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT = 0\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_INTERNET_BROWSER_START_RESTRICT = 0\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT = 0\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ = 4\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE = 1\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE = 9\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD = 0\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD = 1\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF = 0\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_MAGNETOMETER = 0\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME = PS3-EMU\n"
        "CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME = PS3-EMU\n"
    );
}

TEST_CASE("ppu_threads_lwmutex_lwcond") {
    test_interpreter_and_rewriter({testPath("ppu_threads_lwmutex_lwcond/a.elf")},
        "test_lwmutex: 0; i: 4000\n"
        "test_lwmutex_recursive: 0; i: 4000\n"
        "test_lwcond: 5015; i: 0\n"
    );
}

TEST_CASE("ppu_threads_mutex_cond") {
    auto output = startWaitGetOutput({testPath("ppu_threads_mutex_cond/a.elf")});
    REQUIRE( output ==
        "test_mutex: 0; i: 4000\n"
        "test_mutex_recursive: 0; i: 4000\n"
        "test_cond: 5015; i: 0\n"
    );
}

TEST_CASE("ppu_threads_rwlock") {
    auto output = startWaitGetOutput({testPath("ppu_threads_rwlock/a.elf")});
    REQUIRE( output ==
        "test_rwlock_w: 0; i: 4000\n"
        "test_lwmutex: 40; i: 10\n"
    );
}

TEST_CASE("ppu_threads_queue") {
    auto output = startWaitGetOutput({testPath("ppu_threads_queue/a.elf")});
    REQUIRE( output ==
        "test_correctness(1): 0; i: 481200\n"
        "test_correctness(0): 0; i: 481200\n"
    );
}

TEST_CASE("ppu_threads_lwcond_init") {
    auto output = startWaitGetOutput({testPath("ppu_threads_lwcond_init/a.elf")});
    REQUIRE( output ==
        "sys_lwmutex_t.recursive_count 0\n"
        "sys_lwmutex_t.attribute 22\n"
        "sys_lwmutex_t.lock_var.all_info 0\n"
        "SYS_SYNC_PRIORITY | SYS_SYNC_NOT_RECURSIVE 22\n"
        "&mutex == cv.lwmutex 1\n"
    );
}

TEST_CASE("ppu_syscache") {
    auto output = startWaitGetOutput({testPath("ppu_syscache/a.elf")});
    REQUIRE( output ==
        "cellSysCacheMount() : 0x0  sysCachePath:[/dev_hdd1]\n"
        "cellSysCacheClear Ok\n"
        "Save sample data\n"
        "Check sample data\n"
        "    file /dev_hdd1/GAME-DATA1 check OK\n"
        "    file /dev_hdd1/GAME-DATA2 check OK\n"
        "Check OK\n"
    );
}

TEST_CASE("ppu_threads_is_stack") {
    auto output = startWaitGetOutput({testPath("ppu_threads_is_stack/a.elf")});
    REQUIRE( output ==
        "main thread: 1100\n"
        "other thread: 1100\n"
    );
}

TEST_CASE("ppu_fs_readdir") {
    auto output = startWaitGetOutput({testPath("ppu_fs_readdir/a.elf")});
    REQUIRE( output ==
        "type: 1, namelen: 1, name: .\n"
        "type: 1, namelen: 2, name: ..\n"
        "type: 2, namelen: 5, name: file1\n"
        "type: 2, namelen: 5, name: file2\n"
        "type: 2, namelen: 5, name: file3\n"
        "type: 1, namelen: 6, name: subdir\n"
        "size 4096, block size 512, name .\n"
        "size 4096, block size 512, name ..\n"
        "size 3, block size 512, name file1\n"
        "size 6, block size 512, name file2\n"
        "size 3, block size 512, name file3\n"
        "size 0, block size 512, name subdir\n"
    );
}

TEST_CASE("ppu_fios") {
    auto output = startWaitGetOutput({testPath("ppu_fios/USRDIR/a.elf")});
    REQUIRE( output ==
        "FiosSimple build date : Jan  4 2016 09:39:06\n"
        "FiosSimple start.\n"
        "SysCache : /dev_hdd1\n"
        "GameData : /dev_hdd0/USRDIR\n"
        "main 242 : Following files are found in \"/test.psarc\".\n"
        "main 386 : prefetchedSize = 1234567\n"
        "It took _ to read \"pattern5.dat\" from cache.\n"
        "It took _ to read \"pattern5.dat\" without cache.\n"
        "FiosSimple finished.\n"
    );
}

TEST_CASE("ppu_hash") {
    test_interpreter_and_rewriter({testPath("ppu_hash/a.elf")},
        "md5 0 b1a49029323448bf6407b94ad6f6f2cf\n"
        "sha1 0 6eb89053fa6048876d0210e5524b55752908af55\n"
        "sha224 0 2d47a0c20145d4ee365abd1de270b9b8747f7574c664f5db8179d86b\n"
        "sha256 0 3b3b7c0e64a7030bb6b36b9eb8afa279818f75bb4f962f89a3fd0df93d510c5f\n"
        "sha384 0 48070f66c4a1dac55fc4c5a4a0db8677ae1f8ac13e473dd73bab525832d73999fc2fea7f83b95d2aab0c95fe41df11c4\n"
        "sha512 0 9478b106b00d65f506d196006d59b59cf2ba38837abea1adc634cd89a583eac615f60102f482892906c3442b2e5d95dc04f63cf2b66cf9de62ae99a8b42639ef\n"
    );
}

TEST_CASE("ppu_simd_math") {
    auto output = startWaitGetOutput({testPath("ppu_simd_math/a.elf")});
    REQUIRE( output ==
        "Vector3(01): 5.280, 5.280, 5.280\n"
        "Vector3(02): 1.900, 1.900, 1.900\n"
        "Vector3(03): 6.200, 6.200, 6.200\n"
        "Vector3(04): 0.273, 0.273, 0.273\n"
        "Vector3(05): 4.100, 5.100, 6.100\n"
        "float(06): 4.100\n"
        "float(07): 5.100\n"
        "float(08): 6.100\n"
        "Vector3(09): 7.320, 7.320, 7.320\n"
        "Matrix3(01): 7.300, 7.300, 7.300, 7.300, 7.300, 7.300, 7.300, 7.300, 7.300\n"
        "Matrix3(02): -0.900, -0.900, -0.900, -0.900, -0.900, -0.900, -0.900, -0.900, -0.900\n"
        "Matrix3(03): 2.300, 0.000, 0.000, 0.000, 3.200, 0.000, 0.000, 0.000, 4.000\n"
        "Matrix3(04): 1.100, 1.100, 1.100, 1.000, 2.000, 3.000, 0.000, 0.000, 4.000\n"
        "Vector3(05): 1.100, 2.000, 0.000\n"
        "Vector3(06): 1.000, 2.000, 3.000\n"
        "boolInVec(01): 1\n"
        "boolInVec(02): 0\n"
        "boolInVec(03): 0\n"
        "boolInVec(04): 1\n"
        "boolInVec(05): 1\n"
        "boolInVec(06): 0\n"
        "boolInVec(07): 1\n"
        "boolInVec(08): 1\n"
        "boolInVec(09): 1\n"
        "floatInVec(01): 0.707\n"
        "floatInVec(02): 0.863\n"
        "floatInVec(03): 2.153\n"
        "floatInVec(04): 0.650\n"
        "floatInVec(05): 0.913\n"
        "floatInVec(06): 1.000\n"
        "floatInVec(07): -0.760\n"
        "floatInVec(08): 0.725\n"
        "floatInVec(09): -1.520\n"
        "floatInVec(10): 1.693\n"
        "floatInVec(11): 1.138\n"
        "floatInVec(12): 0.760\n"
        "floatInVec(13): 1.260\n"
        "floatInVec(14): 0.000\n"
        "floatInVec(15): -0.180\n"
        "floatInVec(16): 0.760\n"
        "floatInVec(17): -0.500\n"
        "floatInVec(18): 0.260\n"
        "floatInVec(19): 0.910\n"
        "floatInVec(20): -0.119\n"
        "floatInVec(21): 0.565\n"
        "floatInVec(22): -0.396\n"
        "floatInVec(23): -1.000\n"
        "floatInVec(24): -0.274\n"
        "floatInVec(25): 0.000\n"
        "floatInVec(26): 0.760\n"
        "floatInVec(27): -0.760\n"
        "floatInVec(28): 1.147\n"
        "floatInVec(29): 1.316\n"
        "floatInVec(30): -0.240\n"
        "floatInVec(31): 1.147\n"
        "floatInVec(32): 0.650\n"
        "floatInVec(33): 0.760\n"
        "floatInVec(34): 0.689\n"
        "floatInVec(35): 0.872\n"
        "floatInVec(36): 0.950\n"
        "floatInVec(37): 0.000\n"
        "floatInVec(38): 2.138\n"
        "Vector3(01): 1.000, 1.200, 1.400\n"
        "Vector3(02): 1.000, 1.200, -1.400\n"
        "Vector3(03): 1.600, 8.800, 6.400\n"
        "Matrix3(04): 0.000, 1.400, 1.200, -1.400, 0.000, 1.000, -1.200, -1.000, 0.000\n"
        "Matrix3(05): 13.200, -2.400, 2.800, 4.000, 6.400, 5.600, -6.000, 7.200, 2.800\n"
        "Vector3(06): 0.500, -0.300, -0.233\n"
        "floatInVec(07): -11.200\n"
        "floatInVec(08): 2.098\n"
        "floatInVec(09): 4.400\n"
        "Vector3(10): 4.400, 16.480, -23.760\n"
        "floatInVec(11): 1.400\n"
        "Vector3(12): 2.000, 4.000, 1.400\n"
        "floatInVec(13): -1.200\n"
        "Vector3(14): 2.000, -4.800, -8.400\n"
        "Vector3(15): 0.477, -0.572, 0.667\n"
        "Matrix3(16): 2.000, -2.400, 2.800, 4.000, -4.800, 5.600, -6.000, 7.200, -8.400\n"
        "Vector3(17): 1.000, -0.833, 0.714\n"
        "Vector3(18): 1.000, 0.913, 0.845\n"
        "Vector3(19): 2.000, 4.000, -6.000\n"
        "Vector3(20): 1.000, -1.200, 1.400\n"
        "Vector3(21): 0.454, 0.891, 0.000\n"
        "Vector3(22): 1.000, 1.095, 1.183\n"
        "floatInVec(23): 1.200\n"
        "Point3(24): 2.200, -2.400, 4.600\n"
        "floatInVec(25): 4.643\n"
        "floatInVec(26): 5.636\n"
        "floatInVec(27): 21.560\n"
        "floatInVec(28): -2.400\n"
        "floatInVec(29): 6.000\n"
        "Matrix3(30): 3.500, -1.517, 0.017, -9.000, 4.233, -0.233, 5.000, -2.167, 0.167\n"
        "Vector3(31): 1.000, -1.200, 1.400\n"
    );
}

TEST_CASE("spu_getting_argp") {
    test_interpreter_and_rewriter({testPath("spu_getting_argp/a.elf")},
                                  "Creating an SPU thread group.\n"
                                  "Initializing SPU thread 0\n"
                                  "All SPU threads have been successfully initialized.\n"
                                  "Starting the SPU thread group.\n"
                                  "All SPU threads exited by sys_spu_thread_exit().\n"
                                  "SPU thread 0's exit status = 0\n"
                                  "SUCCESS: fromSpu is the same as toSpu\n"
                                  "## libdma : sample_dma_getting_argp SUCCEEDED ##\n");
}

TEST_CASE("raw_spu_printf") {
    test_interpreter_and_rewriter({testPath("raw_spu_printf/a.elf")},
                                  "Initializing SPUs\n"
                                  "sys_raw_spu_create succeeded. raw_spu number is 0\n"
                                  "Hello, World 1\n"
                                  "Hello, World 2\n"
                                  "a, 12, 20, 0x1e, 0X28,   50, \"test\"\n");
}

TEST_CASE("gcm_memory") {
    auto output = startWaitGetOutput({testPath("gcm_memory/a.elf")});
    REQUIRE( output ==
        "* vidmem base: 0xc0000000\n"
        "* vidmem size: 0xf900000\n"
        "* IO size    : 0x100000\n"
        "localAddress offset: 8\n"
        "ioAddress offset: 8\n"
        "offset: 10000000\n"
        "offset: 0\n"
        "va to io: ? -> 0\n"
        "io to va: 0 -> ?\n"
        "va: 0\n"
        "va to io: ? -> 0\n"
        "va to io: ? -> 5\n"
        "va to io: ? -> 6\n"
        "va to io: ? -> 7\n"
        "io to va: 0 -> ?\n"
        "io to va: 5 -> ?\n"
        "io to va: 6 -> ?\n"
        "io to va: 7 -> ?\n"
        "ea2 offset: 500000\n"
        "err: 802100ff\n"
        "va to io: ? -> 0\n"
        "va to io: ? -> 5\n"
        "va to io: ? -> 6\n"
        "va to io: ? -> 7\n"
        "io to va: 0 -> ?\n"
        "io to va: 5 -> ?\n"
        "io to va: 6 -> ?\n"
        "io to va: 7 -> ?\n"
        "err: 0\n"
        "va to io: ? -> 0\n"
        "va to io: ? -> 5\n"
        "va to io: ? -> 6\n"
        "va to io: ? -> 7\n"
        "va to io: ? -> f\n"
        "va to io: ? -> 10\n"
        "io to va: 0 -> ?\n"
        "io to va: 5 -> ?\n"
        "io to va: 6 -> ?\n"
        "io to va: 7 -> ?\n"
        "io to va: f -> ?\n"
        "io to va: 10 -> ?\n"
        "ea3 offset: f00000\n"
        "va to io: ? -> 0\n"
        "va to io: ? -> 5\n"
        "va to io: ? -> 6\n"
        "va to io: ? -> 7\n"
        "va to io: ? -> f\n"
        "va to io: ? -> 10\n"
        "va to io: ? -> 1\n"
        "va to io: ? -> 2\n"
        "va to io: ? -> 3\n"
        "va to io: ? -> 4\n"
        "io to va: 0 -> ?\n"
        "io to va: 1 -> ?\n"
        "io to va: 2 -> ?\n"
        "io to va: 3 -> ?\n"
        "io to va: 4 -> ?\n"
        "io to va: 5 -> ?\n"
        "io to va: 6 -> ?\n"
        "io to va: 7 -> ?\n"
        "io to va: f -> ?\n"
        "io to va: 10 -> ?\n"
        "ea4 offset: 100000\n"
        "va to io: ? -> 0\n"
        "va to io: ? -> 5\n"
        "va to io: ? -> 6\n"
        "va to io: ? -> 7\n"
        "va to io: ? -> 1\n"
        "va to io: ? -> 2\n"
        "va to io: ? -> 3\n"
        "va to io: ? -> 4\n"
        "io to va: 0 -> ?\n"
        "io to va: 1 -> ?\n"
        "io to va: 2 -> ?\n"
        "io to va: 3 -> ?\n"
        "io to va: 4 -> ?\n"
        "io to va: 5 -> ?\n"
        "io to va: 6 -> ?\n"
        "io to va: 7 -> ?\n"
        "va to io: ? -> 0\n"
        "va to io: ? -> 5\n"
        "va to io: ? -> 6\n"
        "va to io: ? -> 7\n"
        "io to va: 0 -> ?\n"
        "io to va: 5 -> ?\n"
        "io to va: 6 -> ?\n"
        "io to va: 7 -> ?\n"
    );
}

TEST_CASE("gcm_transfer") {
    auto output = startWaitGetOutput({testPath("gcm_transfer/a.elf")});
    REQUIRE( output ==
        "success 100\n"
        "success 100()\n"
        "success 101\n"
        "success 200\n"
        "success 300\n"
        "success 400\n"
        "success 500\n"
        "success 600\n"
        "success 601\n"
        "success 700\n"
        "success 800\n"
        "success 900\n"
        "success 1000\n"
        "success 1100\n"
        "success 1200\n"
        "success 1300\n"
        "success 1400\n"
        "success 1500\n"
        "success 1600\n"
        "success 1700\n"
        "success 1800\n"
        "success 1900\n"
        "success 2000\n"
        "completed\n"
    );
}

TEST_CASE("opengl_hash") {
    auto output = startWaitGetOutput({testPath("opengl_hash/a.elf")});
    REQUIRE( output ==
        "hash: 1d0\n"
    );
}

TEST_CASE("raw_spu_opengl_dma") {
    test_interpreter_and_rewriter({testPath("raw_spu_opengl_dma/a.elf")},
        "Initializing SPUs\n"
        "sys_raw_spu_create succeeded. raw_spu number is 0\n"
        "waiting for _jsAsyncCopyQ\n"
        "reading for _jsAsyncCopyQ\n"
        "waiting for _jsAsyncIOIF\n"
        "reading for _jsAsyncIOIF\n"
        "starting copy\n"
        "waiting for completion\n"
        "success!\n"
    );
}

TEST_CASE("ppu_dcbz") {
    auto output = startWaitGetOutput({testPath("ppu_dcbz/a.elf")});
    REQUIRE( output ==
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "1111111111111111111111111111111111111111111111111111111111111111\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "0000000000000000000000000000000000000000000000000000000000000000\n"
        "\n"
    );
}

TEST_CASE("ppu_float_cmp") {
    auto output = startWaitGetOutput({testPath("ppu_float_cmp/a.elf")});
    REQUIRE( output ==
        "1 1 1 0 0 0 1 0 "
    );
}

TEST_CASE("ppu_sraw") {
    auto output = startWaitGetOutput({testPath("ppu_sraw/a.elf")});
    REQUIRE( output ==
        "1000\n"
        "1000\n"
        "250\n"
        "4000\n"
        "500\n"
    );
}

TEST_CASE("ppu_fs") {
    auto output = startWaitGetOutput({testPath("ppu_fs/a.elf")});
    REQUIRE( output ==
        "FILE 0:\n"
        "\t_Mode = 161\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\t_Rsize = 0\n"
        "\t_offset = 0\n"
        "\n"
        "\n"
        "FILE 1:\n"
        "\t_Mode = 161\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\t_Rsize = 0\n"
        "\t_offset = 64\n"
        "\n"
        "\n"
        "FILE 2:\n"
        "\t_Mode = 161\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\t_Rsize = 0\n"
        "\t_offset = 0\n"
        "\n"
        "\n"
        "size: 64\n"
        "pos: 0\n"
        "FILE 3:\n"
        "\t_Mode = 20705\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\t_Rsize = 64\n"
        "\t_offset = 4\n"
        "\n"
        "\n"
        "pos: 4\n"
        "FILE 4:\n"
        "\t_Mode = 16865\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\t_Rsize = 64\n"
        "\t_offset = 0\n"
        "\n"
        "\n"
        "readbytes: 60\n"
        "content(10): 0:GCC:fals\n"
        "FILE 5:\n"
        "\t_Mode = 16609\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\t_Rsize = 0\n"
        "\t_offset = 0\n"
        "\n"
        "\n"
        "FILE 6:\n"
        "\t_Mode = 20705\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\t_Rsize = 64\n"
        "\t_offset = 64\n"
        "\n"
        "\n"
        "readbytes: 1\n"
        "content(10): #v4.0:GCC:\n"
    );
}

TEST_CASE("pngdec_ppu") {
    auto output = startWaitGetOutput({testPath("pngdec_ppu/a.elf")});
    REQUIRE( output ==
        "* displayInit: displayInit: create display ... WIDTH=1280, HEIGHT=720\n"
        "* createModules: cellPngDecCreate() returned CELL_OK\n"
        "* openStream: open filename = /app_home/SampleStream.png\n"
        "* openStream: cellPngDecOpen() returned CELL_OK\n"
        "* setDecodeParam: cellPngDecReadHeader() returned CELL_OK\n"
        "* setDecodeParam: info.imageWidth       = 1024\n"
        "* setDecodeParam: info.imageHeight      = 681\n"
        "* setDecodeParam: info.numComponents    = 3\n"
        "* setDecodeParam: info.bitDepth         = 8\n"
        "* setDecodeParam: info.colorSpace       = 2\n"
        "* setDecodeParam: info.interlaceMethod  = 0\n"
        "* setDecodeParam: info.chunkInformation = 80\n"
        "\n"
        "* setDecodeParam: cellPngDecSetParameter() returned CELL_OK\n"
        "* setDecodeParam: pngdecOutParam.outputWidthByte         = 4096\n"
        "* setDecodeParam: pngdecOutParam.outputWidth             = 1024\n"
        "* setDecodeParam: pngdecOutParam.outputHeight            = 681\n"
        "* setDecodeParam: pngdecOutParam.outputComponents        = 4\n"
        "* setDecodeParam: pngdecOutParam.outputBitDepth          = 8\n"
        "* setDecodeParam: pngdecOutParam.outputMode              = 0\n"
        "* setDecodeParam: pngdecOutParam.outputColorSpace        = 10\n"
        "* setDecodeParam: pngdecOutParam.useMemorySpace          = 42049\n"
        "\n"
        "* decodeStream: cellPngDecDecodeData() returned CELL_OK\n"
        "* decodeStream: pngdecDataOutInfo.chunkInformation  = 8080\n"
        "* decodeStream: pngdecDataOutInfo.numText           = 0\n"
        "* decodeStream: pngdecDataOutInfo.numUnknownChunk   = 0\n"
        "* decodeStream: pngdecDataOutInfo.status            = 0\n"
        "\n"
        "* closeStream: cellPngDecClose() returned CELL_OK\n"
        "* destoryModules: cellPngDecDestroy() returned CELL_OK\n"
        "disp_destroy> cellGcmFinish done ...\n"
        "* main: Call Malloc Function = 10\n"
        "* main: Call Free Function = 10\n"
        "* errorLog: Finished decoding \n"
    );
}

TEST_CASE("spurs_task_hello") {
    test_interpreter_and_rewriter({testPath("spurs_task_hello/a.elf")},
        "SPU: Hello world!\n"
    );
}

TEST_CASE("spu_generic_test") {
    std::string output = startWaitGetOutput({testPath("spu_generic_test/a.elf")});
    std::string expected =
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "message: ! 10 !\n"
        "o[0] = 10111213 03151617 18191a1b 1c1d1e1f\n"
        "o[1] = 10111213 14151617 1819031b 1c1d1e1f\n"
        "o[2] = 10111203 14151617 18191a1b 1c1d1e1f\n"
        "o[3] = 10110313 14151617 18191a1b 1c1d1e1f\n"
        "o[4] = 10111213 14150317 18191a1b 1c1d1e1f\n"
        "o[5] = 10111213 14150317 18191a1b 1c1d1e1f\n"
        "o[6] = 10111213 14150317 18191a1b 1c1d1e1f\n"
        "o[7] = 10111213 14150317 18191a1b 1c1d1e1f\n"
        "o[8] = 00010203 04050607 18191a1b 1c1d1e1f\n"
        "o[9] = 10111213 14151617 00010203 04050607\n"
        "o[10] = 00010203 04050607 18191a1b 1c1d1e1f\n"
        "o[11] = 00010203 04050607 18191a1b 1c1d1e1f\n"
        "o[12] = 00010203 04050607 18191a1b 1c1d1e1f\n"
        "o[13] = 00010203 04050607 18191a1b 1c1d1e1f\n"
        "o[14] = 00010203 04050607 18191a1b 1c1d1e1f\n"
        "o[15] = 00010203 04050607 18191a1b 1c1d1e1f\n"
        "o[16] = 10111213 02031617 18191a1b 1c1d1e1f\n"
        "o[17] = 10111213 14151617 18190203 1c1d1e1f\n"
        "o[18] = 10110203 14151617 18191a1b 1c1d1e1f\n"
        "o[19] = 10110203 14151617 18191a1b 1c1d1e1f\n"
        "o[20] = 10111213 14150203 18191a1b 1c1d1e1f\n"
        "o[21] = 10111213 14150203 18191a1b 1c1d1e1f\n"
        "o[22] = 10111213 14150203 18191a1b 1c1d1e1f\n"
        "o[23] = 10111213 14150203 18191a1b 1c1d1e1f\n"
        "o[24] = 10111213 00010203 18191a1b 1c1d1e1f\n"
        "o[25] = 10111213 14151617 00010203 1c1d1e1f\n"
        "o[26] = 00010203 14151617 18191a1b 1c1d1e1f\n"
        "o[27] = 00010203 14151617 18191a1b 1c1d1e1f\n"
        "o[28] = 10111213 00010203 18191a1b 1c1d1e1f\n"
        "o[29] = 10111213 00010203 18191a1b 1c1d1e1f\n"
        "o[30] = 10111213 00010203 18191a1b 1c1d1e1f\n"
        "o[31] = 10111213 00010203 18191a1b 1c1d1e1f\n"
        "o[32] = 00000000 00000000 00000000 00000000\n"
        "o[33] = ffffffb3 ffffffb3 ffffffb3 ffffffb3\n"
        "o[34] = ffff8000 ffff8000 ffff8000 ffff8000\n"
        "o[35] = 00007fff 00007fff 00007fff 00007fff\n"
        "o[36] = 00000000 00000000 00000000 00000000\n"
        "o[37] = 00000d05 00000d05 00000d05 00000d05\n"
        "o[38] = 0003ffff 0003ffff 0003ffff 0003ffff\n"
        "o[39] = 00000320 00000320 00000320 00000320\n"
        "o[40] = 00000000 00000000 00000000 00000000\n"
        "o[41] = ffb3ffb3 ffb3ffb3 ffb3ffb3 ffb3ffb3\n"
        "o[42] = 80008000 80008000 80008000 80008000\n"
        "o[43] = 7fff7fff 7fff7fff 7fff7fff 7fff7fff\n"
        "o[44] = 00000000 00000000 00000000 00000000\n"
        "o[45] = ffb30000 ffb30000 ffb30000 ffb30000\n"
        "o[46] = 80000000 80000000 80000000 80000000\n"
        "o[47] = 7fff0000 7fff0000 7fff0000 7fff0000\n"
        "o[48] = 00010203 04050607 08090a0b 0c0d0e0f\n"
        "o[49] = 101113f7 141517f7 18191bff 1c1d1fff\n"
        "o[50] = 20212323 24252727 28292b2b 2c2d2f2f\n"
        "o[51] = 3031ffff 3435ffff 3839ffff 3c3dffff\n"
        "o[52] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[53] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[54] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[55] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[56] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[57] = 43ffe808 127ff799 4281018c 3fe00350\n"
        "o[58] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[59] = 43ffe808 127ff799 4281018c 3fe00350\n"
        "o[60] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[61] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[62] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[63] = 1c080412 32878009 42a0c00f 3fe001d3\n"
        "o[64] = 3ee00102 24000202 340b8082 0f610102\n"
        "o[65] = 340f4082 18008182 1c080102 34000102\n"
        "o[66] = 3ee00102 24000202 340b8082 0f610102\n"
        "o[67] = 340f4082 18008182 1c080102 34000102\n"
        "o[68] = 40414243 44454647 48494a4b 4c4d4e4f\n"
        "o[69] = 50515253 54555657 58595a5b 5c5d5e5f\n"
        "o[70] = 60616263 64656667 68696a6b 6c6d6e6f\n"
        "o[71] = 70717273 74757677 78797a7b 7c7d7e7f\n"
        "o[72] = 80818283 84858687 88898a8b 8c8d8e8f\n"
        "o[73] = 90919293 94959697 98999a9b 9c9d9e9f\n"
        "o[74] = a0a1a2a3 a4a5a6a7 a8a9aaab acadaeaf\n"
        "o[75] = b0b1b2b3 b4b5b6b7 b8b9babb bcbdbebf\n"
        "o[76] = c0c1c2c3 c4c5c6c7 c8c9cacb cccdcecf\n"
        "o[77] = d0d1d2d3 d4d5d6d7 d8d9dadb dcdddedf\n"
        "o[78] = e0e1e2e3 e4e5e6e7 e8e9eaeb ecedeeef\n"
        "o[79] = f0f1f2f3 f4f5f6f7 f8f9fafb fcfdfeff\n"
        "o[80] = 00010203 04050607 08090a0b 0c0d0e0f\n"
        "o[81] = 10111213 14151617 18191a1b 1c1d1e1f\n"
        "o[82] = 20212223 24252627 28292a2b 2c2d2e2f\n"
        "o[83] = 30313233 34353637 38393a3b 3c3d3e3f\n"
        "o[84] = 33333333 33333333 33333333 33333333\n"
        "o[85] = 33333333 33333333 33333333 33333333\n"
        "o[86] = 33333333 33333333 33333333 33333333\n"
        "o[87] = 33333333 33333333 33333333 33333333\n"
        "o[88] = 33333333 33333333 33333333 33333333\n"
        "o[89] = 33333333 33333333 33333333 33333333\n"
        "o[90] = 33333333 33333333 33333333 33333333\n"
        "o[91] = 77777777 77777777 77777777 77777777\n"
        "o[92] = 77777777 77777777 77777777 77777777\n"
        "o[93] = 00000000 00000000 00000000 00000000\n"
        "o[94] = 00000000 00000000 00000000 00000000\n"
        "o[95] = 00000013 00000013 00000013 00000013\n"
        "o[96] = ffffff99 ffffff99 ffffff99 ffffff99\n"
        "o[97] = 000077ff 000077ff 000077ff 000077ff\n"
        "o[98] = ffffff77 ffffff77 ffffff77 ffffff77\n"
        "o[99] = 13131313 13131313 13131313 13131313\n"
        "o[100] = f7f7f7f7 f7f7f7f7 f7f7f7f7 f7f7f7f7\n"
        "o[101] = 00000444 00000444 00000444 00000444\n"
        "o[102] = ffaaffaa 11221122 00000000 00000000\n"
        "o[103] = ffaaffaa 11221122 00000000 00000000\n"
        "o[104] = 3de147ae 3de147ae 3de147ae 3de147ae\n"
        "o[105] = 3fc0a3d7 0a3d70a4 00000000 00000000\n"
        "o[106] = 13131313 13131313 13131313 13131313\n"
        "o[107] = 99999999 99999999 99999999 99999999\n"
        "o[108] = 77ff77ff 77ff77ff 77ff77ff 77ff77ff\n"
        "o[109] = ff77ff77 ff77ff77 ff77ff77 ff77ff77\n"
        "o[110] = 13131313 13131313 13131313 13131313\n"
        "o[111] = f7f7f7f7 f7f7f7f7 f7f7f7f7 f7f7f7f7\n"
        "o[112] = ffaaffaa 11221122 ffaaffaa 11221122\n"
        "o[113] = ffaaffaa 11221122 ffaaffaa 11221122\n"
        "o[114] = 3de147ae 3de147ae 3de147ae 3de147ae\n"
        "o[115] = 3fbc28f5 c28f5c29 3fbc28f5 c28f5c29\n"
        "o[116] = 13131313 13131313 13131313 13131313\n"
        "o[117] = 99999999 99999999 99999999 99999999\n"
        "o[118] = 77ff77ff 77ff77ff 77ff77ff 77ff77ff\n"
        "o[119] = ff77ff77 ff77ff77 ff77ff77 ff77ff77\n"
        "o[120] = 13131313 13131313 13131313 13131313\n"
        "o[121] = f7f7f7f7 f7f7f7f7 f7f7f7f7 f7f7f7f7\n"
        "o[122] = ffaaffaa 11221122 ffaaffaa 11221122\n"
        "o[123] = ffaaffaa 11221122 ffaaffaa 11221122\n"
        "o[124] = 3de147ae 3de147ae 3de147ae 3de147ae\n"
        "o[125] = 3fbc28f5 c28f5c29 3fbc28f5 c28f5c29\n"
        "o[126] = 00000000 00000000 00000000 00000000\n"
        "o[127] = 3f000000 3f000000 3f000000 3f000000\n"
        "o[128] = 3d800000 3d800000 3d800000 3d800000\n"
        "o[129] = 10111213 14151617 18191a1b 1c1d1e1f\n"
        "o[130] = 20212223 24252627 28292a2b 2c2d2e2f\n"
        "o[131] = 30313233 34353637 38393a3b 3c3d3e3f\n"
        "o[132] = 00000000 000015cf 00000003 00000000\n"
        "o[133] = 00000001 00015cf1 00000035 ffffffff\n"
        "o[134] = 00000000 0000573c 0000000d 00000000\n"
        "o[135] = 00000000 000015cf 00000003 00000000\n"
        "o[136] = 00000000 0000573c 0000000d 00000000\n"
        "o[137] = 00000000 00002b9e 00000006 00000000\n"
        "o[138] = 0037ff87 00130000 0058ff94 ff940044\n"
        "o[139] = ffff9487 ffff8100 ffff9494 ffff9444\n"
        "o[140] = ffffffff c00aaa89 00000000 301abcde\n"
        "o[141] = 3fbc28f5 c0000000 400aa3d7 00000000\n"
        "o[142] = 00000000 00000000 80000000 00000000\n"
        "o[143] = 93893140 04601100 abbb2053 fcf8abdd\n"
        "o[144] = 93893140 045f1100 abba2053 fcf7abdd\n"
        "o[145] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[146] = 00112431 44556875 8899acb9 ccddf0fd\n"
        "o[147] = 00112432 44556876 8899acba ccddf0fe\n"
        "o[148] = 00112033 44556477 8899a8bb ccddecff\n"
        "o[149] = 00112107 4455654b 8899a98f ccddedd3\n"
        "o[150] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[151] = 020f2431 46536875 8a97acb9 cedbf0fd\n"
        "o[152] = fe112033 42556477 8699a8bb caddecff\n"
        "o[153] = fee52107 4329654b 876da98f cbb1edd3\n"
        "o[154] = -3.13094e-27 851.434 8.75273e-18 -1.16357e+08\n"
        "o[155] = 93780f0d c00aaa88 23217598 301abcdd\n"
        "o[156] = 93893141 04601100 abbb2053 fcf8abdd\n"
        "o[157] = 00000001 00000001 00000001 00000000\n"
        "o[158] = 00000001 00000001 00000000 00000000\n"
        "o[159] = 00000000 00000001 00000000 00000000\n"
        "o[160] = 00000000 00000001 00000000 00000000\n"
        "o[161] = 843a4e1e 6fde5caf 6d2d6a9c 990a1966\n"
        "o[162] = -1.34873e-37 -1849.46 -1.09345e-26 -0.0655012\n"
        "o[163] = 82379487 92138100 94589494 94949444\n"
        "o[164] = 82305f7f 8100ec52 83f61b4d 8af8d2b6\n"
        "o[165] = 82415f7f c555ec52 a7171b4d bb12d2b6\n"
        "o[166] = 1.34873e-37 -1849.46 1.09345e-26 -0.0655012\n"
        "o[167] = bfb90ff9 72474538 40080000 00000000\n"
        "o[168] = 0.11 13008.7 11.0889 0.8\n"
        "o[169] = 3f88c7e2 8240b780 c0000000 00000000\n"
        "o[170] = ffdd0000 037d0000 07d80000 f3a60000\n"
        "o[171] = fff8caf8 eeed6b52 ef9d86b9 f6643e72\n"
        "o[172] = 0009caf8 33426b52 12be86b9 267e3e72\n"
        "o[173] = 0202b997 ddcadbaf d8d4d608 04758522\n"
        "o[174] = 00000000 00000000 00000000 00000000\n"
        "o[175] = ffbb9a00 ff331200 00aa8a00 00220200\n"
        "o[176] = 004443cd 00cc8789 ff55cb45 ffde0f01\n"
        "o[177] = 001ab7d8 00500cf8 ffbd6218 fff2b738\n"
        "o[178] = ffd7ec3c ff87ec8c 0063ecdc 0013ed2c\n"
        "o[179] = 0202b997 4441dbaf 4e6cd608 b0528522\n"
        "o[180] = 00000000 00000000 00000000 00000000\n"
        "o[181] = 21ee9a00 65aa1200 a9658a00 ed210200\n"
        "o[182] = 004443cd 00cc8789 0154cb45 01dd0f01\n"
        "o[183] = 001ab7d8 00500cf8 00856218 00bab738\n"
        "o[184] = 220aec3c 65feec8c a9f2ecdc ede6ed2c\n"
        "o[185] = 00000202 ffffddca ffffd8d4 00000475\n"
        "o[186] = bfbf41f2 12d77319 c0140000 00000000\n"
        "o[187] = -0 -7425.55 -7.7589 -0.9\n"
        "o[188] = 1.4458 3.94696e+30 -2.375 0\n"
        "o[189] = 9 0 0 -10\n"
        "o[190] = 1 2 3 -8\n"
        "o[191] = 3 0 1 3\n"
        "o[192] = 1 2 3 -8\n"
        "o[193] = 82267254 4dbe1a89 0bbfe9d9 c7b7a545\n"
        "o[194] = 82267254 4dbe1a89 0bbee9d9 c7b6a545\n"
        "o[195] = ffeeddcd bbaa9989 77665545 33221101\n"
        "o[196] = ffeedfcc bbaa9b88 77665744 33221300\n"
        "o[197] = ffeedbcd bbaa9789 77665345 33220f01\n"
        "o[198] = ffeedef9 bbaa9ab5 77665671 3322122d\n"
        "o[199] = ffefddcd bbab9989 77675545 33231101\n"
        "o[200] = 01eedfcc bdaa9b88 79665744 35221300\n"
        "o[201] = fdefdbcd b9ab9789 75675345 31230f01\n"
        "o[202] = 011bdef9 bcd79ab5 78935671 344f122d\n"
        "o[203] = -0.89 5580.79 0 7.9\n"
        "o[204] = 00000000 00000000 c0080000 00000000\n"
        "o[205] = 82267254 4dbe1a89 0bbee9d8 c7b6a544\n"
        "o[206] = 82267254 4e421b77 0c411627 38495abb\n"
        "o[207] = 41245b5d 6b34743c 8e799fa8 b0b9c1a2\n"
        "o[208] = 01d40066 01260176 02140286 02000396\n"
        "o[209] = 00000000 ffffffff 00000000 00000000\n"
        "o[210] = 00000000 00000000 00000000 00000000\n"
        "o[211] = 00000000 00000000 00000000 00000000\n"
        "o[212] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[213] = 00000000 00000000 00000000 00000000\n"
        "o[214] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[215] = 00000000 00000000 00000000 00000000\n"
        "o[216] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[217] = 00000000 00000000 ffffffff 00000000\n"
        "o[218] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[219] = b0b1b2b3 b4b5b6b7 b8b9babb bcbdbebf\n"
        "o[220] = c0c1c2c3 c4c5c6c7 c8c9cacb cccdcecf\n"
        "o[221] = d0d1d2d3 d4d5d6d7 d8d9dadb dcdddedf\n"
        "o[222] = e0e1e2e3 e4e5e6e7 e8e9eaeb ecedeeef\n"
        "o[223] = ff000000 00000000 00000000 00000000\n"
        "o[224] = 0000ff00 00000000 00000000 00000000\n"
        "o[225] = 00000000 00000000 00000000 00000000\n"
        "o[226] = 00000000 00000000 00000000 00000000\n"
        "o[227] = 00000000 00000000 00000000 00000000\n"
        "o[228] = 00000000 00000000 00000000 00000000\n"
        "o[229] = ff00ffff ffffffff 0000ffff ffffff00\n"
        "o[230] = ffffffff ff00ff00 ff00ffff ffffff00\n"
        "o[231] = 00000000 00ff00ff 00ffffff ffffffff\n"
        "o[232] = ffff0000 ff00ffff 000000ff 0000ffff\n"
        "o[233] = ffffffff ffffffff 0000ffff ffffffff\n"
        "o[234] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[235] = ffffffff ffffffff 00000000 ffffffff\n"
        "o[236] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[237] = 00000000 00000000 00000000 ffffffff\n"
        "o[238] = ffffffff ffffffff 00000000 00000000\n"
        "o[239] = ffffffff ffffffff ffffffff 00000000\n"
        "o[240] = 00000000 00000000 ffffffff ffffffff\n"
        "o[241] = 10111213 14151617 18191a1b 1c1d1e1f\n"
        "o[242] = 20212223 24252627 28292a2b 2c2d2e2f\n"
        "o[243] = 0000ffff ffffffff 00000000 00000000\n"
        "o[244] = 00ff0000 00000000 ffffff00 ffff0000\n"
        "o[245] = 00000000 00000000 00000000 000000ff\n"
        "o[246] = ffff0000 ff00ffff 0000ffff 0000ffff\n"
        "o[247] = 0000ffff ffffffff 00000000 00000000\n"
        "o[248] = 0000ffff 00000000 ffffffff ffff0000\n"
        "o[249] = ffffffff ffffffff 00000000 00000000\n"
        "o[250] = 00000000 00000000 ffffffff ffffffff\n"
        "o[251] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[252] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[253] = d0d1d2d3 d4d5d6d7 d8d9dadb dcdddedf\n"
        "o[254] = e0e1e2e3 e4e5e6e7 e8e9eaeb ecedeeef\n"
        "o[255] = 00020204 02040406 02040406 04060608\n"
        "o[256] = 0000000b 00000001 00000000 00000000\n"
        "o[257] = 00005555 00000000 00000000 00000000\n"
        "o[258] = 000000ff 00000000 00000000 00000000\n"
        "o[259] = 0000000f 00000000 00000000 00000000\n"
        "o[260] = 0000ff00 0000ff00 0000ffff 0000ffff\n"
        "o[261] = ffffffff ffffffff 000000ff 000000ff\n"
        "o[262] = 00000000 ffffffff 00000000 ffffffff\n"
        "o[263] = 00000000 00000000 ffffffff ffffffff\n"
        "o[264] = 82302635 c446e677 08813ebb 5859feff\n"
        "o[265] = 0078220d 440a6689 00000000 ffff8080\n"
        "o[266] = 0023ffdd ffaa0000 33110a00 937500ff\n"
        "o[267] = 00890000 000d0000 00230000 00000044\n"
        "o[268] = 00000000 00000000 00000000 00000000\n"
        "o[269] = 00100201 40002201 00012098 0018acde\n"
        "o[270] = 00110213 00110213 88998a9b 88998a9b\n"
        "o[271] = 00110033 00550077 009900bb 00dd00ff\n"
        "o[272] = 00000033 00000077 000000bb 000000ff\n"
        "o[273] = 00012032 04554476 88988a23 ccc54221\n"
        "o[274] = 6c96d2c1 7ba03301 544720dc 0338adde\n"
        "o[275] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[276] = 7dcffbfa 7ffd7fff ffffeb6f efef6bbb\n"
        "o[277] = 6c806070 2de45476 48860a63 4b614321\n"
        "o[278] = 93792f3f c45feeff abb9ffbb fcdffeff\n"
        "o[279] = 9b9bbbbb dfdfffff 9b9bbbbb dfdfffff\n"
        "o[280] = 00ff22ff 44ff66ff 88ffaaff ccffeeff\n"
        "o[281] = 001123ff 445567ff 8899abff ccddefff\n"
        "o[282] = 6c97f2f3 7ff57777 dcdfaaff cffdefff\n"
        "o[283] = ccddeeff 00000000 00000000 00000000\n"
        "o[284] = 93692d3e 845fccfe abb8df23 fcc75221\n"
        "o[285] = 9b8ab9a8 dfcefdec 13023120 57467564\n"
        "o[286] = 00ee22cc 44aa6688 8866aa44 cc22ee00\n"
        "o[287] = 001123cc 44556788 8899ab44 ccddef00\n"
        "o[288] = 08801991 22aa6677 9988abba cddceffe\n"
        "o[289] = 08911980 44556677 abb8899a cddeeffc\n"
        "o[290] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[291] = 004488cc 115599dd 2266aaee 3377bbff\n"
        "o[292] = 02244660 8aaccee8 13355771 9bbddff9\n"
        "o[293] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[294] = 00000000 00026677 0088000a 000c0000\n"
        "o[295] = 00000000 44556677 00000000 00000000\n"
        "o[296] = 00000000 00000000 00000000 00000000\n"
        "o[297] = 00000000 00000000 00000000 00000000\n"
        "o[298] = 00000000 00026677 ff88fffa fffcffff\n"
        "o[299] = 00000000 00000000 ffffffff ffffffff\n"
        "o[300] = ffffffff ffffffff ffffffff ffffffff\n"
        "o[301] = 93780f0d c00aaa89 23217598 301abcde\n"
        "o[302] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[303] = 00089119 a22ab33b c44cd55d e66ef77f\n"
        "o[304] = 00089119 a22ab33b c44cd55d e66ef77f\n"
        "o[305] = 00011223 34455667 78899aab bccddeef\n"
        "o[306] = 00089119 a22ab33b c44cd55d e66ef77f\n"
        "o[307] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[308] = 00022446 688aacce f1133557 799bbddf\n"
        "o[309] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[310] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[311] = 00001122 33445566 778899aa bbccddee\n"
        "o[312] = 00000000 00000000 00000000 00000000\n"
        "o[313] = 00000000 00000000 00000000 00112233\n"
        "o[314] = 00000000 00000000 00000000 00000000\n"
        "o[315] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[316] = 00000000 00000000 00000000 00000000\n"
        "o[317] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[318] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[319] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[320] = 00001122 33445566 778899aa bbccddee\n"
        "o[321] = 00000000 00000000 00000000 00000000\n"
        "o[322] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[323] = 089119a2 2ab33bc4 4cd55de6 6ef77f80\n"
        "o[324] = 089119a2 2ab33bc4 4cd55de6 6ef77f80\n"
        "o[325] = 01122334 45566778 899aabbc cddeeff0\n"
        "o[326] = 089119a2 2ab33bc4 4cd55de6 6ef77f80\n"
        "o[327] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[328] = 02244668 8aaccef1 13355779 9bbddfe0\n"
        "o[329] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[330] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[331] = ff001122 33445566 778899aa bbccddee\n"
        "o[332] = 778899aa bbccddee ff001122 33445566\n"
        "o[333] = 44556677 8899aabb ccddeeff 00112233\n"
        "o[334] = 778899aa bbccddee ff001122 33445566\n"
        "o[335] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[336] = ddeeff00 11223344 55667788 99aabbcc\n"
        "o[337] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[338] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[339] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[340] = ff001122 33445566 778899aa bbccddee\n"
        "o[341] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[342] = 00001980 00006677 00000000 0000eff0\n"
        "o[343] = 00000000 00000000 00000000 00000000\n"
        "o[344] = 00006000 5400ee00 11320000 00000000\n"
        "o[345] = 00000000 00000000 00000000 00000000\n"
        "o[346] = 24466000 aaccee00 bb000000 c0000000\n"
        "o[347] = 08911980 44556677 abb00000 cddeeff0\n"
        "o[348] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[349] = 00224466 88aaccee 11325576 99baddfe\n"
        "o[350] = 00000000 00000000 00000000 00000000\n"
        "o[351] = 08801980 2a803b80 4c805d80 6e807f80\n"
        "o[352] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[353] = 00224466 88aaccee 11335576 99bbddfe\n"
        "o[354] = 00000000 00000000 00000000 00000000\n"
        "o[355] = 08911980 2ab33b80 4cd55d80 6ef77f80\n"
        "o[356] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[357] = 00224466 88aaccef 11335577 99bbddfe\n"
        "o[358] = 089119a2 2ab33bc4 4cd55de6 6ef77f80\n"
        "o[359] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[360] = 089119a2 2ab33bc4 4cd55de6 6ef77f80\n"
        "o[361] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[362] = 00224466 88aaccef 11335577 99bbddfe\n"
        "o[363] = 089119a2 2ab33bc4 4cd55de6 6ef77f80\n"
        "o[364] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[365] = 089119a2 2ab33bc4 4cd55de6 6ef77f80\n"
        "o[366] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[367] = 11223344 55667788 99aabbcc ddeeff00\n"
        "o[368] = 00000000 00000000 00000000 00000000\n"
        "o[369] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[370] = 00000000 00000000 00000000 00000000\n"
        "o[371] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[372] = 11223344 55667788 99aabbcc ddeeff00\n"
        "o[373] = 00000000 00000000 00000000 00000000\n"
        "o[374] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[375] = 00000000 00000000 00000000 00000000\n"
        "o[376] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[377] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[378] = 00000000 00000000 00000000 00000000\n"
        "o[379] = 00112233 44556677 8899aabb ccddeeff\n"
        "o[380] = ff000000 00000000 00000000 00000000\n"
        "o[381] = 00000001 00000002 00000003 00000004\n"
        "o[382] = 00000001 00000002 00000003 00000004\n"
        "o[383] = f0f1f2f3 f4f5f6f7 f8f9fafb fcfdfeff\n"
        "o[384] = 00010203 04050607 08090a0b 0c0d0e0f\n"
        "o[385] = 10111213 14151617 18191a1b 1c1d1e1f\n"
        "complete";

    auto patch = [] (auto& text) {
        text = boost::regex_replace(text, boost::regex("o\\[155.*?\\n"), [] (auto line) {
            return boost::regex_replace(line.str(), boost::regex("[0-9a-f]{8}"), [] (auto num) {
                return num.str().substr(0, 7) + "*";
            });
        });
        text = boost::regex_replace(text, boost::regex("o\\[167.*?\\n"), [] (auto line) {
            return boost::regex_replace(line.str(), boost::regex("[0-9a-f]{8}"), [] (auto num) {
                return num.str().substr(0, 7) + "*";
            });
        });
        text = boost::regex_replace(text, boost::regex("o\\[187.*?\\n"), [] (auto line) {
            return boost::regex_replace(line.str(), boost::regex(" -0 "), [] (auto num) {
                return " 0 ";
            });
        });
    };

    patch(output);
    patch(expected);

    REQUIRE( output == expected );
}

TEST_CASE("spu_sync_mutex") {
    test_interpreter_and_rewriter({testPath("spu_sync_mutex/a.elf")},
         "Creating an SPU thread group.\n"
         "Initializing SPU thread 0\n"
         "Initializing SPU thread 1\n"
         "All SPU threads have been successfully initialized.\n"
         "Starting the SPU thread group.\n"
         "All SPU threads exited by sys_spu_thread_exit().\n"
         "SPU thread 0's exit status = 0\n"
         "SPU thread 1's exit status = 0\n"
         "count = 20\n"
         "message : \n"
         "## libsync : sample_sync_mutex_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("prx_simple_c") {
    auto output = startWaitGetOutput({testPath("prx_simple_c/a.elf")});
    REQUIRE( output ==
        "simple-main:start\n"
        "arg0 = 0, arg1 = 2, arg2 = 4, modres = 1\n"
        "simple-main:done\n"
    );
}

TEST_CASE("prx_call") {
    auto output = startWaitGetOutput({testPath("prx_call/a.elf")});
    REQUIRE( output ==
        "call-prx-main:start\n"
        "call_import start\n"
        "20\n"
        "call-prx-main:done\n"
    );
}

TEST_CASE("prx_library_c") {
    auto output = startWaitGetOutput({testPath("prx_library_c/a.elf")});
    REQUIRE( output ==
        "library-c-main:start\n"
        "load val=3\n"
        "load val=12\n"
        "unload val=287454118\n"
        "unload val=9\n"
        "library-c-main:done\n"
    );
}

TEST_CASE("spu_queue") {
    auto output = startWaitGetOutput({testPath("spu_queue/a.elf")});
    REQUIRE( output ==
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "source: 53505501, data: data1 [00223344,0000002c] [aabbccdd,00000000]\n"
        "event.data01 == thread id: 1\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "count = 60\n"
        "message : \n"
        "## libsync : sample_sync_mutex_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("spu_image") {
    auto output = startWaitGetOutput({testPath("spu_image/a.elf")});
    REQUIRE( output ==
        "type: 00000000\n"
        "entry point: 00003050\n"
        "nsegs: 00000004\n"
        "type: 00000001\n"
        "ls start: 00003000\n"
        "size: 00000310\n"
        "pa_start/value: 0001f100\n"
        "type: 00000001\n"
        "ls start: 00003380\n"
        "size: 00000030\n"
        "pa_start/value: 0001f480\n"
        "type: 00000002\n"
        "ls start: 000033b0\n"
        "size: 00000170\n"
        "pa_start/value: 00000000\n"
        "type: 00000004\n"
        "ls start: 00000000\n"
        "size: 00000020\n"
        "pa_start/value: 0001f4c4\n"
    );
}

TEST_CASE("fiber_hello") {
    test_interpreter_and_rewriter({testPath("fiber_hello/a.elf")},
        "Hello, fiber!\n"
        "## libfiber : sample_fiber_hello SUCCEEDED ##\n"
    );
}

TEST_CASE("ppu_threads_event_flag", TAG_SERIAL) {
    auto output = startWaitGetOutput({testPath("ppu_threads_event_flag/a.elf")});
    REQUIRE( output ==
        "p = 8008, flag = 8005\n"
        "flag value: 8000\n"
        "p = 40158118, flag = 801c\n"
        "flag value: 0\n"
        "p = 67bb8558, flag = 1d6\n"
        "flag value: 1c0\n"
        "test_event_flag: 0; p: 1740342616\n"
        "flag value (clear 0): 1c0\n"
        "flag value (clear 1): c0\n"
        "p = 17d36230, flag = cf\n"
        "flag value: ca\n"
        "test_event_flag: 0; p: 399729200\n"
    );
}

TEST_CASE("spurs_task_signal", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("spurs_task_signal/a.elf")},
        "SPU: Signal task start!\n"
        "SPU: Waiting for a signal....\n"
        "SPU: Receiving a signal succeeded.\n"
        "## libspurs : sample_spurs_task_signal SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_yield", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("spurs_task_yield/a.elf")},
        "PPU: waiting for completion of tasks\n"
        "Task#0 exited with code 0\n"
        "Task#1 exited with code 0\n"
        "Task#2 exited with code 0\n"
        "Task#3 exited with code 0\n"
        "Task#4 exited with code 0\n"
        "PPU: destroy taskset\n"
        "## libspurs : sample_spurs_yield SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_semaphore", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("spurs_task_semaphore/a.elf")},
        "PPU: waiting for completion of tasks\n"
        "Task#0 exited with code 0\n"
        "Task#1 exited with code 0\n"
        "PPU: destroy taskset\n"
        "## libspurs : sample_spurs_semaphore SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_event_flag", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("spurs_task_event_flag/a.elf")},
        "PPU: waiting for completion of tasks\n"
        "Task#0 exited with code 0\n"
        "Task#1 exited with code 0\n"
        "Task#2 exited with code 0\n"
        "Task#3 exited with code 0\n"
        "Task#4 exited with code 0\n"
        "Task#5 exited with code 0\n"
        "Task#6 exited with code 0\n"
        "Task#7 exited with code 0\n"
        "Task#8 exited with code 0\n"
        "Task#9 exited with code 0\n"
        "Task#10 exited with code 0\n"
        "Task#11 exited with code 0\n"
        "Task#12 exited with code 0\n"
        "Task#13 exited with code 0\n"
        "Task#14 exited with code 0\n"
        "Task#15 exited with code 0\n"
        "Task#16 exited with code 0\n"
        "## libspurs : sample_spurs_event_flag SUCCEEDED ##\n"
    );
}

TEST_CASE("spu_thread_group_0_sample_sync_queue", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("sample_sync_queue/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "SPU Start\n"
        "Send S\n"
        "Send C\n"
        "Send E\n"
        "Send I\n"
        "receive [SPU] Sony\n"
        "\n"
        "receive [SPU] Computer\n"
        "\n"
        "receive [SPU] Entertainment\n"
        "\n"
        "receive [SPU] Inc\n"
        "\n"
        "The SPU thread group exited by sys_spu_thread_group_exit().\n"
        "The group's exit status = 0\n"
        "## libsync : sample_sync_queue_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_sync_mutex", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("sample_sync_mutex/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Initializing SPU thread 1\n"
        "Initializing SPU thread 2\n"
        "Initializing SPU thread 3\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SPU thread 1's exit status = 0\n"
        "SPU thread 2's exit status = 0\n"
        "SPU thread 3's exit status = 0\n"
        "count = 12000\n"
        "message : \n"
        "## libsync : sample_sync_mutex_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_sync_lfqueue", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("sample_sync_lfqueue/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Initializing SPU thread 1\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "SPU Start\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SPU thread 1's exit status = 0\n"
        "## libsync : sample_sync_lfqueue_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_sync_barrier", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("sample_sync_barrier/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Initializing SPU thread 1\n"
        "Initializing SPU thread 2\n"
        "Initializing SPU thread 3\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SPU thread 1's exit status = 0\n"
        "SPU thread 2's exit status = 0\n"
        "SPU thread 3's exit status = 0\n"
        "result(0): SPU 0 reads \"SPU 1\"\n"
        "result(1): SPU 1 reads \"SPU 2\"\n"
        "result(2): SPU 2 reads \"SPU 3\"\n"
        "result(3): SPU 3 reads \"SPU 0\"\n"
        "## libsync : sample_sync_barrier_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_sheap_allocate") {
    test_interpreter_and_rewriter({testPath("sample_sheap_allocate/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "512 th PRIME NUMBER is 3671\n"
        "\n"
        "## libsheap : sample_sheap_allocate_ppu SUCCEEDED ##\n"
        "Exit PPU\n"
    );
}

TEST_CASE("sample_sheap_key_buffer") {
    test_interpreter_and_rewriter({testPath("sample_sheap_key_buffer/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "PRIME NUMBERS(512)\n"
        "\n"
        "2, 3, 5, 7, 11, 13, 17, 19, \n"
        "23, 29, 31, 37, 41, 43, 47, 53, \n"
        "59, 61, 67, 71, 73, 79, 83, 89, \n"
        "97, 101, 103, 107, 109, 113, 127, 131, \n"
        "137, 139, 149, 151, 157, 163, 167, 173, \n"
        "179, 181, 191, 193, 197, 199, 211, 223, \n"
        "227, 229, 233, 239, 241, 251, 257, 263, \n"
        "269, 271, 277, 281, 283, 293, 307, 311, \n"
        "313, 317, 331, 337, 347, 349, 353, 359, \n"
        "367, 373, 379, 383, 389, 397, 401, 409, \n"
        "419, 421, 431, 433, 439, 443, 449, 457, \n"
        "461, 463, 467, 479, 487, 491, 499, 503, \n"
        "509, 521, 523, 541, 547, 557, 563, 569, \n"
        "571, 577, 587, 593, 599, 601, 607, 613, \n"
        "617, 619, 631, 641, 643, 647, 653, 659, \n"
        "661, 673, 677, 683, 691, 701, 709, 719, \n"
        "727, 733, 739, 743, 751, 757, 761, 769, \n"
        "773, 787, 797, 809, 811, 821, 823, 827, \n"
        "829, 839, 853, 857, 859, 863, 877, 881, \n"
        "883, 887, 907, 911, 919, 929, 937, 941, \n"
        "947, 953, 967, 971, 977, 983, 991, 997, \n"
        "1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, \n"
        "1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097, \n"
        "1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, \n"
        "1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223, \n"
        "1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, \n"
        "1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321, \n"
        "1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, \n"
        "1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, \n"
        "1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, \n"
        "1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, \n"
        "1579, 1583, 1597, 1601, 1607, 1609, 1613, 1619, \n"
        "1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693, \n"
        "1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747, \n"
        "1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811, \n"
        "1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, \n"
        "1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949, \n"
        "1951, 1973, 1979, 1987, 1993, 1997, 1999, 2003, \n"
        "2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069, \n"
        "2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129, \n"
        "2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, \n"
        "2207, 2213, 2221, 2237, 2239, 2243, 2251, 2267, \n"
        "2269, 2273, 2281, 2287, 2293, 2297, 2309, 2311, \n"
        "2333, 2339, 2341, 2347, 2351, 2357, 2371, 2377, \n"
        "2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423, \n"
        "2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, \n"
        "2521, 2531, 2539, 2543, 2549, 2551, 2557, 2579, \n"
        "2591, 2593, 2609, 2617, 2621, 2633, 2647, 2657, \n"
        "2659, 2663, 2671, 2677, 2683, 2687, 2689, 2693, \n"
        "2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741, \n"
        "2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801, \n"
        "2803, 2819, 2833, 2837, 2843, 2851, 2857, 2861, \n"
        "2879, 2887, 2897, 2903, 2909, 2917, 2927, 2939, \n"
        "2953, 2957, 2963, 2969, 2971, 2999, 3001, 3011, \n"
        "3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079, \n"
        "3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, \n"
        "3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221, \n"
        "3229, 3251, 3253, 3257, 3259, 3271, 3299, 3301, \n"
        "3307, 3313, 3319, 3323, 3329, 3331, 3343, 3347, \n"
        "3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413, \n"
        "3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, \n"
        "3499, 3511, 3517, 3527, 3529, 3533, 3539, 3541, \n"
        "3547, 3557, 3559, 3571, 3581, 3583, 3593, 3607, \n"
        "3613, 3617, 3623, 3631, 3637, 3643, 3659, 3671, \n"
        "## libsheap : sample_sheap_key_buffer_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_sheap_key_mutex", TAG_SERIAL) {
    test_interpreter_and_rewriter({testPath("sample_sheap_key_mutex/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Initializing SPU thread 1\n"
        "Initializing SPU thread 2\n"
        "Initializing SPU thread 3\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SPU thread 1's exit status = 0\n"
        "SPU thread 2's exit status = 0\n"
        "SPU thread 3's exit status = 0\n"
        "ans => 4000\n"
        "## libsheap : sample_sheap_key_mutex_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("libatomic_xor64") {
    test_interpreter_and_rewriter({testPath("libatomic_xor64/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Initializing SPU thread 1\n"
        "Initializing SPU thread 2\n"
        "Initializing SPU thread 3\n"
        "Initializing SPU thread 4\n"
        "Initializing SPU thread 5\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SPU thread 1's exit status = 0\n"
        "SPU thread 2's exit status = 0\n"
        "SPU thread 3's exit status = 0\n"
        "SPU thread 4's exit status = 0\n"
        "SPU thread 5's exit status = 0\n"
        "result=0000022001e7c4e0\n"
        "## libatomic : sample_atomic_xor64 SUCCEEDED ##\n"
    );
}

TEST_CASE("libatomic_semaphore") {
    test_interpreter_and_rewriter({testPath("libatomic_semaphore/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Initializing SPU thread 1\n"
        "Initializing SPU thread 2\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SPU thread 1's exit status = 0\n"
        "SPU thread 2's exit status = 0\n"
        "result=00000bb8\n"
        "## libatomic : sample_atomic_semaphore SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_ovis_auto") {
    test_interpreter_and_rewriter({testPath("sample_ovis_auto/a.elf")},
        "spu_initialize\n"
        "elf=20d00\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "Addresses are:\n"
        "	rand_middle_square	=0xe20\n"
        "	rand_park_and_miller	=0xe70\n"
        "	quicksort	=0xdd0\n"
        "	bubblesort	=0xd80\n"
        "Ordered Arrays\n"
        "[PM x BS] 16807,36580059,282475249,613381968,620725711,1189182576,1622627973,2108642790,\n"
        "[MS x BS] -2016949479,-1234473228,-1108361218,-957849801,-229523864,-152798498,692497262,1644763201,\n"
        "[PM x QS] 61419071,806566190,907109719,1385301875,1478170937,1502283124,1527993983,1898287588,\n"
        "[MS x QS] -1088118757,-817060822,-772252656,-22634920,284221454,1113421421,1395619322,1810871737,\n"
        "[PM x BS] 278361986,1039170054,1041112346,1210493756,1275182217,1655874331,1656271693,1993998854,\n"
        "[MS x BS] -1072050635,446807488,461335667,541919755,620597854,1086011429,1280643451,1504824066,\n"
        "[PM x QS] 69178572,100624259,655861856,860648447,895601167,1122282954,1202732959,1616018834,\n"
        "[MS x QS] -1728891484,-1633311521,24513172,147624528,579004639,1266262165,1822327583,2111864724,\n"
        "[PM x BS] 36602411,158592774,346233139,441534281,948488575,996395775,1310625832,1607140360,\n"
        "[MS x BS] -1375237229,-494865538,132308376,549995884,718193186,824856623,967861897,2125474287,\n"
        "[PM x QS] 50462644,419789074,476294114,911153473,1169748548,1403584359,2017096850,2081833225,\n"
        "[MS x QS] -1715002214,27056063,120989553,450581492,854795608,1224101632,1534460071,2037116001,\n"
        "[PM x BS] 41816532,583297485,787581002,1082866564,1898450058,1932122523,1961831730,2085432757,\n"
        "[MS x BS] -1615165679,-1067017785,-600086797,-511292798,472196562,631891018,764258806,1490306408,\n"
        "[PM x QS] 76168511,217936190,249801078,548592802,746337945,924099967,1051883713,1393910145,\n"
        "[MS x QS] -1933797049,-1610013683,-882910260,528221258,625593853,1792164361,1797991859,1927439902,\n"
        "## libovis : sample_ovis_auto_ppu SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_ovis_manual") {
    test_interpreter_and_rewriter({testPath("sample_ovis_manual/a.elf")},
        "spu_initialize\n"
        "elf=20d00\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "Addresses are:\n"
        "	rand_middle_square	=0x480\n"
        "	rand_park_and_miller	=0x480\n"
        "	quicksort	=0x100\n"
        "	bubblesort	=0x100\n"
        "Ordered Arrays\n"
        "[PM x BS] 16807,	36580059,	282475249,	613381968,	620725711,	1189182576,	1622627973,	2108642790,	\n"
        "[MS x BS] -2016949479,	-1234473228,	-1108361218,	-957849801,	-229523864,	-152798498,	692497262,	1644763201,	\n"
        "[PM x QS] 61419071,	806566190,	907109719,	1385301875,	1478170937,	1502283124,	1527993983,	1898287588,	\n"
        "[MS x QS] -1088118757,	-817060822,	-772252656,	-22634920,	284221454,	1113421421,	1395619322,	1810871737,	\n"
        "[PM x BS] 278361986,	1039170054,	1041112346,	1210493756,	1275182217,	1655874331,	1656271693,	1993998854,	\n"
        "[MS x BS] -1072050635,	446807488,	461335667,	541919755,	620597854,	1086011429,	1280643451,	1504824066,	\n"
        "[PM x QS] 69178572,	100624259,	655861856,	860648447,	895601167,	1122282954,	1202732959,	1616018834,	\n"
        "[MS x QS] -1728891484,	-1633311521,	24513172,	147624528,	579004639,	1266262165,	1822327583,	2111864724,	\n"
        "[PM x BS] 36602411,	158592774,	346233139,	441534281,	948488575,	996395775,	1310625832,	1607140360,	\n"
        "[MS x BS] -1375237229,	-494865538,	132308376,	549995884,	718193186,	824856623,	967861897,	2125474287,	\n"
        "[PM x QS] 50462644,	419789074,	476294114,	911153473,	1169748548,	1403584359,	2017096850,	2081833225,	\n"
        "[MS x QS] -1715002214,	27056063,	120989553,	450581492,	854795608,	1224101632,	1534460071,	2037116001,	\n"
        "[PM x BS] 41816532,	583297485,	787581002,	1082866564,	1898450058,	1932122523,	1961831730,	2085432757,	\n"
        "[MS x BS] -1615165679,	-1067017785,	-600086797,	-511292798,	472196562,	631891018,	764258806,	1490306408,	\n"
        "[PM x QS] 76168511,	217936190,	249801078,	548592802,	746337945,	924099967,	1051883713,	1393910145,	\n"
        "[MS x QS] -1933797049,	-1610013683,	-882910260,	528221258,	625593853,	1792164361,	1797991859,	1927439902,	\n"
        "## libovis : sample_ovis_manual SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_ovis_manual_function") {
    test_interpreter_and_rewriter({testPath("sample_ovis_manual_function/a.elf")},
        "spu_initialize\n"
        "elf=20d00\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "Addresses are:\n"
        "	rand_middle_square	=0x11f8\n"
        "	rand_park_and_miller	=0x1248\n"
        "	quicksort	=0x100\n"
        "	bubblesort	=0x100\n"
        "Ordered Arrays\n"
        "[PM x BS] 16807,	36580059,	282475249,	613381968,	620725711,	1189182576,	1622627973,	2108642790,	\n"
        "[MS x BS] -2016949479,	-1234473228,	-1108361218,	-957849801,	-229523864,	-152798498,	692497262,	1644763201,	\n"
        "[PM x QS] 61419071,	806566190,	907109719,	1385301875,	1478170937,	1502283124,	1527993983,	1898287588,	\n"
        "[MS x QS] -1088118757,	-817060822,	-772252656,	-22634920,	284221454,	1113421421,	1395619322,	1810871737,	\n"
        "[PM x BS] 278361986,	1039170054,	1041112346,	1210493756,	1275182217,	1655874331,	1656271693,	1993998854,	\n"
        "[MS x BS] -1072050635,	446807488,	461335667,	541919755,	620597854,	1086011429,	1280643451,	1504824066,	\n"
        "[PM x QS] 69178572,	100624259,	655861856,	860648447,	895601167,	1122282954,	1202732959,	1616018834,	\n"
        "[MS x QS] -1728891484,	-1633311521,	24513172,	147624528,	579004639,	1266262165,	1822327583,	2111864724,	\n"
        "[PM x BS] 36602411,	158592774,	346233139,	441534281,	948488575,	996395775,	1310625832,	1607140360,	\n"
        "[MS x BS] -1375237229,	-494865538,	132308376,	549995884,	718193186,	824856623,	967861897,	2125474287,	\n"
        "[PM x QS] 50462644,	419789074,	476294114,	911153473,	1169748548,	1403584359,	2017096850,	2081833225,	\n"
        "[MS x QS] -1715002214,	27056063,	120989553,	450581492,	854795608,	1224101632,	1534460071,	2037116001,	\n"
        "[PM x BS] 41816532,	583297485,	787581002,	1082866564,	1898450058,	1932122523,	1961831730,	2085432757,	\n"
        "[MS x BS] -1615165679,	-1067017785,	-600086797,	-511292798,	472196562,	631891018,	764258806,	1490306408,	\n"
        "[PM x QS] 76168511,	217936190,	249801078,	548592802,	746337945,	924099967,	1051883713,	1393910145,	\n"
        "[MS x QS] -1933797049,	-1610013683,	-882910260,	528221258,	625593853,	1792164361,	1797991859,	1927439902,	\n"
        "## libovis : sample_ovis_manual_function SUCCEEDED ##\n"
    );
}

TEST_CASE("sample_ovis_on_spurs") {
    test_interpreter_and_rewriter({testPath("sample_ovis_on_spurs/a.elf")},
        "spu_initialize\n"
        "elf=1f900\n"
        "Addresses are:\n"
        "	rand_middle_square	=0x3400\n"
        "	rand_park_and_miller	=0x3400\n"
        "	quicksort	=0x3080\n"
        "	bubblesort	=0x3080\n"
        "Ordered Arrays\n"
        "[PM x BS] 16807,	36580059,	282475249,	613381968,	620725711,	1189182576,	1622627973,	2108642790,	\n"
        "[MS x BS] -2016949479,	-1234473228,	-1108361218,	-957849801,	-229523864,	-152798498,	692497262,	1644763201,	\n"
        "[PM x QS] 61419071,	806566190,	907109719,	1385301875,	1478170937,	1502283124,	1527993983,	1898287588,	\n"
        "[MS x QS] -1088118757,	-817060822,	-772252656,	-22634920,	284221454,	1113421421,	1395619322,	1810871737,	\n"
        "[PM x BS] 278361986,	1039170054,	1041112346,	1210493756,	1275182217,	1655874331,	1656271693,	1993998854,	\n"
        "[MS x BS] -1072050635,	446807488,	461335667,	541919755,	620597854,	1086011429,	1280643451,	1504824066,	\n"
        "[PM x QS] 69178572,	100624259,	655861856,	860648447,	895601167,	1122282954,	1202732959,	1616018834,	\n"
        "[MS x QS] -1728891484,	-1633311521,	24513172,	147624528,	579004639,	1266262165,	1822327583,	2111864724,	\n"
        "[PM x BS] 36602411,	158592774,	346233139,	441534281,	948488575,	996395775,	1310625832,	1607140360,	\n"
        "[MS x BS] -1375237229,	-494865538,	132308376,	549995884,	718193186,	824856623,	967861897,	2125474287,	\n"
        "[PM x QS] 50462644,	419789074,	476294114,	911153473,	1169748548,	1403584359,	2017096850,	2081833225,	\n"
        "[MS x QS] -1715002214,	27056063,	120989553,	450581492,	854795608,	1224101632,	1534460071,	2037116001,	\n"
        "[PM x BS] 41816532,	583297485,	787581002,	1082866564,	1898450058,	1932122523,	1961831730,	2085432757,	\n"
        "[MS x BS] -1615165679,	-1067017785,	-600086797,	-511292798,	472196562,	631891018,	764258806,	1490306408,	\n"
        "[PM x QS] 76168511,	217936190,	249801078,	548592802,	746337945,	924099967,	1051883713,	1393910145,	\n"
        "[MS x QS] -1933797049,	-1610013683,	-882910260,	528221258,	625593853,	1792164361,	1797991859,	1927439902,	\n"
        "## libovis : sample_ovis_on_spurs SUCCEEDED ##\n"
    );
}

TEST_CASE("spu_thr_mmio") {
    test_interpreter_and_rewriter({testPath("spu_thr_mmio/a.elf")},
        "Initializing SPUs\n"
        "Creating an event queue.\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Conencting SPU thread (0) to the SPU thread user event.\n"
        "Initializing SPU thread 1\n"
        "Conencting SPU thread (0x1) to the SPU thread user event.\n"
        "Initializing SPU thread 2\n"
        "Conencting SPU thread (0x2) to the SPU thread user event.\n"
        "Initializing SPU thread 3\n"
        "Conencting SPU thread (0x3) to the SPU thread user event.\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal (4) matches with the expected.\n"
        "Waiting for the SPU thread group to be terminated.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SPU thread 1's exit status = 0\n"
        "SPU thread 2's exit status = 0\n"
        "SPU thread 3's exit status = 0\n"
        "Exiting.\n"
    );
}

TEST_CASE("spu_thr_recv_event") {
    test_interpreter_and_rewriter({testPath("spu_thr_recv_event/a.elf")},
        "Initializing SPUs\n"
        "Creating an event queue.\n"
        "Creating an event port.\n"
        "Connectiong the event port and queue.\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Binding SPU thread (0) to the event queue.\n"
        "Initializing SPU thread 1\n"
        "Binding SPU thread (0x1) to the event queue.\n"
        "Initializing SPU thread 2\n"
        "Binding SPU thread (0x2) to the event queue.\n"
        "Initializing SPU thread 3\n"
        "Binding SPU thread (0x3) to the event queue.\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "Waiting for the SPU thread group to be terminated.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "The expected sum of all exit status is 300000.\n"
        "The actual sum of all exit status is 300000.\n"
        "Unbound SPU thread 0 from SPU queue number 16850944.\n"
        "Unbound SPU thread 1 from SPU queue number 16850944.\n"
        "Unbound SPU thread 2 from SPU queue number 16850944.\n"
        "Unbound SPU thread 3 from SPU queue number 16850944.\n"
        "Destroyed the SPU thread group.\n"
        "Disconnected the event port.\n"
        "Destroyed the event port.\n"
        "Destroyed the event queue.\n"
        "Exiting.\n"
    );
}

TEST_CASE("spu_thr_send_event") {
    test_interpreter_and_rewriter({testPath("spu_thr_send_event/a.elf")},
        "Initializing SPUs\n"
        "Creating an event queue.\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Conencting SPU thread (0) to the SPU thread user event.\n"
        "Initializing SPU thread 1\n"
        "Conencting SPU thread (0x1) to the SPU thread user event.\n"
        "Initializing SPU thread 2\n"
        "Conencting SPU thread (0x2) to the SPU thread user event.\n"
        "Initializing SPU thread 3\n"
        "Conencting SPU thread (0x3) to the SPU thread user event.\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "Received a user SPU thread event.\n"
        "SPU thread port number = 58\n"
        "Data = 100\n"
        "Data = 200\n"
        "\n"
        "Received a user SPU thread event.\n"
        "SPU thread port number = 58\n"
        "Data = 100\n"
        "Data = 200\n"
        "\n"
        "Received a user SPU thread event.\n"
        "SPU thread port number = 58\n"
        "Data = 100\n"
        "Data = 200\n"
        "\n"
        "Received a user SPU thread event.\n"
        "SPU thread port number = 58\n"
        "Data = 100\n"
        "Data = 200\n"
        "\n"
        "Received the terminating event.\n"
        "SPU Thread Event Handler is exiting.\n"
        "Disconnected SPU thread 0 from the user event port 58.\n"
        "Disconnected SPU thread 1 from the user event port 58.\n"
        "Disconnected SPU thread 2 from the user event port 58.\n"
        "Disconnected SPU thread 3 from the user event port 58.\n"
        "Destroyed the SPU thread group.\n"
        "Destroyed the event queue.\n"
        "Exiting.\n"
    );
}

TEST_CASE("spu_thr_dma_sync") {
    test_interpreter_and_rewriter({testPath("spu_thr_dma_sync/a.elf")},
        "Initializing SPUs\n"
        "Creating an event queue.\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Conencting SPU thread (0) to the SPU thread user event.\n"
        "Initializing SPU thread 1\n"
        "Conencting SPU thread (0x1) to the SPU thread user event.\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "PPU: Received an event. (i = 0)\n"
        "PPU: Received an event. (i = 1)\n"
        "PPU: Received an event. (i = 2)\n"
        "PPU: Received an event. (i = 3)\n"
        "PPU: Received an event. (i = 4)\n"
        "PPU: Received an event. (i = 5)\n"
        "PPU: Received an event. (i = 6)\n"
        "PPU: Received an event. (i = 7)\n"
        "PPU: Received an event. (i = 8)\n"
        "PPU: Received an event. (i = 9)\n"
        "PPU: Received an event. (i = 10)\n"
        "PPU: Received an event. (i = 11)\n"
        "PPU: Received an event. (i = 12)\n"
        "PPU: Received an event. (i = 13)\n"
        "PPU: Received an event. (i = 14)\n"
        "Dump the DMA buffer\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "\t0xfff, 0xfff, 0xfff, 0xfff\n"
        "Waiting for the SPU thread group to be terminated.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SPU thread 1's exit status = 0\n"
        "Disconnected SPU thread 0 from the user event port 58.\n"
        "Disconnected SPU thread 1 from the user event port 58.\n"
        "Destroyed the SPU thread group.\n"
        "Destroyed the event queue.\n"
        "Exiting.\n"
    );
}

TEST_CASE("ppu_spu_round_trip_ppu_side_raw_spu") {
#define ASSERT REQUIRE(output.find("has finihsed successfully.") != std::string::npos)
    std::string output;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "GETLLAR_POLLING___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "GETLLAR_POLLING___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "GETLLAR_POLLING___SPU_OUTBOUND_INTERRUPT_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "GETLLAR_POLLING___SPU_OUTBOUND_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "LLR_LOST_EVENT___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "LLR_LOST_EVENT___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "LLR_LOST_EVENT___SPU_OUTBOUND_INTERRUPT_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "LLR_LOST_EVENT___SPU_OUTBOUND_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "SIGNAL_NOTIFICATION___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "SIGNAL_NOTIFICATION___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "SIGNAL_NOTIFICATION___SPU_OUTBOUND_INTERRUPT_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "SIGNAL_NOTIFICATION___SPU_OUTBOUND_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "SPU_INBOUND_MAILBOX___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "SPU_INBOUND_MAILBOX___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "SPU_INBOUND_MAILBOX___SPU_OUTBOUND_INTERRUPT_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_raw_spu/"
                                 "SPU_INBOUND_MAILBOX___SPU_OUTBOUND_MAILBOX/a.elf")});
    ASSERT;
#undef ASSERT
}

TEST_CASE("ppu_spu_round_trip_ppu_side_spu_thread") {
#define ASSERT REQUIRE(output.find("has finihsed successfully.") != std::string::npos)
    std::string output;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "GETLLAR_POLLING___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "GETLLAR_POLLING___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "GETLLAR_POLLING___EVENT_QUEUE_SEND/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "GETLLAR_POLLING___EVENT_QUEUE_THROW/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "LLR_LOST_EVENT___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "LLR_LOST_EVENT___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "LLR_LOST_EVENT___EVENT_QUEUE_SEND/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "LLR_LOST_EVENT___EVENT_QUEUE_THROW/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "SIGNAL_NOTIFICATION___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "SIGNAL_NOTIFICATION___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "SIGNAL_NOTIFICATION___EVENT_QUEUE_SEND/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/ppu_side_spu_thread/"
                                 "SIGNAL_NOTIFICATION___EVENT_QUEUE_THROW/a.elf")});
    ASSERT;
#undef ASSERT
}

TEST_CASE("ppu_spu_round_trip_spu_side_raw_spu") {
#define ASSERT REQUIRE(output.find("has finihsed successfully.") != std::string::npos)
    std::string output;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "GETLLAR_POLLING___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "GETLLAR_POLLING___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "GETLLAR_POLLING___SPU_OUTBOUND_INTERRUPT_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "GETLLAR_POLLING___SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "GETLLAR_POLLING___SPU_OUTBOUND_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "SIGNAL_NOTIFICATION___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "SIGNAL_NOTIFICATION___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "SIGNAL_NOTIFICATION___SPU_OUTBOUND_INTERRUPT_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "SIGNAL_NOTIFICATION___SPU_OUTBOUND_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "SPU_INBOUND_MAILBOX___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "SPU_INBOUND_MAILBOX___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "SPU_INBOUND_MAILBOX___SPU_OUTBOUND_INTERRUPT_MAILBOX/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_raw_spu/"
                                 "SPU_INBOUND_MAILBOX___SPU_OUTBOUND_MAILBOX/a.elf")});
    ASSERT;
#undef ASSERT
}

TEST_CASE("ppu_spu_round_trip_spu_side_spu_thread") {
#define ASSERT REQUIRE(output.find("has finihsed successfully.") != std::string::npos)
    std::string output;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_spu_thread/"
                                 "GETLLAR_POLLING___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_spu_thread/"
                                 "GETLLAR_POLLING___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_spu_thread/"
                                 "GETLLAR_POLLING___EVENT_QUEUE_SEND/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_spu_thread/"
                                 "GETLLAR_POLLING___EVENT_QUEUE_THROW/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_spu_thread/"
                                 "SIGNAL_NOTIFICATION___ATOMIC_PUTLLUC/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_spu_thread/"
                                 "SIGNAL_NOTIFICATION___DMA_PUT/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_spu_thread/"
                                 "SIGNAL_NOTIFICATION___EVENT_QUEUE_SEND/a.elf")});
    ASSERT;
    output = startWaitGetOutput({testPath("ppu_spu_round_trip/output/spu_side_spu_thread/"
                                 "SIGNAL_NOTIFICATION___EVENT_QUEUE_THROW/a.elf")});
    ASSERT;
#undef ASSERT
}

TEST_CASE("spurs_iwl_communication1_event_flag") {
    test_interpreter_and_rewriter({testPath("spurs_iwl_communication1_event_flag/a.elf")},
        "PPU: wait until all task becomes ready\n"
        "PPU: wake up setter tasks\n"
        "PPU: joining all tasks\n"
        "PPU: destroying taskset\n"
        "## libspurs : sample_spurs_iwl_event_flag SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_hello2") {
    auto output = startWaitGetOutput({testPath("spurs_task_hello2/a.elf")});
    REQUIRE( output ==
        "PPU: wait for completion of the task\n"
        "Total sum = 7116240\n"
        "PPU: taskset completed\n"
        "## libspurs : sample_spurs_hello SUCCEEDED ##\n"
    );
}

TEST_CASE("l10n_convert_str") {
    test_interpreter_and_rewriter({testPath("l10n_convert_str/a.elf"), "CODEPAGE_1251", "UTF8"},
        "cellSysmoduleLoadModule() : 0\n"
        "l10n_convert_str: CODEPAGE_1251 0xe0 0xe1 0xe2 => UTF8 0xd0 0xb0 0xd0 0xb1 0xd0 0xb2\n"
    );
}

TEST_CASE("spu_dma_polling") {
    test_interpreter_and_rewriter({testPath("spu_dma_polling/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "## libdma : sample_dma_polling SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_queue") {
    auto output = startWaitGetOutput({testPath("spurs_task_queue/a.elf")});
    REQUIRE( output ==
        "PPU: waiting for completion of tasks\n"
        "Task#0 exited with code 0\n"
        "Task#1 exited with code 0\n"
        "Task#2 exited with code 0\n"
        "Task#3 exited with code 0\n"
        "Task#4 exited with code 0\n"
        "Task#5 exited with code 0\n"
        "## libspurs : sample_spurs_queue SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_poll") {
    auto output = startWaitGetOutput({testPath("spurs_task_poll/a.elf")});
    REQUIRE( output ==
        "PPU: waiting for completion of tasks\n"
        "Task#0 exited with code 0\n"
        "Task#0 exited with code 0\n"
        "PPU: destroy taskset\n"
        "PPU: destroy taskset\n"
        "## libspurs : sample_spurs_poll SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_lfqueue") {
    auto output = startWaitGetOutput({testPath("spurs_task_lfqueue/a.elf")});
    REQUIRE( output ==
        "PPU: waiting for completion of sender/receiver PPU threads\n"
        "PPU: waiting for completion of tasks\n"
        "Task#0 exited with code 0\n"
        "Task#1 exited with code 0\n"
        "PPU: taskset completed\n"
        "PPU: sample_spurs_lfqueue finished.\n"
        "## libspurs : sample_spurs_lfqueue SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_barrier") {
    auto output = startWaitGetOutput({testPath("spurs_task_barrier/a.elf")});
    REQUIRE( output ==
        "PPU: waiting for completion of tasks\n"
        "Task#0 exited with code 0\n"
        "Task#1 exited with code 0\n"
        "Task#2 exited with code 0\n"
        "Task#3 exited with code 0\n"
        "Task#4 exited with code 0\n"
        "Task#5 exited with code 0\n"
        "Task#6 exited with code 0\n"
        "Task#7 exited with code 0\n"
        "Task#8 exited with code 0\n"
        "Task#9 exited with code 0\n"
        "Task#10 exited with code 0\n"
        "Task#11 exited with code 0\n"
        "Task#12 exited with code 0\n"
        "Task#13 exited with code 0\n"
        "Task#14 exited with code 0\n"
        "Task#15 exited with code 0\n"
        "PPU: destroy taskset\n"
        "## libspurs : sample_spurs_barrier SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_create_on_task") {
    test_interpreter_and_rewriter({testPath("spurs_task_create_on_task/a.elf")},
        "PPU: wait for completion of the task\n"
        "SPU: Task create task start!\n"
        "Task#0 exited with code 0\n"
        "PPU: taskset completed\n"
        "## libspurs : sample_create_on_task SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_job_inout_dma") {
    test_interpreter_and_rewriter({testPath("spurs_job_inout_dma/a.elf")},
        "00031700 dstBuffer[0]=1\n"
        "00031710 dstBuffer[1]=2\n"
        "00031720 dstBuffer[2]=3\n"
        "00031730 dstBuffer[3]=4\n"
        "00031740 dstBuffer[4]=5\n"
        "00031750 dstBuffer[5]=6\n"
        "00031760 dstBuffer[6]=7\n"
        "00031770 dstBuffer[7]=8\n"
        "00031780 dstBuffer[8]=9\n"
        "00031790 dstBuffer[9]=a\n"
        "000317a0 dstBuffer[10]=b\n"
        "000317b0 dstBuffer[11]=c\n"
        "000317c0 dstBuffer[12]=d\n"
        "000317d0 dstBuffer[13]=e\n"
        "000317e0 dstBuffer[14]=f\n"
        "000317f0 dstBuffer[15]=10\n"
        "00031800 dstBuffer[16]=11\n"
        "00031810 dstBuffer[17]=12\n"
        "00031820 dstBuffer[18]=13\n"
        "00031830 dstBuffer[19]=14\n"
        "00031840 dstBuffer[20]=15\n"
        "00031850 dstBuffer[21]=16\n"
        "00031860 dstBuffer[22]=17\n"
        "00031870 dstBuffer[23]=18\n"
        "00031880 dstBuffer[24]=19\n"
        "00031890 dstBuffer[25]=1a\n"
        "000318a0 dstBuffer[26]=1b\n"
        "000318b0 dstBuffer[27]=1c\n"
        "000318c0 dstBuffer[28]=1d\n"
        "000318d0 dstBuffer[29]=1e\n"
        "000318e0 dstBuffer[30]=1f\n"
        "000318f0 dstBuffer[31]=20\n"
        "00031900 dstBuffer[32]=21\n"
        "00031910 dstBuffer[33]=22\n"
        "00031920 dstBuffer[34]=23\n"
        "00031930 dstBuffer[35]=24\n"
        "00031940 dstBuffer[36]=25\n"
        "00031950 dstBuffer[37]=26\n"
        "00031960 dstBuffer[38]=27\n"
        "00031970 dstBuffer[39]=28\n"
        "00031980 dstBuffer[40]=29\n"
        "00031990 dstBuffer[41]=2a\n"
        "000319a0 dstBuffer[42]=2b\n"
        "000319b0 dstBuffer[43]=2c\n"
        "000319c0 dstBuffer[44]=2d\n"
        "000319d0 dstBuffer[45]=2e\n"
        "000319e0 dstBuffer[46]=2f\n"
        "000319f0 dstBuffer[47]=30\n"
        "00031a00 dstBuffer[48]=31\n"
        "00031a10 dstBuffer[49]=32\n"
        "00031a20 dstBuffer[50]=33\n"
        "00031a30 dstBuffer[51]=34\n"
        "00031a40 dstBuffer[52]=35\n"
        "00031a50 dstBuffer[53]=36\n"
        "00031a60 dstBuffer[54]=37\n"
        "00031a70 dstBuffer[55]=38\n"
        "00031a80 dstBuffer[56]=39\n"
        "00031a90 dstBuffer[57]=3a\n"
        "00031aa0 dstBuffer[58]=3b\n"
        "00031ab0 dstBuffer[59]=3c\n"
        "00031ac0 dstBuffer[60]=3d\n"
        "00031ad0 dstBuffer[61]=3e\n"
        "00031ae0 dstBuffer[62]=3f\n"
        "00031af0 dstBuffer[63]=40\n"
        "00031b00 dstBuffer[64]=41\n"
        "00031b10 dstBuffer[65]=42\n"
        "00031b20 dstBuffer[66]=43\n"
        "00031b30 dstBuffer[67]=44\n"
        "00031b40 dstBuffer[68]=45\n"
        "00031b50 dstBuffer[69]=46\n"
        "00031b60 dstBuffer[70]=47\n"
        "00031b70 dstBuffer[71]=48\n"
        "00031b80 dstBuffer[72]=49\n"
        "00031b90 dstBuffer[73]=4a\n"
        "00031ba0 dstBuffer[74]=4b\n"
        "00031bb0 dstBuffer[75]=4c\n"
        "00031bc0 dstBuffer[76]=4d\n"
        "00031bd0 dstBuffer[77]=4e\n"
        "00031be0 dstBuffer[78]=4f\n"
        "00031bf0 dstBuffer[79]=50\n"
        "00031c00 dstBuffer[80]=51\n"
        "00031c10 dstBuffer[81]=52\n"
        "00031c20 dstBuffer[82]=53\n"
        "00031c30 dstBuffer[83]=54\n"
        "00031c40 dstBuffer[84]=55\n"
        "00031c50 dstBuffer[85]=56\n"
        "00031c60 dstBuffer[86]=57\n"
        "00031c70 dstBuffer[87]=58\n"
        "00031c80 dstBuffer[88]=59\n"
        "00031c90 dstBuffer[89]=5a\n"
        "00031ca0 dstBuffer[90]=5b\n"
        "00031cb0 dstBuffer[91]=5c\n"
        "00031cc0 dstBuffer[92]=5d\n"
        "00031cd0 dstBuffer[93]=5e\n"
        "00031ce0 dstBuffer[94]=5f\n"
        "00031cf0 dstBuffer[95]=60\n"
        "00031d00 dstBuffer[96]=61\n"
        "00031d10 dstBuffer[97]=62\n"
        "00031d20 dstBuffer[98]=63\n"
        "00031d30 dstBuffer[99]=64\n"
        "00031d40 dstBuffer[100]=65\n"
        "00031d50 dstBuffer[101]=66\n"
        "00031d60 dstBuffer[102]=67\n"
        "00031d70 dstBuffer[103]=68\n"
        "00031d80 dstBuffer[104]=69\n"
        "00031d90 dstBuffer[105]=6a\n"
        "00031da0 dstBuffer[106]=6b\n"
        "00031db0 dstBuffer[107]=6c\n"
        "00031dc0 dstBuffer[108]=6d\n"
        "00031dd0 dstBuffer[109]=6e\n"
        "00031de0 dstBuffer[110]=6f\n"
        "00031df0 dstBuffer[111]=70\n"
        "00031e00 dstBuffer[112]=71\n"
        "00031e10 dstBuffer[113]=72\n"
        "00031e20 dstBuffer[114]=73\n"
        "00031e30 dstBuffer[115]=74\n"
        "00031e40 dstBuffer[116]=75\n"
        "00031e50 dstBuffer[117]=76\n"
        "00031e60 dstBuffer[118]=77\n"
        "00031e70 dstBuffer[119]=78\n"
        "00031e80 dstBuffer[120]=79\n"
        "00031e90 dstBuffer[121]=7a\n"
        "00031ea0 dstBuffer[122]=7b\n"
        "00031eb0 dstBuffer[123]=7c\n"
        "00031ec0 dstBuffer[124]=7d\n"
        "00031ed0 dstBuffer[125]=7e\n"
        "00031ee0 dstBuffer[126]=7f\n"
        "00031ef0 dstBuffer[127]=80\n"
        "## libspurs : sample_spurs_job_inout_dma SUCCEEDED ##\n"
    );
}

TEST_CASE("libswcache_linked_list_sort") {
    auto output = startWaitGetOutput({testPath("libswcache_linked_list_sort/a.elf")});
    REQUIRE( output ==
        "(1) ### Sort linked list by PPU ###\n"
        "Linked list by PatchedObject\n"
        "verify linked list sort by PatchObject\n"
        "Linked list by Pointer\n"
        "verify linked list sort by Pointer\n"
        "(2) ### Sort linked list by SPU ###\n"
        "Linked list by PatchObject\n"
        "Linked list by Pointer\n"
        "verify linked list sort by PatchObject\n"
        "verify linked list sort by Pointer\n"
        "## libswcache : sample_swcache_linked_list_sort SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_task_hello3_exit_if_no_work") {
    test_interpreter_and_rewriter({testPath("spurs_task_hello3_exit_if_no_work/a.elf")},
        "PPU: wait for completion of the task\n"
        "res = 80\n"
        "PPU: taskset completed\n"
        "## libspurs : sample_spurs_hello SUCCEEDED ##\n"
    );
}

TEST_CASE("ppu_threads_mutex_errors") {
    auto output = startWaitGetOutput({testPath("ppu_threads_mutex_errors/a.elf")});
    REQUIRE( output ==
        "create RECURSIVE LOCK\n"
        "8001000d\n"
        "8001000d\n"
        "8001000d\n"
        "0\n"
        "locking\n"
        "0\n"
        "0\n"
        "destroying\n"
        "80010005\n"
        "8001000a\n"
        "8001000a\n"
        "unlocking\n"
        "80010005\n"
        "0\n"
        "0\n"
        "80010009\n"
        "destroying\n"
        "0\n"
        "80010005\n"
        "create NOT_RECURSIVE LOCK\n"
        "8001000d\n"
        "8001000d\n"
        "8001000d\n"
        "0\n"
        "locking\n"
        "0\n"
        "80010008\n"
        "destroying\n"
        "80010005\n"
        "8001000a\n"
        "8001000a\n"
        "unlocking\n"
        "80010005\n"
        "0\n"
        "80010009\n"
        "80010009\n"
        "destroying\n"
        "0\n"
        "80010005\n"
        "create RECURSIVE TRYLOCK\n"
        "8001000d\n"
        "8001000d\n"
        "8001000d\n"
        "0\n"
        "locking\n"
        "0\n"
        "0\n"
        "destroying\n"
        "80010005\n"
        "8001000a\n"
        "8001000a\n"
        "unlocking\n"
        "80010005\n"
        "0\n"
        "0\n"
        "80010009\n"
        "destroying\n"
        "0\n"
        "80010005\n"
        "create NOT_RECURSIVE TRYLOCK\n"
        "8001000d\n"
        "8001000d\n"
        "8001000d\n"
        "0\n"
        "locking\n"
        "0\n"
        "80010008\n"
        "destroying\n"
        "80010005\n"
        "8001000a\n"
        "8001000a\n"
        "unlocking\n"
        "80010005\n"
        "0\n"
        "80010009\n"
        "80010009\n"
        "destroying\n"
        "0\n"
        "80010005\n"
        "create LW RECURSIVE LOCK\n"
        "0\n"
        "locking\n"
        "0\n"
        "0\n"
        "destroying\n"
        "8001000a\n"
        "8001000a\n"
        "unlocking\n"
        "0\n"
        "0\n"
        "80010009\n"
        "destroying\n"
        "0\n"
        "80010002\n"
        "create LW NOT_RECURSIVE LOCK\n"
        "0\n"
        "locking\n"
        "0\n"
        "80010008\n"
        "destroying\n"
        "8001000a\n"
        "8001000a\n"
        "unlocking\n"
        "0\n"
        "80010009\n"
        "80010009\n"
        "destroying\n"
        "0\n"
        "80010002\n"
        "create LW RECURSIVE TRYLOCK\n"
        "0\n"
        "locking\n"
        "0\n"
        "0\n"
        "destroying\n"
        "8001000a\n"
        "8001000a\n"
        "unlocking\n"
        "0\n"
        "0\n"
        "80010009\n"
        "destroying\n"
        "0\n"
        "80010002\n"
        "create LW NOT_RECURSIVE TRYLOCK\n"
        "0\n"
        "locking\n"
        "0\n"
        "80010008\n"
        "destroying\n"
        "8001000a\n"
        "8001000a\n"
        "unlocking\n"
        "0\n"
        "80010009\n"
        "80010009\n"
        "destroying\n"
        "0\n"
        "80010002\n"
        "done\n"
    );
}

TEST_CASE("lwarx_stwcx") {
    test_interpreter_and_rewriter({testPath("lwarx_stwcx/a.elf")},
        "loc address 20c00\n"
        "cache line 20c00\n"
        "simple incrementing\n"
        "loc = {1, 0}\n"
        "incrementing with modification of unrelated location on the same granule\n"
        "loc = {2, undefined} is b > 0: 1\n"
        "creating reservation for a, but storing to b\n"
        "loc = {0, 7}\n"
        "creating reservation for a, then doing regular store to a\n"
        "loc = {10, 7}\n"
    );
}

TEST_CASE("spu_generic_test2") {
    test_interpreter_and_rewriter({testPath("spu_generic_test2/a.elf")},
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "message: ! 10 !\n"
        "o[0] = 00000000 00000000 43fa0000 c3fa0000\n"
        "o[1] = 00000000 00000000 437a0000 c37a0000\n"
        "o[2] = 00000000 00000000 42fa0000 c2fa0000\n"
        "o[3] = 00000000 00000000 41fa0000 c1fa0000\n"
        "o[4] = 00000000 0000000a fffffff6 000000c8\n"
        "o[5] = 00000000 00000014 ffffffec 00000190\n"
        "o[6] = 00000000 00000028 ffffffd8 00000320\n"
        "o[7] = 00000000 7fffffff 80000000 7fffffff\n"
        "o[8] = 00000000 00000000 43fa0000 4f7ffffe\n"
        "o[9] = 00000000 00000000 437a0000 4efffffe\n"
        "o[10] = 00000000 00000000 42fa0000 4e7ffffe\n"
        "o[11] = 00000000 00000000 41fa0000 4d7ffffe\n"
        "o[12] = 00000000 000f4240 00000000 ffffffff\n"
        "o[13] = 00000000 001e8480 00000000 ffffffff\n"
        "o[14] = 00000000 003d0900 00000000 ffffffff\n"
        "o[15] = 00000000 0f424000 00000000 ffffffff\n"
        "complete"
    );
}

TEST_CASE("sysutil_save_basic_auto_only", TAG_SERIAL) {
    auto savedir = testPath("sysutil_save_basic_auto_only/SAVEDATA");
    std::filesystem::remove_all(savedir);
    auto output = startWaitGetOutput({testPath("sysutil_save_basic_auto_only/USRDIR/a.elf")});
    REQUIRE( output ==
        "thr_auto_save() start\n"
        "cb_data_status_save() start\n"
        "Dump CellSaveDataStatGet in CellSaveDataStatCallback--------------------\n"
        "	get->dir.dirName : ABCD00002-AUTO-\n"
        "	get->isNewData: 1\n"
        "	get->sizeKB : 0x0 MB\n"
        "	get->dir : dirName : ABCD00002-AUTO-\n"
        "	get->fileListNum: 0\n"
        "	get->fileNum: 0\n"
        "\n"
        "	PARAM.SFO:TITLE: \n"
        "	PARAM.SFO:SUB_TITLE: \n"
        "	PARAM.SFO:DETAIL: \n"
        "	PARAM.SFO:ATTRIBUTE: 0\n"
        "	PARAM.SFO:LIST_PARAM: \n"
        "\n"
        "Data ABCD00002-AUTO- is not exist yet.\n"
        "result->result=[0]\n"
        "cb_data_status_save() end\n"
        "cb_file_operation_save() start\n"
        "saving ICON0.PNG ...\n"
        "cb_file_operation_save() end\n"
        "cb_file_operation_save() start\n"
        "saved ICON0.PNG size : 78504\n"
        "saving ICON1.PAM ...\n"
        "cb_file_operation_save() end\n"
        "cb_file_operation_save() start\n"
        "saved ICON1.PMF size : 2631680\n"
        "saving PIC1.PNG ...\n"
        "cb_file_operation_save() end\n"
        "cb_file_operation_save() start\n"
        "saved PIC1.PNG size : 715557\n"
        "saving SYS-DATA ...\n"
        "cb_file_operation_save() end\n"
        "cb_file_operation_save() start\n"
        "saved SYS-DATA size : 2345\n"
        "saving USR-DATA ...\n"
        "cb_file_operation_save() end\n"
        "cb_file_operation_save() start\n"
        "saved USR-DATA size : 678\n"
        "SAVE_OPERATION_STEP_END\n"
        "cb_file_operation_save() end\n"
        "cellSaveDataAutoSave2() : 0x0\n"
        "thr_auto_save() end\n"
        "thr_auto_load() start\n"
        "cb_data_status_load() start\n"
        "Dump CellSaveDataStatGet in CellSaveDataStatCallback--------------------\n"
        "	get->dir.dirName : ABCD00002-AUTO-\n"
        "	get->isNewData: 0\n"
        "	get->sizeKB : 0x3 MB\n"
        "	get->dir : dirName : ABCD00002-AUTO-\n"
        "	get->fileListNum: 5\n"
        "	  0  FILENAME: ICON0.PNG   type : 2  size : 78504\n"
        "	  1  FILENAME: ICON1.PAM   type : 3  size : 2631680\n"
        "	  2  FILENAME: PIC1.PNG   type : 4  size : 715557\n"
        "	  3  FILENAME: SYS-DATA   type : 0  size : 2345\n"
        "	  4  FILENAME: USR-DATA   type : 0  size : 678\n"
        "	get->fileNum: 5\n"
        "\n"
        "	PARAM.SFO:TITLE: SYSUTIL SAVEDATA SAMPLE\n"
        "	PARAM.SFO:SUB_TITLE: History\n"
        "	PARAM.SFO:DETAIL: This data was saved by sysutil savedata sample.\n"
        "	PARAM.SFO:ATTRIBUTE: 0\n"
        "	PARAM.SFO:LIST_PARAM: CONFIG\n"
        "\n"
        "Data exist. fileListNum=[5] fileNum=[5]\n"
        "found [SYS-DATA]\n"
        "found [USR-DATA]\n"
        "result->result=[0]\n"
        "cb_data_status_load() end\n"
        "cb_file_operation_load() start\n"
        "cb_file_operation_load() end\n"
        "cb_file_operation_load() start\n"
        "cb_file_operation_load() end\n"
        "cb_file_operation_load() start\n"
        "SAVE_OPERATION_STEP_END\n"
        "cb_file_operation_load() end\n"
        "cellSaveDataAutoLoad2() : 0x0\n"
        "thr_auto_load() end\n"
    );
}

TEST_CASE("ppu_mem_alloc") {
    test_interpreter_and_rewriter({testPath("ppu_mem_alloc/a.elf")},
        "at lease 180 mb have been allocated\n"
    );
}

TEST_CASE("spurs_sample_jobqueue_sync_command") {
    test_interpreter_and_rewriter({testPath("spurs_sample_jobqueue_sync_command/a.elf")},
        "*** sync with sync() method \n"
        "*** sync with SYNC command with multiple tags\n"
        "## libspurs : sample_spurs_jobqueue_sync_command SUCCEEDED ##\n"
    );
}

TEST_CASE("spurs_sample_jobqueue_hello") {
    test_interpreter_and_rewriter({testPath("spurs_sample_jobqueue_hello/a.elf")},
        "Hello, jobqueue!\n"
    );
}

TEST_CASE("ppu_simd_generic") {
    test_interpreter_and_rewriter({testPath("ppu_simd_generic/a.elf")},
        "0: 00140032 0bba1257 f59bdc8c 7fff0000\n"
        "1: 01000005 0000ffff 0001000a 141effff\n"
        "2: 00010001 00010001 00010001 00010001\n"
        "3: 00080008 00080008 00080008 00080008\n"
        "4: 00030003 00030003 00030003 00030003\n"
        "5: 00060006 00060006 00060006 00060006\n"
        "6: 00050005 00050005 00050005 00050005\n"
        "7: 000a14ff 00ffff00 00ffff00 0000ff00\n"
        "8: 00000010 fc171ffe 06a013b8 017cfed4\n"
        "9: 00000002 80008000 03500030 0050c000\n"
        "10: 00008000 ff008000 03503000 00000c00\n"
        "11: 000000a0 0140fa00 f830e200 79600000\n"
        "12: 00000004 fff8fff0 6a000c00 28000c00\n"
        "13: 00010004 80000000 00050006 00078000\n"
        "14: 00000001 000a0014 fffffffe fff6ffec\n"
        "15: 00010001 00020002 00030003 00040005\n"
        "16: 00000000 ffff0000 03500030 0050ffff\n"
        "17: 00000000 ffff0000 03500000 0000fffe\n"
        "18: 00000000 00010003 ff83004e 79600000\n"
        "19: 00000000 ffff00ff 001a0000 0000fffe\n"
        "20: 00010001 00000000 00050006 00070000\n"
        "21: 4a48484a 4a4a4aff 00000000 00000000\n"
        "22: fffffffe fffdfffd ffffffff fffefffd\n"
        "23: 00010a14 fffef6ec 01010202 03030405\n"
        "24: 00010a14 fffef6ec 01010202 03030405\n"
        "25: 00010a14 fffef6ec 01010202 03030405\n"
        "26: 00000000 00000000 00000000 00000000\n"
        "27: 00000000 00000000 00000000 00000000\n"
        "28: 00010a14 fffef6ec 01010202 03030405\n"
        "29: 00010a14 fffef6ec 01010202 03030405\n"
        "30: 00010a14 fffef6ec 01010202 03030405\n"
        "31: 00000000 00000000 00000000 00000000\n"
        "32: 00000000 00000000 00000000 00000000\n"
    );
}

TEST_CASE("gcm_cube_tls_callback") {
    // also checks gcmFlush/flipCallback synchronization, testing for a possible deadlock
    test_interpreter_and_rewriter({testPath("gcm_cube_tls_callback/a.elf")},
        "tls int: 201\n"
        "tls int: 202\n"
        "tls int: 203\n"
        "tls int: 204\n"
        "tls int: 205\n"
        "tls int: 206\n"
    );
}

TEST_CASE("performance_tips_advanced_merge_sort") {
    test_interpreter_and_rewriter({testPath("performance_tips_advanced_merge_sort/a.elf")},
        "Initilaize Sort Data ... Done\n"
        "initializing communication\n"
        "Check Sorted Data ... done\n"
        "SPURS task finished\n"
    );
}

TEST_CASE("spu_thr_embed_img") {
    test_interpreter_and_rewriter({testPath("spu_thr_embed_img/a.elf")},
        "start=21c80 end=22054 size=3d4\n"
        "Initializing SPUs\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "Waiting for the SPU thread group to be terminated.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 17\n"
        "Exiting.\n"
    );
}

TEST_CASE("ppu_threads_mutex_cond2") {
    test_interpreter_and_rewriter({testPath("ppu_threads_mutex_cond2/a.elf")},
        "test_all: ok=4 timeout=0\n"
        "test_one: ok=1 timeout=3\n"
        "test_to_3(ok): e1=0 e2=0 e3=1 e4=0\n"
    );
}

TEST_CASE("ppu_threads_event_queue") {
    test_interpreter_and_rewriter({testPath("ppu_threads_event_queue/a.elf")},
        "done\n"
    );
}

TEST_CASE("ppu_threads_event_queue2") {
    test_interpreter_and_rewriter({testPath("ppu_threads_event_queue2/a.elf")},
        "done 0: 1317 1 2 3\n"
        "done 0: expected=1 1 2 3\n"
    );
}

TEST_CASE("ppu_threads_event_queue3") {
    test_interpreter_and_rewriter({testPath("ppu_threads_event_queue3/a.elf")},
        "ret=0\n"
        "ret=0\n"
        "ret=0\n"
        "ret=8001000a\n"
        "ret=8001000a\n"
        "ret=8001000a\n"
        "ret=0 (drain)\n"
        "ret=0\n"
    );
}

TEST_CASE("spu_thread_group_1") {
    test_interpreter_and_rewriter({testPath("spu_thread_group_1/a.elf")},
        "Initializing SPUs\n"
        "Creating an event queue.\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Conencting SPU thread (0) to the SPU thread user event.\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Starting the SPU thread group.\n"
        "Waiting for an SPU thread event\n"
        "The resultant signal = 13.\n"
        "All SPU thread exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 3\n"
        "Exiting.\n"
    );
}

TEST_CASE("spu_thread_group_2") {
    test_interpreter_and_rewriter({testPath("spu_thread_group_2/a.elf")},
        "Initializing SPUs\n"
        "Creating an event queue.\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Initializing SPU thread 1\n"
        "Initializing SPU thread 2\n"
        "Initializing SPU thread 3\n"
        "All SPU threads have been successfully initialized.\n"
        "\n"
        "Exiting.\n"
    );
}

TEST_CASE("spu_thread_group_5") {
    test_interpreter_and_rewriter({testPath("spu_thread_group_5/a.elf")},
        "Initializing SPUs\n"
        "Creating an event queue.\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Exiting.\n"
    );
}

TEST_CASE("ps3call_tests") {
    test_interpreter_and_rewriter({testPath("ps3call_tests/a.elf")},
        "main\n"
        "stolen ps3call_tests(107c4, 10744, 106f4)\n"
        "calling simple(5,7)\n"
        "simple_cb(5, 7)\n"
        "returned 12\n"
        "[after a ps3call] stolen ps3call_tests(107c4, 10744, 106f4)\n"
        "calling recursive(11)\n"
        "calling (from recursive child) simple(10,20)\n"
        "simple_cb(10, 20)\n"
        "simple returned 30\n"
        "recursive child returned 13\n"
        "recursive returned 35\n"
        "done\n"
    );
}

TEST_CASE("function_splicing") {
    auto output = startWaitGetOutput({testPath("function_splicing/a.elf")});
    REQUIRE( output ==
        "main\n"
        "---------\n"
        "proxy single(1, 2)\n"
        "proxy single = 3\n"
        "---------\n"
        "proxy multiple(5, 6)\n"
        "multiple_bb(5, 6) first\n"
        "multiple_bb(5, 6) second\n"
        "proxy multiple = 11\n"
        "---------\n"
        "proxy multipleRecursive(7, 9)\n"
        "proxy single(9, 7)\n"
        "proxy single = 16\n"
        "proxy multiple(14, 27)\n"
        "multiple_bb(14, 27) first\n"
        "multiple_bb(14, 27) second\n"
        "proxy multiple = 41\n"
        "proxy multipleRecursive = 41\n"
        "---------\n"
        "done\n"
    );
}

TEST_CASE("libfs_general_simple") {
    auto output = startWaitGetOutput({testPath("libfs_general_simple/a.elf")});
    REQUIRE( output ==
        "Waiting for mounting\n"
        "Waiting for mounting done\n"
        "cellFsMkdir in\n"
        "cellFsMkdir: ret = 0\n"
        "cellFsOpen in\n"
        "cellFsOpen: err = 0\n"
        "cellFsStat in\n"
        "cellFsStat: ret = 0\n"
        "cellFsStat: status.st_blksize = 512\n"
        "cellFsStat: status.st_size    = 0\n"
        "cellFsFstat in\n"
        "cellFsFstat: ret = 0\n"
        "cellFsWrite in - 1\n"
        "cellFsWrite: err = 0\n"
        "cellFsWrite: nwrite = 100\n"
        "cellFsLseek-1 SEEK_SET in\n"
        "cellFsLseek: pos = 0\n"
        "cellFsLseek: err = 0\n"
        "cellFsRead in\n"
        "cellFsRead: err     = 0\n"
        "cellFsRead: nread = 100\n"
        "cellFsRead: r     = ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\n"
        "cellFsLseek-2 SEEK_END in\n"
        "cellFsLseek-2: pos = 0\n"
        "cellFsLseek-2: err = 0\n"
        "cellFsRead in\n"
        "cellFsRead: err     = 0\n"
        "cellFsRead: nread = 100\n"
        "cellFsRead: r         = ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\n"
        "cellFsLseek-3 SEEK_CUR in\n"
        "cellFsLseek-3: pos = 0\n"
        "cellFsLseek-3: err = 0\n"
        "cellFsRead in\n"
        "cellFsRead: err   = 0\n"
        "cellFsRead: nread = 100\n"
        "cellFsRead: r     = ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\n"
        "cellFsFstat in\n"
        "cellFsFstat: ret = 0\n"
        "cellFsFtruncate in\n"
        "cellFsFtruncate: err = 0\n"
        "cellFsFstat in\n"
        "cellFsFstat: ret = 0\n"
        "cellFsClose in\n"
        "cellFsClose: ret = 0\n"
        "cellFsStat in\n"
        "cellFsStat: ret = 0\n"
        "cellFsStat: status.st_blksize = 512\n"
        "cellFsStat: status.st_size    = 500\n"
        "cellFsTruncate in\n"
        "cellFsTruncate: err = 0\n"
        "cellFsStat in\n"
        "cellFsStat: ret = 0\n"
        "cellFsStat: status.st_blksize = 512\n"
        "cellFsStat: status.st_size    = 1000\n"
        "cellFsRename in\n"
        "cellFsRename: ret = 0\n"
        "cellFsOpendir in\n"
        "cellFsOpendir: err = 0\n"
        "cellFsReaddir in\n"
        "cellFsReaddir: nread = 258\n"
        "cellFsReaddir: err   = 0\n"
        "dent.d_type          = 1\n"
        "dent.d_name          = .\n"
        "cellFsReaddir in\n"
        "cellFsReaddir: nread = 258\n"
        "cellFsReaddir: err   = 0\n"
        "dent.d_type          = 1\n"
        "dent.d_name          = ..\n"
        "cellFsReaddir in\n"
        "cellFsReaddir: nread = 258\n"
        "cellFsReaddir: err   = 0\n"
        "dent.d_type          = 2\n"
        "dent.d_name          = My_TEST_File_Renamed\n"
        "cellFsReaddir in\n"
        "cellFsReaddir: out\n"
        "cellFsClosedir in\n"
        "cellFsClosedir: ret = 0\n"
        "cellFsUnlink in\n"
        "cellFsUnlink: ret = 0\n"
        "cellFsRmdir in\n"
        "cellFsRmdir: ret = 0\n"
    );
}

TEST_CASE("libfs_general_stream") {
    auto output = startWaitGetOutput({testPath("libfs_general_stream/a.elf")});
    REQUIRE( output ==
        "Waiting for mounting\n"
        "Waiting for mounting done\n"
        "cellFsMkdir in\n"
        "cellFsMkdir: ret = 0\n"
        "cellFsOpen in\n"
        "cellFsOpen: err = 0\n"
        "cellFsFstat in\n"
        "cellFsFstat: ret = 0\n"
        "cellFsFtruncate in\n"
        "cellFsFtruncate: err = 0\n"
        "cellFsFstat in\n"
        "cellFsFstat: ret = 0\n"
        "cellFsFGetBlockSize in\n"
        "cellFsFGetBlockSize: ret = 0\n"
        "cellFsFGetBlockSize: sector_size = 512\n"
        "cellFsFGetBlockSize: block_size  = 512\n"
        "***** COPY MODE TEST START *****\n"
        "cellFsStReadInit in\n"
        "cellFsStReadInit: ret = 0\n"
        "cellFsStReadStart in\n"
        "cellFsStReadStart: ret = 0\n"
        "cellFsStRead reached ERANGE !!\n"
        "cellFsStReadStop in\n"
        "cellFsStReadStop: ret = 0\n"
        "cellFsStReadFinish in\n"
        "cellFsStReadFinish: ret = 0\n"
        "read = 2097155\n"
        "***** COPY MODE TEST DONE *****\n"
        "***** COPYLESS MODE TEST START *****\n"
        "cellFsStReadInit in\n"
        "cellFsStReadInit: ret = 0\n"
        "cellFsStReadStart in\n"
        "cellFsStReadStart: ret = 0\n"
        "cellFsStReadGetCurrentAddr reached ERANGE !!\n"
        "cellFsStReadStop in\n"
        "cellFsStReadStop: ret = 0\n"
        "cellFsStReadFinish in\n"
        "cellFsStReadFinish: ret = 0\n"
        "read = 2097155\n"
        "***** COPYLESS MODE TEST DONE *****\n"
        "***** COPYLESS MODE TEST WITH CALLBACK START *****\n"
        "cellFsStReadInit in\n"
        "cellFsStReadInit: ret = 0\n"
        "cellFsStReadStart in\n"
        "cellFsStReadStart: ret = 0\n"
        "cellFsStReadGetCurrentAddr reached ERANGE !!\n"
        "cellFsStReadStop in\n"
        "cellFsStReadStop: ret = 0\n"
        "cellFsStReadFinish in\n"
        "cellFsStReadFinish: ret = 0\n"
        "read = 2097155\n"
        "***** COPYLESS MODE WITH CALLBACK TEST DONE *****\n"
        "cellFsClose in\n"
        "cellFsClose: ret = 0\n"
        "cleanup:cellFsUnlink in\n"
        "cleanup:cellFsUnlink: ret = 0\n"
        "cleanup:cellFsRmdir in\n"
        "cleanup:cellFsRmdir: ret = 0\n"
        "stream succeeded\n"
    );
}

TEST_CASE("ppu_threads_lwcond_2") {
    test_interpreter_and_rewriter({testPath("ppu_threads_lwcond_2/a.elf")},
        "consumer started\n"
        "consumer started\n"
        "producers done\n"
        "setting stop to 1\n"
        "signalling\n"
        "count: 100000\n"
        "consumer started\n"
        "producers done\n"
        "setting stop to 1\n"
        "count: 150000\n"
    );
}

TEST_CASE("ppu_threads_main_thread_id") {
    test_interpreter_and_rewriter({testPath("ppu_threads_main_thread_id/a.elf")},
        "thread id: 1000087 prio: 1000\n"
    );
}

TEST_CASE("ppu_threads_cond_repeated_signal") {
    test_interpreter_and_rewriter({testPath("ppu_threads_cond_repeated_signal/a.elf")},
        "all done\n"
    );
}

TEST_CASE("hash_spurs_sum") {
    test_interpreter_and_rewriter({testPath("hash_spurs_sum/a.elf"), "/app_home file"},
        "SPU Sum samples\n"
        "-----------------------------------------------------------------------\n"
        "SPU Sum MD5:\n"
        "Waiting for mounting on /app_home\n"
        "Waiting for mounting done\n"
        "Hash result is: e6db0be0d1c01bd33b585b5811933d71\n"
        "-----------------------------------------------------------------------\n"
        "SPU Sum SHA-1:\n"
        "Waiting for mounting on /app_home\n"
        "Waiting for mounting done\n"
        "Hash result is: 3da70be8a81a250c498e2c7cff4ad65cde5efd9d\n"
        "-----------------------------------------------------------------------\n"
        "SPU Sum SHA-224:\n"
        "Waiting for mounting on /app_home\n"
        "Waiting for mounting done\n"
        "Hash result is: 3fd869b9d94722ec638b5bf713d51d8a02efcf176fa57a21fe74fc85\n"
        "-----------------------------------------------------------------------\n"
        "SPU Sum SHA-256:\n"
        "Waiting for mounting on /app_home\n"
        "Waiting for mounting done\n"
        "Hash result is: b803c8f777b1bdd18ea9abaf4dbc4d05b5634d94db8a2fb086823d007a88d6e3\n"
        "-----------------------------------------------------------------------\n"
        "SPU Sum SHA-384:\n"
        "Waiting for mounting on /app_home\n"
        "Waiting for mounting done\n"
        "Hash result is: ad1f41e1a0fefda8316220b1e1404d219e0b45c4fe83f503ce6cdeee9a0be93c607d39ef49ab394e10eefbbd04d4b835\n"
        "-----------------------------------------------------------------------\n"
        "SPU Sum SHA-512:\n"
        "Waiting for mounting on /app_home\n"
        "Waiting for mounting done\n"
        "Hash result is: ba1cf3f5b23254cc9e63bfe77acfe418ee80e9a68169a347f43ba7cff6b9fc140d0552249834637040e632b5283326cc68fcba507b99216e523a20c6d866ba4c\n"
        "PPU: wait for taskset shutdown...\n"
        "PPU: finished.\n"
    );
}

TEST_CASE("spu_thread_group_6") {
    test_interpreter_and_rewriter({testPath("spu_thread_group_6/a.elf")},
        "Initializing SPUs\n"
        "Creating an event queue.\n"
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "Conencting SPU thread (0) to the SPU thread user event.\n"
        "All SPU threads have been successfully initialized.\n"
        "The resultant signal = 1.\n"
        "SPU thread 0's exit status = 3\n"
        "The resultant signal = 1.\n"
        "SPU thread 0's exit status = 3\n"
        "The resultant signal = 1.\n"
        "SPU thread 0's exit status = 3\n"
        "The resultant signal = 1.\n"
        "SPU thread 0's exit status = 3\n"
        "The resultant signal = 1.\n"
        "SPU thread 0's exit status = 3\n"
        "Exiting.\n"
    );
}

TEST_CASE("job_call_ret_next") {
    test_interpreter_and_rewriter({testPath("job_call_ret_next/a.elf")},
        "job main\n"
        "job sub1\n"
        "job sub2\n"
        "job next\n"
    );
}

TEST_CASE("job_cpp_virtual") {
    test_interpreter_and_rewriter({testPath("job_cpp_virtual/a.elf")},
        "Constructor for Base\n"
        "Constructor for Derived\n"
        "*** job_virtual(PIC) started\n"
        "[auto instance]\n"
        "Constructor for Base\n"
        "Constructor for Derived\n"
        "This is Derived val=2\n"
        "[global instance]\n"
        "This is Derived val=2\n"
        "[static instance]\n"
        "Constructor for Base\n"
        "Constructor for Derived\n"
        "Constructor for StaticDerived\n"
        "This is StaticDerived val=3\n"
        "[placement new]\n"
        "Constructor for Base\n"
        "Constructor for Derived\n"
        "This is Derived val=2\n"
        "*** job_virtual(PIC) completed\n"
        "Destructor for Derived\n"
        "Destructor for Base\n"
        "Destructor for StaticDerived\n"
        "Destructor for Derived\n"
        "Destructor for Base\n"
        "Destructor for Derived\n"
        "Destructor for Base\n"
        "job_shutdown\n"
        " ****** shutdown : eaJobChain=ffffffffffffffff\n"
    );
}

TEST_CASE("job_double_buffer") {
    test_interpreter_and_rewriter({testPath("job_double_buffer/a.elf")},
        "uniform = 2a700f3d\n"
        "uniform = 135ab93d\n"
    );
}

TEST_CASE("job_guard_and_next") {
    test_interpreter_and_rewriter({testPath("job_guard_and_next/a.elf")},
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
        "job1 hello\n"
    );
}

TEST_CASE("job_joblist") {
    test_interpreter_and_rewriter({testPath("job_joblist/a.elf")},
        "Waiting for the event flag\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        "job hello\n"
        " ****** event sender : ev=30f00\n"
    );
}

//TEST_CASE("job_stall_successor") {
//    test_interpreter_and_rewriter({testPath("job_stall_successor/a.elf")},
//        "DMA in job TEST\n"
//        "  elapsed time = 0(us)\n"
//        "DMA in job w/Restart TEST\n"
//        "  elapsed time = 0(us)\n"
//    );
//}

TEST_CASE("job_sync_label") {
    test_interpreter_and_rewriter({testPath("job_sync_label/a.elf")},
        "TEST with SYNC command\n"
        "  elapsed time = 0(us)\n"
        "TEST with SYNC_LABEL command\n"
        "  elapsed time = 0(us)\n"
    );
}

TEST_CASE("job_urgent_call") {
    test_interpreter_and_rewriter({testPath("job_urgent_call/a.elf")},
        "notify\n"
        " ****** event sender : ev=0, counter=0\n"
        "notify\n"
        " ****** event sender : ev=0, counter=1\n"
        "notify\n"
        " ****** event sender : ev=0, counter=2\n"
        "notify\n"
        " ****** event sender : ev=0, counter=3\n"
        "notify\n"
        " ****** event sender : ev=0, counter=4\n"
    );
}
