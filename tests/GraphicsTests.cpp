#include <catch.hpp>

#include <QProcess>

static const char* runnerPath = "../ps3run/ps3run";

void compareLastFrame(const char* expected) {
    QProcess proc;
    auto args = QStringList() << "-depth" << "8"
                              << "-size" << "1280x720"
                              << "-rotate" << "180"
                              << "/tmp/ps3frame.rgba"
                              << "/tmp/ps3frame.png";
    proc.start("convert", args);
    proc.waitForFinished();
    REQUIRE( proc.exitCode() == 0 );
    
    args = QStringList() << "-metric" << "AE"
                         << "/tmp/ps3frame.png"
                         << expected
                         << "/tmp/ps3frame-diff.png";
    proc.start("compare", args);
    proc.waitForFinished();
    REQUIRE( proc.exitCode() != 2 );
    auto output = QString(proc.readAllStandardError()).toStdString();
    REQUIRE( output == "0" );
}

TEST_CASE("gcm_hello") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_hello/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_hello/ps3frame.png");
}

TEST_CASE("gcm_simple_fshader") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_simple_fshader/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_simple_fshader/ps3frame.png");
}