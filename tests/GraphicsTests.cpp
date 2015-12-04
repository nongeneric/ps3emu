#include <catch.hpp>

#include <QProcess>

static const char* runnerPath = "../ps3run/ps3run";

void compareLastFrame(const char* expected) {
    QProcess proc;
    auto args = QStringList() << "-depth" << "8"
                              << "-size" << "1280x720"
                              << "-flip"
                              << "/tmp/ps3frame.rgba"
                              << "/tmp/ps3frame.png";
    proc.start("convert", args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    
    args = QStringList() << "-metric" << "AE"
                         << "-fuzz" << "2%"
                         << "/tmp/ps3frame.png"
                         << expected
                         << "/tmp/ps3frame-diff.png";
    proc.start("compare", args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() != 2 );
    auto output = QString(proc.readAllStandardError()).toStdString();
    REQUIRE( output == "0" );
}

TEST_CASE("gcm_hello") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_hello/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_hello/ps3frame.png");
}

TEST_CASE("gcm_simple_fshader") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_simple_fshader/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_simple_fshader/ps3frame.png");
}

TEST_CASE("gcm_simple_shaders") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_simple_shaders/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_simple_shaders/ps3frame.png");
}

TEST_CASE("gcm_vertex_texture_wrapping") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_vertex_texture_wrapping/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_vertex_texture_wrapping/ps3frame.png");
}

TEST_CASE("gcm_vertex_texture_wrapping3") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_vertex_texture_wrapping3/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_vertex_texture_wrapping3/ps3frame.png");
}

TEST_CASE("gcm_vertex_texture") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_vertex_texture/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_vertex_texture/ps3frame.png");
}

TEST_CASE("gcm_cube") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_cube/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_cube/ps3frame.png");
}

TEST_CASE("gcm_mrt") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_cube_mrt/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_cube_mrt/ps3frame.png");
}

TEST_CASE("gcm_human") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_human/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_human/ps3frame.png");
}