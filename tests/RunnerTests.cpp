#include <catch.hpp>

#include <QProcess>

QString runnerPath = "../ps3run/ps3run";

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