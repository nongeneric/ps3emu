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