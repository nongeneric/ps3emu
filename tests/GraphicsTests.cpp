#include <catch.hpp>

#include <QProcess>
#include "../ps3emu/utils.h"

static const char* runnerPath = "../ps3run/ps3run";

int comparisonNum = 0;

void compareLastFrame(const char* expected, int n = 0, int tolerance = 5) {
    comparisonNum++;
    QProcess proc;
    auto args = QStringList() << "-depth" << "8"
                              << "-size" << "1280x720"
                              << "-flip"
                              << "-quality" << "11"
                              << QString("/tmp/ps3frame%1.rgb").arg(n)
                              << QString("/tmp/ps3frame%1.png").arg(n);
    proc.start("convert", args);
    proc.waitForStarted();
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    
    args = QStringList() << "-metric" << "AE"
                         << "-fuzz" << QString("%1%%").arg(tolerance)
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

TEST_CASE("gcm_fragment_texture") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_fragment_texture/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame0.png", 0);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame1.png", 1);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame2.png", 2);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame3.png", 3);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame4.png", 4);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame5.png", 5);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame6.png", 6);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame7.png", 7);
}

TEST_CASE("simple_console_gcm") {
    QProcess proc;
    auto args = QStringList() << "./binaries/simple_console_gcm/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/simple_console_gcm/ps3frame0.png");
}

TEST_CASE("gcm_dice") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_dice/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_dice/ps3frame0.png");
}

TEST_CASE("gcm_flip_handler") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_flip_handler/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_flip_handler/ps3frame0.png");
}

TEST_CASE("opengl_createdevice") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_createdevice/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_createdevice/ps3frame1.png", 1);
}

TEST_CASE("gcm_duck") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_duck/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_duck/ps3frame0.png");
}

TEST_CASE("gcm_video_texturing") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_video_texturing/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_video_texturing/ps3frame0.png");
}

TEST_CASE("gcm_stencil_reflect") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_stencil_reflect/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_stencil_reflect/ps3frame0.png", 0);
    compareLastFrame("./binaries/gcm_stencil_reflect/ps3frame1.png", 1);
}

TEST_CASE("gcm_render_to_texture") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_render_to_texture/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/gcm_render_to_texture/ps3frame0.png", 0);
    compareLastFrame("./binaries/gcm_render_to_texture/ps3frame1.png", 1);
}

TEST_CASE("opengl_1_basiccg") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_1_basiccg/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_1_basiccg/ps3frame1.png", 1);
}

TEST_CASE("opengl_2_basic_vertex_lighting") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_2_basic_vertex_lighting/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_2_basic_vertex_lighting/ps3frame1.png", 1);
}

TEST_CASE("opengl_3_basic_fragment_lighting") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_3_basic_fragment_lighting/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_3_basic_fragment_lighting/ps3frame1.png", 1);
}

TEST_CASE("opengl_4_proc_anim") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_4_proc_anim/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_4_proc_anim/ps3frame1.png", 1);
}

TEST_CASE("opengl_5_mipmap") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_5_mipmap/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_5_mipmap/ps3frame1.png", 1);
}

TEST_CASE("opengl_6_gloss_map") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_6_gloss_map/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_6_gloss_map/ps3frame1.png", 1);
}

TEST_CASE("opengl_7_environment_map") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_7_environment_map/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_7_environment_map/ps3frame1.png", 1);
}

TEST_CASE("opengl_8_irradiance_map") {
    QProcess proc;
    auto args = QStringList() << "./binaries/opengl_8_irradiance_map/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/opengl_8_irradiance_map/ps3frame1.png", 1);
}

TEST_CASE("pngdec_ppu_graphics") {
    QProcess proc;
    auto args = QStringList() << "./binaries/pngdec_ppu/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    compareLastFrame("./binaries/pngdec_ppu/ps3frame0.png", 0);
}

TEST_CASE("gcm_strip_branch") {
    QProcess proc;
    auto args = QStringList() << "./binaries/gcm_strip_branch/a.elf";
    proc.start(runnerPath, args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() == 0 );
    for (int i = 0; i <= 18; ++i) {
        compareLastFrame(
            ssnprintf("./binaries/gcm_strip_branch/ps3frame%d.png", i).c_str(),
            i);
    }
}