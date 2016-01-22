#include <catch.hpp>

#include <QProcess>

static const char* runnerPath = "../ps3run/ps3run";

TEST_CASE("simple_printf") {
    QProcess proc;
    proc.start(runnerPath, QStringList() << "./binaries/simple_printf/a.elf");
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == "some output\n" );
}

TEST_CASE("bubblesort") {
    QProcess proc;
    auto args = QStringList() << "./binaries/bubblesort/a.elf" 
                              << "5" << "17" << "30" << "-1" << "20" << "12" << "100" << "0";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == "args: 5 17 30 -1 20 12 100 0 \nsorted: -1 0 5 12 17 20 30 100 \n" );
}

TEST_CASE("md5") {
    QProcess proc;
    auto args = QStringList() << "./binaries/md5/a.elf" << "-x";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/printf/a.elf" << "-x";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/fcmpconv/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/matrixmul/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/dtoa/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/float_printf/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "18.516 = 1.851600e+01\n"
        "float = 4.179412e-01\n"
        "double = 4.179412e-01\n"
    );
}

TEST_CASE("gcm_context_size") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_context_size/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output ==  "1bff\n" );
}

TEST_CASE("gcm_memory_mapping") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_memory_mapping/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "host_addr to offset: 0\n"
        "offset 0 to address == host_addr?: 1\n"
        "address 0xc0000005 to offset: 5\n"
    );
}

TEST_CASE("hello_simd") {
    QProcess proc;
    auto args = QStringList() << "./binaries/hello_simd/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    REQUIRE(proc.exitCode() == 0);
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/basic_large_cmdbuf/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output ==  
        "end - begin = 6ffc\n"
        "success\n"
    );
}

TEST_CASE("ppu_threads") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == "exitstatus: 3; i: 4000\n" );
}

TEST_CASE("ppu_threads_tls") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_tls/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "exitstatus: 125055; i: 4000\n"
        "primary thread tls_int: 500\n"
    );
}

TEST_CASE("ppu_threads_atomic_inc") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_atomic_inc/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == "exitstatus: 1; i: 80000\n" );
}

TEST_CASE("ppu_threads_atomic_single_lwarx") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_atomic_single_lwarx/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == "5, 3\n" );
}

TEST_CASE("ppu_cellgame") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_cellgame/USRDIR/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "title: GameUpdate Utility Sample\n"
        "gamedir: EMUGAME\n"
        "contentdir: /dev_hdd0\n"
        "usrdir: /dev_hdd0/USRDIR\n"
        "filesize: 4\n"
    );
}

TEST_CASE("ppu_cellSysutil") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_cellSysutil/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_lwmutex_lwcond/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "test_lwmutex: 0; i: 4000\n"
        "test_lwmutex_recursive: 0; i: 4000\n"
        "test_lwcond: 5015; i: 0\n"
    );
}

TEST_CASE("ppu_threads_mutex_cond") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_mutex_cond/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "test_mutex: 0; i: 4000\n"
        "test_mutex_recursive: 0; i: 4000\n"
        "test_cond: 5015; i: 0\n"
    );
}

TEST_CASE("ppu_threads_rwlock") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_rwlock/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "test_rwlock_w: 0; i: 4000\n"
        "test_lwmutex: 40; i: 10\n"
    );
}

TEST_CASE("ppu_threads_queue") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_queue/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "test_correctness(1): 0; i: 481200\n"
        "test_correctness(0): 0; i: 481200\n"
    );
}

TEST_CASE("ppu_threads_lwcond_init") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_lwcond_init/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "sys_lwmutex_t.recursive_count 0\n"
        "sys_lwmutex_t.attribute 22\n"
        "sys_lwmutex_t.lock_var.all_info 0\n"
        "SYS_SYNC_PRIORITY | SYS_SYNC_NOT_RECURSIVE 22\n"
        "&mutex == cv.lwmutex 1\n"
    );
}

TEST_CASE("ppu_syscache") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_syscache/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_threads_is_stack/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == 
        "main thread: 1100\n"
        "other thread: 1100\n"
    );
}

TEST_CASE("ppu_fs_readdir") {
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_fs_readdir/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_fios/USRDIR/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_hash/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/ppu_simd_math/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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
    QProcess proc;
    auto args = QStringList() << "./binaries/spu_getting_argp/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
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