#include <catch2/catch.hpp>

#include <QProcess>
#include "TestUtils.h"
#include "ps3emu/utils.h"
#include "ps3emu/state.h"
#include "ps3emu/Config.h"

static auto runnerPath = "../ps3run/ps3run";
static auto gcmvizPath = "../gcmviz/gcmviz";

int comparisonNum = 0;
int lastProcId = 0;

void compareLastFrame(std::string expected, int n = 0, int tolerance = 5, int safePixels = 0) {
    auto testDir = getTestOutputDir();
    comparisonNum++;
    QProcess proc;
    auto id = lastProcId;
    auto args = QStringList() 
        << "-metric"
        << "AE"
        << "-alpha" << "opaque"
        << "-fuzz" << QString("%1%%").arg(tolerance)
        << QString::fromStdString(ssnprintf("%s/ps3frame_%d_%d.png", testDir, id, n)) << expected.c_str()
        << QString::fromStdString(ssnprintf("%s/ps3frame-diff_%d_%d.png", testDir, id, n));
    proc.start("compare", args);
    proc.waitForFinished(-1);
    REQUIRE( proc.exitCode() != 2 );
    auto output = QString(proc.readAllStandardError()).toInt();
    if (output > safePixels) {
        args = QStringList() 
            << QString::fromStdString(ssnprintf("%s/ps3frame-diff_%d_%d.png", testDir, id, n))
            << QString::fromStdString(ssnprintf("%s/ps3frame-diff_%d_%d_bad%d.png", testDir, id, n, comparisonNum));
        proc.start("cp", args);
        proc.waitForFinished(-1);
        args = QStringList() 
            << QString::fromStdString(ssnprintf("%s/ps3frame_%d_%d.png", testDir, id, n))
            << QString::fromStdString(ssnprintf("%s/ps3frame_%d_%d_bad%d.png", testDir, id, n, comparisonNum));
        proc.start("cp", args);
        proc.waitForFinished(-1);
        REQUIRE( proc.exitCode() == 0 );
    }
    CHECK( output <= safePixels );
}

void runAndWait(std::string path, bool gcmviz = false, bool nocapture = false) {
    QProcess proc;
    QStringList args;
    if (gcmviz) {
        args << "--trace"
             << QString::fromStdString(ssnprintf("/tmp/ps3emu_%d.trace", lastProcId))
             << "--replay";
    } else {
        args << "--elf" << path.c_str();
        if (!nocapture) {
            args << "--capture-rsx";
        }
    }   
    proc.start(gcmviz ? gcmvizPath : runnerPath, args);
    lastProcId = proc.processId();
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
        WARN(path);
        REQUIRE( proc.exitCode() == 0 );
    }
}

TEST_CASE("gcm_hello") {
    runAndWait(testPath("gcm_hello/a.elf"));
    compareLastFrame(testPath("gcm_hello/ps3frame.png"));
    runAndWait("", true);
    compareLastFrame(testPath("gcm_hello/ps3frame.png"));
}

TEST_CASE("gcm_simple_fshader") {
    runAndWait(testPath("gcm_simple_fshader/a.elf"));
    compareLastFrame(testPath("gcm_simple_fshader/ps3frame.png"));
    runAndWait("", true);
    compareLastFrame(testPath("gcm_simple_fshader/ps3frame.png"));
}

TEST_CASE("gcm_simple_shaders") {
    runAndWait(testPath("gcm_simple_shaders/a.elf"));
    compareLastFrame(testPath("gcm_simple_shaders/ps3frame.png"));
    runAndWait("", true);
    compareLastFrame(testPath("gcm_simple_shaders/ps3frame.png"));
}

TEST_CASE("gcm_vertex_texture_wrapping") {
    runAndWait(testPath("gcm_vertex_texture_wrapping/a.elf"));
    compareLastFrame(testPath("gcm_vertex_texture_wrapping/ps3frame.png"));
}

TEST_CASE("gcm_vertex_texture_wrapping3") {
    runAndWait(testPath("gcm_vertex_texture_wrapping3/a.elf"));
    compareLastFrame(testPath("gcm_vertex_texture_wrapping3/ps3frame.png"));
}

TEST_CASE("gcm_vertex_texture") {
    runAndWait(testPath("gcm_vertex_texture/a.elf"));
    compareLastFrame(testPath("gcm_vertex_texture/ps3frame.png"));
}

TEST_CASE("gcm_cube") {
    runAndWait(testPath("gcm_cube/a.elf"));
    compareLastFrame(testPath("gcm_cube/ps3frame.png"));
    runAndWait("", true);
    compareLastFrame(testPath("gcm_cube/ps3frame.png"));
}

TEST_CASE("gcm_cube_mrt") {
    runAndWait(testPath("gcm_cube_mrt/a.elf"));
    compareLastFrame(testPath("gcm_cube_mrt/ps3frame.png"));
}

TEST_CASE("gcm_human") {
    runAndWait(testPath("gcm_human/a.elf"));
    compareLastFrame(testPath("gcm_human/ps3frame0.png"), 0, 20, 20);
    compareLastFrame(testPath("gcm_human/ps3frame1.png"), 1, 20, 20);
    compareLastFrame(testPath("gcm_human/ps3frame2.png"), 2, 20, 20);
}

TEST_CASE("gcm_vertex_shader_branch") {
    runAndWait(testPath("gcm_vertex_shader_branch/a.elf"));
    compareLastFrame(testPath("gcm_vertex_shader_branch/ps3frame.png"));
}

TEST_CASE("gcm_fragment_texture") {
    runAndWait(testPath("gcm_fragment_texture/a.elf"));
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame0.png"), 0);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame1.png"), 1);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame2.png"), 2);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame3.png"), 3);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame4.png"), 4);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame5.png"), 5);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame6.png"), 6);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame7.png"), 7);
    runAndWait("", true);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame0.png"), 0);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame1.png"), 1);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame2.png"), 2);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame3.png"), 3);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame4.png"), 4);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame5.png"), 5);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame6.png"), 6);
    compareLastFrame(testPath("gcm_fragment_texture/ps3frame7.png"), 7);
}

TEST_CASE("simple_console_gcm") {
    runAndWait(testPath("simple_console_gcm/a.elf"));
    compareLastFrame(testPath("simple_console_gcm/ps3frame0.png"));
}

TEST_CASE("gcm_dice") {
    runAndWait(testPath("gcm_dice/a.elf"));
    compareLastFrame(testPath("gcm_dice/ps3frame0.png"));
}

TEST_CASE("gcm_flip_handler") {
    runAndWait(testPath("gcm_flip_handler/a.elf"));
    compareLastFrame(testPath("gcm_flip_handler/ps3frame0.png"));
}

TEST_CASE("opengl_createdevice") {
    runAndWait(testPath("opengl_createdevice/a.elf"));
    compareLastFrame(testPath("opengl_createdevice/ps3frame1.png"), 1);
}

TEST_CASE("gcm_duck") {
    runAndWait(testPath("gcm_duck/a.elf"));
    compareLastFrame(testPath("gcm_duck/ps3frame0.png"), 0, 25, 20);
}

TEST_CASE("gcm_video_texturing") {
    runAndWait(testPath("gcm_video_texturing/a.elf"));
    compareLastFrame(testPath("gcm_video_texturing/ps3frame0.png"));
}

TEST_CASE("gcm_stencil_reflect") {
    runAndWait(testPath("gcm_stencil_reflect/a.elf"));
    compareLastFrame(testPath("gcm_stencil_reflect/ps3frame0.png"), 0, 5, 10);
    compareLastFrame(testPath("gcm_stencil_reflect/ps3frame1.png"), 1, 5, 10);
}

TEST_CASE("gcm_render_to_texture") {
    runAndWait(testPath("gcm_render_to_texture/a.elf"));
    compareLastFrame(testPath("gcm_render_to_texture/ps3frame0.png"), 0, 5, 10);
    compareLastFrame(testPath("gcm_render_to_texture/ps3frame1.png"), 1, 5, 10);
}

TEST_CASE("opengl_1_basiccg", TAG_SERIAL) {
    runAndWait(testPath("opengl_1_basiccg/a.elf"));
    compareLastFrame(testPath("opengl_1_basiccg/ps3frame1.png"), 1);
}

TEST_CASE("opengl_2_basic_vertex_lighting", TAG_SERIAL) {
    runAndWait(testPath("opengl_2_basic_vertex_lighting/a.elf"));
    compareLastFrame(testPath("opengl_2_basic_vertex_lighting/ps3frame1.png"), 1);
}

TEST_CASE("opengl_3_basic_fragment_lighting", TAG_SERIAL) {
    runAndWait(testPath("opengl_3_basic_fragment_lighting/a.elf"));
    compareLastFrame(testPath("opengl_3_basic_fragment_lighting/ps3frame1.png"), 1);
}

TEST_CASE("opengl_4_proc_anim", TAG_SERIAL) {
    runAndWait(testPath("opengl_4_proc_anim/a.elf"));
    compareLastFrame(testPath("opengl_4_proc_anim/ps3frame1.png"), 1);
}

TEST_CASE("opengl_5_mipmap", TAG_SERIAL) {
    runAndWait(testPath("opengl_5_mipmap/a.elf"));
    compareLastFrame(testPath("opengl_5_mipmap/ps3frame1.png"), 1, 5, 100);
}

TEST_CASE("opengl_6_gloss_map", TAG_SERIAL) {
    runAndWait(testPath("opengl_6_gloss_map/a.elf"));
    compareLastFrame(testPath("opengl_6_gloss_map/ps3frame1.png"), 1);
}

TEST_CASE("opengl_7_environment_map", TAG_SERIAL) {
    runAndWait(testPath("opengl_7_environment_map/a.elf"));
    compareLastFrame(testPath("opengl_7_environment_map/ps3frame1.png"), 1);
}

TEST_CASE("opengl_8_irradiance_map", TAG_SERIAL) {
    runAndWait(testPath("opengl_8_irradiance_map/a.elf"));
    compareLastFrame(testPath("opengl_8_irradiance_map/ps3frame1.png"), 1);
}

TEST_CASE("pngdec_ppu_graphics", TAG_SERIAL) {
    runAndWait(testPath("pngdec_ppu/a.elf"));
    compareLastFrame(testPath("pngdec_ppu/ps3frame0.png"), 0);
    runAndWait("", true);
    compareLastFrame(testPath("pngdec_ppu/ps3frame0.png"), 0);
}

TEST_CASE("gcm_strip_branch", TAG_SERIAL) {
    runAndWait(testPath("gcm_strip_branch/a.elf"), false, true);
    for (int i = 0; i <= 18; ++i) {
        compareLastFrame(
            testPath(ssnprintf("gcm_strip_branch/ps3frame%d.png", i).c_str()),
            i, 30, 400);
    }
}

TEST_CASE("cube_with_font") {
    runAndWait(testPath("cube_with_font/a.elf"));
    compareLastFrame(testPath("cube_with_font/ps3frame0.png"), 0, 5, 100);
}

TEST_CASE("resc_basic") {
    runAndWait(testPath("resc_basic/a.elf"));
    compareLastFrame(testPath("resc_basic/ps3frame0.png"), 0);
}

TEST_CASE("particle_simulator_02_spu_particles") {
    runAndWait(testPath("particle_simulator_02_spu_particles/a.elf"));
    compareLastFrame(testPath("particle_simulator_02_spu_particles/ps3frame1.png"), 1, 5, 20);
}

TEST_CASE("particle_simulator_01_ppu_particles") {
    runAndWait(testPath("particle_simulator_01_ppu_particles/a.elf"));
    compareLastFrame(testPath("particle_simulator_01_ppu_particles/ps3frame1.png"), 1);
}

TEST_CASE("gcm_texture_cache") {
    runAndWait(testPath("gcm_texture_cache/a.elf"));
    compareLastFrame(testPath("gcm_texture_cache/ps3frame0.png"), 0);
    compareLastFrame(testPath("gcm_texture_cache/ps3frame1.png"), 1);
    compareLastFrame(testPath("gcm_texture_cache/ps3frame2.png"), 2);
    runAndWait("", true);
    compareLastFrame(testPath("gcm_texture_cache/ps3frame0.png"), 0);
    compareLastFrame(testPath("gcm_texture_cache/ps3frame1.png"), 1);
    compareLastFrame(testPath("gcm_texture_cache/ps3frame2.png"), 2);
}

TEST_CASE("ppu_codec_jdec") {
    runAndWait(testPath("ppu_codec_jdec/a.elf"), false, true);
    compareLastFrame(testPath("ppu_codec_jdec/ps3frame0.png"), 0);
}

TEST_CASE("spu_render") {
    runAndWait(testPath("spu_render/a.elf"), false, true);
    compareLastFrame(testPath("spu_render/ps3frame0.png"), 0);
}

TEST_CASE("performance_tips_advanced_poisson") {
    runAndWait(testPath("performance_tips_advanced_poisson/a.elf"), false, true);
    compareLastFrame(testPath("performance_tips_advanced_poisson/ps3frame0.png"), 0);
}

TEST_CASE("edge_fragment_patch_sample") {
    runAndWait(testPath("edge_fragment_patch_sample/a.elf"), false, true);
    compareLastFrame(testPath("edge_fragment_patch_sample/ps3frame0.png"), 0);
}

TEST_CASE("np_basic_render") {
    runAndWait(testPath("np_basic_render/a.elf"), false, true);
    compareLastFrame(testPath("np_basic_render/ps3frame0.png"), 0);
}
