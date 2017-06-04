#include <catch/catch.hpp>

#include <QProcess>
#include "TestUtils.h"
#include "ps3emu/utils.h"

static auto runnerPath = "../ps3run/ps3run";
static auto gcmvizPath = "../gcmviz/gcmviz";

int comparisonNum = 0;
int lastProcId = 0;

void compareLastFrame(const char* expected, int n = 0, int tolerance = 5, int safePixels = 0) {
    comparisonNum++;
    QProcess proc;
    auto id = lastProcId;
    auto args = QStringList() 
        << "-metric"
        << "AE"
        << "-fuzz" << QString("%1%%").arg(tolerance)
        << QString::fromStdString(ssnprintf("/tmp/ps3frame_%d_%d.png", id, n)) << expected
        << QString::fromStdString(ssnprintf("/tmp/ps3frame-diff_%d_%d.png", id, n));
    proc.start("compare", args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() != 2 );
    auto output = QString(proc.readAllStandardError()).toInt();
    if (output > safePixels) {
        args = QStringList() 
            << QString::fromStdString(ssnprintf("/tmp/ps3frame-diff_%d_%d.png", id, n))
            << QString::fromStdString(ssnprintf("/tmp/ps3frame-diff_%d_%d_bad%d.png", id, n, comparisonNum));
        proc.start("cp", args);
        proc.waitForFinished(-1);
        args = QStringList() 
            << QString::fromStdString(ssnprintf("/tmp/ps3frame_%d_%d.png", id, n))
            << QString::fromStdString(ssnprintf("/tmp/ps3frame_%d_%d_bad%d.png", id, n, comparisonNum));
        proc.start("cp", args);
        proc.waitForFinished(-1);
        REQUIRE( proc.exitCode() == 0 );
    }
    CHECK( output <= safePixels );
}

void runAndWait(QString path, bool gcmviz = false, bool nocapture = false) {
    QProcess proc;
    QStringList args;
    if (gcmviz) {
        args << "--trace"
             << QString::fromStdString(ssnprintf("/tmp/ps3emu_%d.trace", lastProcId))
             << "--replay";
    } else {
        args << "--elf" << path;
        if (!nocapture) {
            args << "--capture-rsx";
        }
    }   
    proc.start(gcmviz ? gcmvizPath : runnerPath, args);
    lastProcId = proc.processId();
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
        WARN(path.toStdString());
        REQUIRE( proc.exitCode() == 0 );
    }
}

TEST_CASE("gcm_hello") {
    runAndWait("./binaries/gcm_hello/a.elf");
    compareLastFrame("./binaries/gcm_hello/ps3frame.png");
    runAndWait("", true);
    compareLastFrame("./binaries/gcm_hello/ps3frame.png");
}

TEST_CASE("gcm_simple_fshader") {
    runAndWait("./binaries/gcm_simple_fshader/a.elf");
    compareLastFrame("./binaries/gcm_simple_fshader/ps3frame.png");
    runAndWait("", true);
    compareLastFrame("./binaries/gcm_simple_fshader/ps3frame.png");
}

TEST_CASE("gcm_simple_shaders") {
    runAndWait("./binaries/gcm_simple_shaders/a.elf");
    compareLastFrame("./binaries/gcm_simple_shaders/ps3frame.png");
    runAndWait("", true);
    compareLastFrame("./binaries/gcm_simple_shaders/ps3frame.png");
}

TEST_CASE("gcm_vertex_texture_wrapping") {
    runAndWait("./binaries/gcm_vertex_texture_wrapping/a.elf");
    compareLastFrame("./binaries/gcm_vertex_texture_wrapping/ps3frame.png");
}

TEST_CASE("gcm_vertex_texture_wrapping3") {
    runAndWait("./binaries/gcm_vertex_texture_wrapping3/a.elf");
    compareLastFrame("./binaries/gcm_vertex_texture_wrapping3/ps3frame.png");
}

TEST_CASE("gcm_vertex_texture") {
    runAndWait("./binaries/gcm_vertex_texture/a.elf");
    compareLastFrame("./binaries/gcm_vertex_texture/ps3frame.png");
}

TEST_CASE("gcm_cube") {
    runAndWait("./binaries/gcm_cube/a.elf");
    compareLastFrame("./binaries/gcm_cube/ps3frame.png");
    runAndWait("", true);
    compareLastFrame("./binaries/gcm_cube/ps3frame.png");
}

TEST_CASE("gcm_cube_mrt") {
    runAndWait("./binaries/gcm_cube_mrt/a.elf");
    compareLastFrame("./binaries/gcm_cube_mrt/ps3frame.png");
}

TEST_CASE("gcm_human") {
    runAndWait("./binaries/gcm_human/a.elf");
    compareLastFrame("./binaries/gcm_human/ps3frame0.png", 0, 20, 20);
    compareLastFrame("./binaries/gcm_human/ps3frame1.png", 1, 20, 20);
    compareLastFrame("./binaries/gcm_human/ps3frame2.png", 2, 20, 20);
}

TEST_CASE("gcm_vertex_shader_branch") {
    runAndWait("./binaries/gcm_vertex_shader_branch/a.elf");
    compareLastFrame("./binaries/gcm_vertex_shader_branch/ps3frame.png");
}

TEST_CASE("gcm_fragment_texture") {
    runAndWait("./binaries/gcm_fragment_texture/a.elf");
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame0.png", 0);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame1.png", 1);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame2.png", 2);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame3.png", 3);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame4.png", 4);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame5.png", 5);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame6.png", 6);
    compareLastFrame("./binaries/gcm_fragment_texture/ps3frame7.png", 7);
    runAndWait("", true);
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
    runAndWait("./binaries/simple_console_gcm/a.elf");
    compareLastFrame("./binaries/simple_console_gcm/ps3frame0.png");
}

TEST_CASE("gcm_dice") {
    runAndWait("./binaries/gcm_dice/a.elf");
    compareLastFrame("./binaries/gcm_dice/ps3frame0.png");
}

TEST_CASE("gcm_flip_handler") {
    runAndWait("./binaries/gcm_flip_handler/a.elf");
    compareLastFrame("./binaries/gcm_flip_handler/ps3frame0.png");
}

TEST_CASE("opengl_createdevice") {
    runAndWait("./binaries/opengl_createdevice/a.elf");
    compareLastFrame("./binaries/opengl_createdevice/ps3frame1.png", 1);
}

TEST_CASE("gcm_duck") {
    runAndWait("./binaries/gcm_duck/a.elf");
    compareLastFrame("./binaries/gcm_duck/ps3frame0.png", 0, 20);
}

TEST_CASE("gcm_video_texturing") {
    runAndWait("./binaries/gcm_video_texturing/a.elf");
    compareLastFrame("./binaries/gcm_video_texturing/ps3frame0.png");
}

TEST_CASE("gcm_stencil_reflect") {
    runAndWait("./binaries/gcm_stencil_reflect/a.elf");
    compareLastFrame("./binaries/gcm_stencil_reflect/ps3frame0.png", 0, 5, 10);
    compareLastFrame("./binaries/gcm_stencil_reflect/ps3frame1.png", 1, 5, 10);
}

TEST_CASE("gcm_render_to_texture") {
    runAndWait("./binaries/gcm_render_to_texture/a.elf");
    compareLastFrame("./binaries/gcm_render_to_texture/ps3frame0.png", 0, 5, 10);
    compareLastFrame("./binaries/gcm_render_to_texture/ps3frame1.png", 1, 5, 10);
}

TEST_CASE("opengl_1_basiccg", TAG_SERIAL) {
    runAndWait("./binaries/opengl_1_basiccg/a.elf");
    compareLastFrame("./binaries/opengl_1_basiccg/ps3frame1.png", 1);
}

TEST_CASE("opengl_2_basic_vertex_lighting", TAG_SERIAL) {
    runAndWait("./binaries/opengl_2_basic_vertex_lighting/a.elf");
    compareLastFrame("./binaries/opengl_2_basic_vertex_lighting/ps3frame1.png", 1);
}

TEST_CASE("opengl_3_basic_fragment_lighting", TAG_SERIAL) {
    runAndWait("./binaries/opengl_3_basic_fragment_lighting/a.elf");
    compareLastFrame("./binaries/opengl_3_basic_fragment_lighting/ps3frame1.png", 1);
}

TEST_CASE("opengl_4_proc_anim", TAG_SERIAL) {
    runAndWait("./binaries/opengl_4_proc_anim/a.elf");
    compareLastFrame("./binaries/opengl_4_proc_anim/ps3frame1.png", 1);
}

TEST_CASE("opengl_5_mipmap", TAG_SERIAL) {
    runAndWait("./binaries/opengl_5_mipmap/a.elf");
    compareLastFrame("./binaries/opengl_5_mipmap/ps3frame1.png", 1, 5, 100);
}

TEST_CASE("opengl_6_gloss_map", TAG_SERIAL) {
    runAndWait("./binaries/opengl_6_gloss_map/a.elf");
    compareLastFrame("./binaries/opengl_6_gloss_map/ps3frame1.png", 1);
}

TEST_CASE("opengl_7_environment_map", TAG_SERIAL) {
    runAndWait("./binaries/opengl_7_environment_map/a.elf");
    compareLastFrame("./binaries/opengl_7_environment_map/ps3frame1.png", 1);
}

TEST_CASE("opengl_8_irradiance_map", TAG_SERIAL) {
    runAndWait("./binaries/opengl_8_irradiance_map/a.elf");
    compareLastFrame("./binaries/opengl_8_irradiance_map/ps3frame1.png", 1);
}

TEST_CASE("pngdec_ppu_graphics", TAG_SERIAL) {
    runAndWait("./binaries/pngdec_ppu/a.elf");
    compareLastFrame("./binaries/pngdec_ppu/ps3frame0.png", 0);
    runAndWait("", true);
    compareLastFrame("./binaries/pngdec_ppu/ps3frame0.png", 0);
}

TEST_CASE("gcm_strip_branch", TAG_SERIAL) {
    runAndWait("./binaries/gcm_strip_branch/a.elf", false, true);
    for (int i = 0; i <= 18; ++i) {
        compareLastFrame(
            ssnprintf("./binaries/gcm_strip_branch/ps3frame%d.png", i).c_str(),
            i, 30, 400);
    }
}

TEST_CASE("cube_with_font") {
    runAndWait("./binaries/cube_with_font/a.elf");
    compareLastFrame("./binaries/cube_with_font/ps3frame0.png", 0, 5, 100);
}

TEST_CASE("resc_basic") {
    runAndWait("./binaries/resc_basic/a.elf");
    compareLastFrame("./binaries/resc_basic/ps3frame0.png", 0);
}

TEST_CASE("particle_simulator_02_spu_particles") {
    runAndWait("./binaries/particle_simulator_02_spu_particles/a.elf");
    compareLastFrame("./binaries/particle_simulator_02_spu_particles/ps3frame1.png", 1, 5, 20);
}

TEST_CASE("particle_simulator_01_ppu_particles") {
    runAndWait("./binaries/particle_simulator_01_ppu_particles/a.elf");
    compareLastFrame("./binaries/particle_simulator_01_ppu_particles/ps3frame1.png", 1);
}

TEST_CASE("gcm_texture_cache") {
    runAndWait("./binaries/gcm_texture_cache/a.elf");
    compareLastFrame("./binaries/gcm_texture_cache/ps3frame0.png", 0);
    compareLastFrame("./binaries/gcm_texture_cache/ps3frame1.png", 1);
    compareLastFrame("./binaries/gcm_texture_cache/ps3frame2.png", 2);
    runAndWait("", true);
    compareLastFrame("./binaries/gcm_texture_cache/ps3frame0.png", 0);
    compareLastFrame("./binaries/gcm_texture_cache/ps3frame1.png", 1);
    compareLastFrame("./binaries/gcm_texture_cache/ps3frame2.png", 2);
}

TEST_CASE("ppu_codec_jdec") {
    runAndWait("./binaries/ppu_codec_jdec/a.elf", false, true);
    compareLastFrame("./binaries/ppu_codec_jdec/ps3frame0.png", 0);
}

TEST_CASE("spu_render") {
    runAndWait("./binaries/spu_render/a.elf", false, true);
    compareLastFrame("./binaries/spu_render/ps3frame0.png", 0);
}
