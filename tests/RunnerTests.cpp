#include <catch.hpp>

#include <QProcess>
#include <vector>

static const char* runnerPath = "../ps3run/ps3run";

std::string startWaitGetOutput(std::vector<std::string> args) {
    QProcess proc;
    std::string argstr;
    for (size_t i = 1; i < args.size(); ++i) {
        argstr += args[i];
        if (i != args.size() - 1)
             argstr += " ";
    }
    proc.start(runnerPath,
               QStringList() << "--elf" << QString::fromStdString(args.front())
                             << "--args" << QString::fromStdString(argstr));
    proc.waitForStarted();
    proc.waitForFinished();
    return QString(proc.readAll()).toStdString();
}

TEST_CASE("simple_printf") {
    auto output = startWaitGetOutput({"./binaries/simple_printf/a.elf"});
    REQUIRE( output == "some output\n" );
}

TEST_CASE("bubblesort") {
    auto output = startWaitGetOutput({"./binaries/bubblesort/a.elf",
                                      "5",
                                      "17",
                                      "30",
                                      "-1",
                                      "20",
                                      "12",
                                      "100",
                                      "0"});
    REQUIRE( output == "args: 5 17 30 -1 20 12 100 0 \nsorted: -1 0 5 12 17 20 30 100 \n" );
}

TEST_CASE("md5") {
    auto output = startWaitGetOutput({"./binaries/md5/a.elf", "-x"});
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
    auto output = startWaitGetOutput({"./binaries/printf/a.elf", "-x"});
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
    auto output = startWaitGetOutput({"./binaries/fcmpconv/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/matrixmul/a.elf"});
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
        "11633689.000000\n"
        "38058352.000000\n"
        "-12969724.000000\n"
        "7520124.500000\n"
        "2673266.750000\n"
        "-5750935.500000\n"
        "24634854.000000\n"
        "42614816.000000\n"
        "-22973844.000000\n"
        "1436.699829\n"
        "33139.847656\n"
        "788596.937500\n"
        "-nan\n"
        "-513356256.000000\n"
    );
}

TEST_CASE("dtoa") {
    auto output = startWaitGetOutput({"./binaries/dtoa/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/float_printf/a.elf"});
    REQUIRE( output == 
        "18.516 = 1.851600e+01\n"
        "float = 4.179412e-01\n"
        "double = 4.179412e-01\n"
    );
}

TEST_CASE("gcm_context_size") {
    auto output = startWaitGetOutput({"./binaries/gcm_context_size/a.elf"});
    REQUIRE( output ==  "1bff\n" );
}

TEST_CASE("gcm_memory_mapping") {
    auto output = startWaitGetOutput({"./binaries/gcm_memory_mapping/a.elf"});
    REQUIRE( output == 
        "host_addr to offset: 0\n"
        "offset 0 to address == host_addr?: 1\n"
        "address 0xc0000005 to offset: 5\n"
    );
}

TEST_CASE("hello_simd") {
    auto output = startWaitGetOutput({"./binaries/hello_simd/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/basic_large_cmdbuf/a.elf"});
    REQUIRE( output ==  
        "end - begin = 6ffc\n"
        "success\n"
    );
}

TEST_CASE("ppu_threads") {
    auto output = startWaitGetOutput({"./binaries/ppu_threads/a.elf"});
    REQUIRE( output == "exitstatus: 3; i: 4000\n" );
}

TEST_CASE("ppu_threads_tls") {
    auto output = startWaitGetOutput({"./binaries/ppu_threads_tls/a.elf"});
    REQUIRE( output == 
        "exitstatus: 125055; i: 4000\n"
        "primary thread tls_int: 500\n"
    );
}

TEST_CASE("ppu_threads_atomic_inc") {
    auto output = startWaitGetOutput({"./binaries/ppu_threads_atomic_inc/a.elf"});
    REQUIRE( output == "exitstatus: 1; i: 80000\n" );
}

TEST_CASE("ppu_threads_atomic_single_lwarx") {
    auto output = startWaitGetOutput({"./binaries/ppu_threads_atomic_single_lwarx/a.elf"});
    REQUIRE( output == "5, 3\n" );
}

TEST_CASE("ppu_cellgame") {
    auto output = startWaitGetOutput({"./binaries/ppu_cellgame/USRDIR/a.elf"});
    REQUIRE( output == 
        "title: GameUpdate Utility Sample\n"
        "gamedir: EMUGAME\n"
        "contentdir: /dev_hdd0\n"
        "usrdir: /dev_hdd0/USRDIR\n"
        "filesize: 4\n"
    );
}

TEST_CASE("ppu_cellSysutil") {
    auto output = startWaitGetOutput({"./binaries/ppu_cellSysutil/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/ppu_threads_lwmutex_lwcond/a.elf"});
    REQUIRE( output == 
        "test_lwmutex: 0; i: 4000\n"
        "test_lwmutex_recursive: 0; i: 4000\n"
        "test_lwcond: 5015; i: 0\n"
    );
}

TEST_CASE("ppu_threads_mutex_cond") {
    auto output = startWaitGetOutput({"./binaries/ppu_threads_mutex_cond/a.elf"});
    REQUIRE( output == 
        "test_mutex: 0; i: 4000\n"
        "test_mutex_recursive: 0; i: 4000\n"
        "test_cond: 5015; i: 0\n"
    );
}

TEST_CASE("ppu_threads_rwlock") {
    auto output = startWaitGetOutput({"./binaries/ppu_threads_rwlock/a.elf"});
    REQUIRE( output == 
        "test_rwlock_w: 0; i: 4000\n"
        "test_lwmutex: 40; i: 10\n"
    );
}

TEST_CASE("ppu_threads_queue") {
    auto output = startWaitGetOutput({"./binaries/ppu_threads_queue/a.elf"});
    REQUIRE( output == 
        "test_correctness(1): 0; i: 481200\n"
        "test_correctness(0): 0; i: 481200\n"
    );
}

TEST_CASE("ppu_threads_lwcond_init") {
    auto output = startWaitGetOutput({"./binaries/ppu_threads_lwcond_init/a.elf"});
    REQUIRE( output == 
        "sys_lwmutex_t.recursive_count 0\n"
        "sys_lwmutex_t.attribute 22\n"
        "sys_lwmutex_t.lock_var.all_info 0\n"
        "SYS_SYNC_PRIORITY | SYS_SYNC_NOT_RECURSIVE 22\n"
        "&mutex == cv.lwmutex 1\n"
    );
}

TEST_CASE("ppu_syscache") {
    auto output = startWaitGetOutput({"./binaries/ppu_syscache/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/ppu_threads_is_stack/a.elf"});
    REQUIRE( output == 
        "main thread: 1100\n"
        "other thread: 1100\n"
    );
}

TEST_CASE("ppu_fs_readdir") {
    auto output = startWaitGetOutput({"./binaries/ppu_fs_readdir/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/ppu_fios/USRDIR/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/ppu_hash/a.elf"});
    REQUIRE( output == 
        "md5 0 b1a49029323448bf6407b94ad6f6f2cf\n"
        "sha1 0 6eb89053fa6048876d0210e5524b55752908af55\n"
        "sha224 0 2d47a0c20145d4ee365abd1de270b9b8747f7574c664f5db8179d86b\n"
        "sha256 0 3b3b7c0e64a7030bb6b36b9eb8afa279818f75bb4f962f89a3fd0df93d510c5f\n"
        "sha384 0 48070f66c4a1dac55fc4c5a4a0db8677ae1f8ac13e473dd73bab525832d73999fc2fea7f83b95d2aab0c95fe41df11c4\n"
        "sha512 0 9478b106b00d65f506d196006d59b59cf2ba38837abea1adc634cd89a583eac615f60102f482892906c3442b2e5d95dc04f63cf2b66cf9de62ae99a8b42639ef\n"
    );
}

TEST_CASE("ppu_simd_math") {
    auto output = startWaitGetOutput({"./binaries/ppu_simd_math/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/spu_getting_argp/a.elf"});
    REQUIRE( output == 
        "Creating an SPU thread group.\n"
        "Initializing SPU thread 0\n"
        "All SPU threads have been successfully initialized.\n"
        "Starting the SPU thread group.\n"
        "All SPU threads exited by sys_spu_thread_exit().\n"
        "SPU thread 0's exit status = 0\n"
        "SUCCESS: fromSpu is the same as toSpu\n"
        "## libdma : sample_dma_getting_argp SUCCEEDED ##\n"
    );
}

TEST_CASE("raw_spu_printf") {
    auto output = startWaitGetOutput({"./binaries/raw_spu_printf/a.elf"});
    REQUIRE( output == 
        "Initializing SPUs\n"
        "sys_raw_spu_create succeeded. raw_spu number is 0\n"
        "Hello, World 1\n"
        "Hello, World 2\n"
        "a, 12, 20, 0x1e, 0X28,   50, \"test\"\n"
    );
}

TEST_CASE("gcm_memory") {
    auto output = startWaitGetOutput({"./binaries/gcm_memory/a.elf"});
    REQUIRE( output == 
        "* vidmem base: 0xc0000000\n"
        "* IO base    : 0x30100000\n"
        "* vidmem size: 0xf900000\n"
        "* IO size    : 0x100000\n"
        "localAddress offset: 8\n"
        "ioAddress offset: 8\n"
        "offset: 10000000\n"
        "offset: 0\n"
        "va to io: 301 -> 0\n"
        "io to va: 0 -> 301\n"
        "va: 30100000\n"
        "va to io: 301 -> 0\n"
        "va to io: 303 -> 5\n"
        "va to io: 304 -> 6\n"
        "va to io: 305 -> 7\n"
        "io to va: 0 -> 301\n"
        "io to va: 5 -> 303\n"
        "io to va: 6 -> 304\n"
        "io to va: 7 -> 305\n"
        "ea2 offset: 500000\n"
        "err: 802100ff\n"
        "va to io: 301 -> 0\n"
        "va to io: 303 -> 5\n"
        "va to io: 304 -> 6\n"
        "va to io: 305 -> 7\n"
        "io to va: 0 -> 301\n"
        "io to va: 5 -> 303\n"
        "io to va: 6 -> 304\n"
        "io to va: 7 -> 305\n"
        "err: 0\n"
        "va to io: 301 -> 0\n"
        "va to io: 303 -> 5\n"
        "va to io: 304 -> 6\n"
        "va to io: 305 -> 7\n"
        "va to io: 307 -> f\n"
        "va to io: 308 -> 10\n"
        "io to va: 0 -> 301\n"
        "io to va: 5 -> 303\n"
        "io to va: 6 -> 304\n"
        "io to va: 7 -> 305\n"
        "io to va: f -> 307\n"
        "io to va: 10 -> 308\n"
        "ea3 offset: f00000\n"
        "va to io: 301 -> 0\n"
        "va to io: 303 -> 5\n"
        "va to io: 304 -> 6\n"
        "va to io: 305 -> 7\n"
        "va to io: 307 -> f\n"
        "va to io: 308 -> 10\n"
        "va to io: 30a -> 1\n"
        "va to io: 30b -> 2\n"
        "va to io: 30c -> 3\n"
        "va to io: 30d -> 4\n"
        "io to va: 0 -> 301\n"
        "io to va: 1 -> 30a\n"
        "io to va: 2 -> 30b\n"
        "io to va: 3 -> 30c\n"
        "io to va: 4 -> 30d\n"
        "io to va: 5 -> 303\n"
        "io to va: 6 -> 304\n"
        "io to va: 7 -> 305\n"
        "io to va: f -> 307\n"
        "io to va: 10 -> 308\n"
        "ea4 offset: 100000\n"
        "va to io: 301 -> 0\n"
        "va to io: 303 -> 5\n"
        "va to io: 304 -> 6\n"
        "va to io: 305 -> 7\n"
        "va to io: 30a -> 1\n"
        "va to io: 30b -> 2\n"
        "va to io: 30c -> 3\n"
        "va to io: 30d -> 4\n"
        "io to va: 0 -> 301\n"
        "io to va: 1 -> 30a\n"
        "io to va: 2 -> 30b\n"
        "io to va: 3 -> 30c\n"
        "io to va: 4 -> 30d\n"
        "io to va: 5 -> 303\n"
        "io to va: 6 -> 304\n"
        "io to va: 7 -> 305\n"
        "va to io: 301 -> 0\n"
        "va to io: 303 -> 5\n"
        "va to io: 304 -> 6\n"
        "va to io: 305 -> 7\n"
        "io to va: 0 -> 301\n"
        "io to va: 5 -> 303\n"
        "io to va: 6 -> 304\n"
        "io to va: 7 -> 305\n"
    );
}

TEST_CASE("gcm_transfer") {
    auto output = startWaitGetOutput({"./binaries/gcm_transfer/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/opengl_hash/a.elf"});
    REQUIRE( output == 
        "hash: 1d0\n"
    );
}

TEST_CASE("raw_spu_opengl_dma") {
    auto output = startWaitGetOutput({"./binaries/raw_spu_opengl_dma/a.elf"});
    REQUIRE( output == 
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
    auto output = startWaitGetOutput({"./binaries/ppu_dcbz/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/ppu_float_cmp/a.elf"});
    REQUIRE( output == 
        "1 1 1 0 0 0 1 0 "
    );
}

TEST_CASE("ppu_sraw") {
    auto output = startWaitGetOutput({"./binaries/ppu_sraw/a.elf"});
    REQUIRE( output == 
        "1000\n"
        "1000\n"
        "250\n"
        "4000\n"
        "500\n"
    );
}

TEST_CASE("ppu_fs") {
    auto output = startWaitGetOutput({"./binaries/ppu_fs/a.elf"});
    REQUIRE( output == 
        "FILE 0:\n"
        "\t_Mode = 161\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\tfpos = 805306451\n"
        "\t_Rsize = 0\n"
        "\t_offset = 0\n"
        "\n"
        "\n"
        "FILE 1:\n"
        "\t_Mode = 161\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\tfpos = 805306515\n"
        "\t_Rsize = 0\n"
        "\t_offset = 64\n"
        "\n"
        "\n"
        "FILE 2:\n"
        "\t_Mode = 161\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\tfpos = 805306451\n"
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
        "\tfpos = 805372100\n"
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
        "\tfpos = 805372160\n"
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
        "\tfpos = 805372096\n"
        "\t_Rsize = 0\n"
        "\t_offset = 0\n"
        "\n"
        "\n"
        "FILE 6:\n"
        "\t_Mode = 20705\n"
        "\t_Idx = 0\n"
        "\t_Wstate = 0\n"
        "\t_Cbuf = 0\n"
        "\tfpos = 805372160\n"
        "\t_Rsize = 64\n"
        "\t_offset = 64\n"
        "\n"
        "\n"
        "readbytes: 1\n"
        "content(10): #v4.0:GCC:\n"
    );
}

TEST_CASE("pngdec_ppu") {
    auto output = startWaitGetOutput({"./binaries/pngdec_ppu/a.elf"});
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
        "* setDecodeParam: pngdecOutParam.useMemorySpace          = 512\n"
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
        "* main: Call Malloc Function = 0\n"
        "* main: Call Free Function = 0\n"
        "* errorLog: Finished decoding \n"
    );
}

TEST_CASE("spurs_task_hello") {
    auto output = startWaitGetOutput({"./binaries/spurs_task_hello/a.elf"});
    REQUIRE( output == 
        "SPU: Hello world!\n"
    );
}

TEST_CASE("spu_generic_test") {
    auto output = startWaitGetOutput({"./binaries/spu_generic_test/a.elf"});
    REQUIRE( output == 
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
        "o[155] = 93780f0d c00aaa89 23217598 301abcde\n"
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
        "o[167] = bfb90ff9 72474539 40080000 00000000\n"
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
        "o[187] = 0 -7425.55 -7.7589 -0.9\n"
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
        "complete"
    );
}

TEST_CASE("spu_sync_mutex") {
    auto output = startWaitGetOutput({"./binaries/spu_sync_mutex/a.elf"});
    REQUIRE( output ==
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
    auto output = startWaitGetOutput({"./binaries/prx_simple_c/a.elf"});
    REQUIRE( output ==
        "simple-main:start\n"
        "arg0 = 0, arg1 = 2, arg2 = 4, modres = 1\n"
        "simple-main:done\n"
    );
}

TEST_CASE("prx_call") {
    auto output = startWaitGetOutput({"./binaries/prx_call/a.elf"});
    REQUIRE( output ==
        "call-prx-main:start\n"
        "call_import start\n"
        "20\n"
        "call-prx-main:done\n"
    );
}

TEST_CASE("prx_library_c") {
    auto output = startWaitGetOutput({"./binaries/prx_library_c/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/spu_queue/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/spu_image/a.elf"});
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
    auto output = startWaitGetOutput({"./binaries/fiber_hello/a.elf"});
    REQUIRE( output ==
        "Hello, fiber!\n"
        "## libfiber : sample_fiber_hello SUCCEEDED ##\n"
    );
}
