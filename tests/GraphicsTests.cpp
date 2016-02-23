#include <catch.hpp>

#include <QProcess>

static const char* runnerPath = "../ps3run/ps3run";

int comparisonNum = 0;

void compareLastFrame(const char* expected, int n = 0) {
    comparisonNum++;
    QProcess proc;
    auto args = QStringList() << "-depth" << "8"
                              << "-size" << "1280x720"
                              << "-flip"
                              << QString("/tmp/ps3frame%1.rgba").arg(n)
                              << QString("/tmp/ps3frame%1.png").arg(n);
    proc.start("convert", args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    
    args = QStringList() << "-metric" << "AE"
                         << "-fuzz" << "2%"
                         << QString("/tmp/ps3frame%1.png").arg(n)
                         << expected
                         << QString("/tmp/ps3frame-diff%1.png").arg(n);
    proc.start("compare", args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() != 2 );
    auto output = QString(proc.readAllStandardError()).toStdString();
    if (output != "0") {
        args = QStringList() << QString("/tmp/ps3frame-diff%1.png").arg(n)
                             << QString("/tmp/ps3frame-diff%1_bad%2.png").arg(n).arg(comparisonNum);
        proc.start("mv", args);
        proc.waitForFinished(-1);
        args = QStringList() << QString("/tmp/ps3frame%1.png").arg(n)
                             << QString("/tmp/ps3frame%1_bad%2.png").arg(n).arg(comparisonNum);
        proc.start("mv", args);
        proc.waitForFinished(-1);
        REQUIRE( proc.exitCode() == 0 );
    }
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

TEST_CASE("gcm_cube_mrt") {
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
    compareLastFrame("./binaries/gcm_human/ps3frame0.png", 0);
    compareLastFrame("./binaries/gcm_human/ps3frame1.png", 1);
    compareLastFrame("./binaries/gcm_human/ps3frame2.png", 2);
}

TEST_CASE("gcm_vertex_shader_branch") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_vertex_shader_branch/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_vertex_shader_branch/ps3frame.png");
}