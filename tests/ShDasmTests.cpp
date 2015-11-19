#include "../ps3emu/shaders/shader_dasm.h"
#include <catch.hpp>

int dasm_print(uint8_t* ptr, std::string& res) {
    FragmentInstr instr;
    auto size = fragment_dasm_instr(ptr, instr);
    fragment_dasm(instr, res);
    return size;
}

bool vdasm_print(uint8_t* ptr, std::string& res) {
    std::array<VertexInstr, 2> instr;
    int count = vertex_dasm_instr(ptr, instr);
    bool last = false;
    for (int i = 0; i < count; ++i) {
        res += vertex_dasm(instr[i]);
        last |= instr[i].is_last;
    }
    return last;
}

TEST_CASE() {
    uint8_t instr[] = { 
        0x3E, 0x01, 0x01, 0x00, 0xC8, 0x01, 0x1C, 0x9D, 
        0xC8, 0x00, 0x00, 0x01, 0xC8, 0x00, 0x3F, 0xE1
    };
    std::string res;
    dasm_print(instr, res);
    REQUIRE(res == "MOVR R0, f[COL0]; # last instruction");
}

TEST_CASE() {
    unsigned char instr[] = {
        0x06, 0x00, 0x02, 0x00, 0xc8, 0x01, 0x1c, 0x9d, 0x00, 0x02, 0x00, 0x00,
        0xc8, 0x00, 0x3f, 0xe1, 0xd7, 0x0a, 0x3c, 0xa3, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00,
        0xc8, 0x00, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01,
        0x06, 0x00, 0x02, 0x00, 0xc8, 0x00, 0x1c, 0x9d, 0x00, 0x02, 0x00, 0x00,
        0xc8, 0x00, 0x00, 0x01, 0x00, 0x00, 0x42, 0x48, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x7e, 0x41, 0x00,
        0xc8, 0x00, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01,
        0x18, 0x00, 0x11, 0x00, 0x80, 0x00, 0x3c, 0x9c, 0xc8, 0x00, 0x00, 0x01,
        0xc8, 0x00, 0x00, 0x01, 0x10, 0x02, 0x01, 0x00, 0xc8, 0x00, 0x1c, 0x9d,
        0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01, 0x02, 0x00, 0x01, 0x00,
        0x54, 0x00, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01,
        0x10, 0x02, 0x01, 0x00, 0xc8, 0x00, 0x0a, 0xa7, 0xc8, 0x00, 0x00, 0x01,
        0xc8, 0x00, 0x00, 0x01, 0x02, 0x00, 0x01, 0x00, 0x54, 0x00, 0x00, 0x07,
        0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01, 0x04, 0x00, 0x01, 0x00,
        0xfe, 0x04, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01,
        0x18, 0x00, 0x03, 0x00, 0x80, 0x00, 0x1c, 0x9c, 0x00, 0x02, 0x00, 0x00,
        0xc8, 0x00, 0x00, 0x01, 0x00, 0x00, 0xc1, 0xc8, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0x00, 0x01, 0x00,
        0xc8, 0x01, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x3f, 0xe1,
        0x02, 0x02, 0x38, 0x00, 0x5c, 0x00, 0x1c, 0x9d, 0x5c, 0x00, 0x00, 0x01,
        0xc8, 0x00, 0x00, 0x01, 0x03, 0x7e, 0x4a, 0x00, 0xc8, 0x04, 0x1c, 0x9d,
        0x00, 0x02, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x01, 0x00, 0x00, 0x43, 0xc8,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x18, 0x00, 0x01, 0x00, 0x32, 0x02, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01,
        0xc8, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x7e, 0x7e, 0x00,
        0xc8, 0x00, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01,
        0x1e, 0x01, 0x01, 0x00, 0x80, 0x02, 0x00, 0x14, 0xc8, 0x00, 0x00, 0x01,
        0xc8, 0x00, 0x00, 0x01, 0x66, 0x66, 0x3f, 0x66, 0x00, 0x00, 0x3f, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    std::string res;
    int pos = 0, size;
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 32 );
    REQUIRE(res == "MULR R0.xy, f[WPOS], {0x3ca3d70a(0.02), 0x00000000(0), 0x00000000(0), 0x00000000(0)}.x;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "FRCR R0.xy, R0;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 32 );
    REQUIRE(res == "MULR R0.xy, R0, {0x42480000(50), 0x00000000(0), 0x00000000(0), 0x00000000(0)}.x;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "MOVRC RC.xy, R0;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "FLRR R0.zw, |R0.xxxy|;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "MOVR R1.w, R0;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "MOVR R0.x, R0.z;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "MOVR R1.w(LT.y), -R0;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "MOVR R0.x(LT.x), -R0.z;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "MOVR R0.y, R1.w;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 32 );
    REQUIRE(res == "ADDR R0.zw, R0.xxxy, {0xc1c80000(-25), 0x00000000(0), 0x00000000(0), 0x00000000(0)}.x;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "MOVR R0.xy, f[TEX0];");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "DP2R R1.x, R0.zwzz, R0.zwzz;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 32 );
    REQUIRE(res == "SLTRC RC.x, R1, {0x43c80000(400), 0x00000000(0), 0x00000000(0), 0x00000000(0)}.x;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 32 );
    REQUIRE(res == "MOVR R0.zw, {0x00000000(0), 0x00000000(0), 0x3f800000(1), 0x00000000(0)}.yzyz;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 16 );
    REQUIRE(res == "FENCBR;");
    res.clear();
    
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE( size == 32 );
    REQUIRE(res == "MOVR R0(NE.x), {0x3f666666(0.9), 0x3f800000(1), 0x00000000(0), 0x00000000(0)}.xxxy; # last instruction");
    res.clear();
}

TEST_CASE() {
    unsigned char instr[] = {
        0x3e, 0x84, 0x01, 0x40, 0xc8, 0x01, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01,
        0xc8, 0x00, 0x3f, 0xe1, 0x1e, 0x01, 0x03, 0x00, 0xc9, 0x08, 0x1f, 0xf5,
        0xc8, 0x08, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01
    };
    
    std::string res;
    int pos = 0, size;
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE(res == "MOVH H2, f[COL0];");
    res.clear();
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE(res == "ADDR R0(NE.w), H2, R2; # last instruction");
    res.clear();
}

TEST_CASE() {
    unsigned char instr[] = {
        0x40, 0x1f, 0x9c, 0x6c, 0x00, 0xdd, 0x23, 0x55, 0x01, 0x86, 0xc0, 0x83,
        0x60, 0x41, 0xff, 0x84, 0x00, 0x00, 0x9c, 0x6c, 0x00, 0x5d, 0x20, 0x00,
        0x01, 0x86, 0xc0, 0x83, 0x60, 0x40, 0x7f, 0xfc, 0x40, 0x1f, 0x9c, 0x6c,
        0x80, 0x5d, 0x30, 0x0d, 0x81, 0x06, 0xc0, 0x80, 0x00, 0x79, 0xe0, 0x9c,
        0x00, 0x00, 0x1c, 0x6c, 0x00, 0x5d, 0x20, 0x00, 0x01, 0x86, 0xc0, 0x83,
        0x60, 0x40, 0x7f, 0xfc, 0x00, 0x1f, 0x9c, 0x6c, 0x78, 0x1d, 0x30, 0x0d,
        0x81, 0x06, 0xc0, 0x80, 0x00, 0x70, 0x00, 0x7c, 0x00, 0x00, 0x1c, 0x6c,
        0x00, 0x40, 0x00, 0x2a, 0x82, 0x86, 0xc0, 0x83, 0x60, 0x40, 0x9f, 0xfc,
        0x00, 0x00, 0x9c, 0x6c, 0x00, 0x40, 0x00, 0x80, 0x00, 0x86, 0xc0, 0x83,
        0x60, 0x40, 0x9f, 0xfc, 0x00, 0x00, 0x1c, 0x6c, 0x00, 0x80, 0x00, 0x2a,
        0x81, 0x06, 0xc0, 0x43, 0x60, 0x41, 0xff, 0xfc, 0x00, 0x00, 0x1c, 0x6c,
        0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0xc1, 0x43, 0x60, 0x21, 0xff, 0xfc,
        0x00, 0x00, 0x1c, 0x6c, 0x01, 0x1d, 0x20, 0x55, 0x01, 0x01, 0x00, 0xc3,
        0x60, 0x21, 0xff, 0xfc, 0x00, 0x00, 0x1c, 0x6c, 0x01, 0x1d, 0x20, 0x7f,
        0x81, 0x00, 0x40, 0xc3, 0x60, 0x21, 0xff, 0xfc, 0x40, 0x1f, 0x9c, 0x6c,
        0x00, 0x5d, 0x10, 0x0d, 0x81, 0x86, 0xc0, 0x83, 0x60, 0x41, 0xff, 0xa0,
        0x00, 0x00, 0x9c, 0x6c, 0x00, 0x90, 0x10, 0x2a, 0x80, 0x86, 0xc0, 0xc3,
        0x60, 0x41, 0xff, 0xfc, 0x00, 0x00, 0x9c, 0x6c, 0x01, 0x10, 0x00, 0x00,
        0x00, 0x86, 0xc0, 0xc3, 0x60, 0xa1, 0xff, 0xfc, 0x00, 0x00, 0x9c, 0x6c,
        0x01, 0x10, 0x20, 0x55, 0x00, 0x86, 0xc0, 0xc3, 0x60, 0xa1, 0xff, 0xfc,
        0x40, 0x1f, 0x9c, 0x6c, 0x01, 0x10, 0x30, 0x7f, 0x80, 0x86, 0xc0, 0xc3,
        0x60, 0xa1, 0xff, 0x81
    };
    
    std::string res;
    REQUIRE( !vdasm_print(instr, res) );
    REQUIRE(res == "ADD o[1], c[466].z, v[3];");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16, res) );
    REQUIRE(res == "MOV R1.zw, c[466].x;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 2, res) );
    REQUIRE(res == "COS R1.xy, c[467].x;MOV o[7], v[0];");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 3, res) );
    REQUIRE(res == "MOV R0.zw, c[466].x;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 4, res) );
    REQUIRE(res == "SIN R0.x, c[467].x;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 5, res) );
    REQUIRE(res == "MOV R0.y, R1.y;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 6, res) );
    REQUIRE(res == "MOV R1.y, -R0.x;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 7, res) );
    REQUIRE(res == "MUL R0, v[0].y, R0;");
    res.clear();    
    
    REQUIRE( !vdasm_print(instr + 16 * 8, res) );
    REQUIRE(res == "MAD R0, v[0].x, R1, R0;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 9, res) );
    REQUIRE(res == "MAD R0, v[0].z, c[466].xxyx, R0;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 10, res) );
    REQUIRE(res == "MAD R0, v[0].w, c[466].xxxy, R0;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 11, res) );
    REQUIRE(res == "MOV o[8], c[465];");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 12, res) );
    REQUIRE(res == "MUL R1, R0.y, c[257];");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 13, res) );
    REQUIRE(res == "MAD R1, R0.x, c[256], R1;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 14, res) );
    REQUIRE(res == "MAD R1, R0.z, c[258], R1;");
    res.clear();
    
    REQUIRE( vdasm_print(instr + 16 * 15, res) );
    REQUIRE(res == "MAD o[0], R0.w, c[259], R1;");
    res.clear();
}

TEST_CASE() {
    unsigned char instr[] = {
        0x00, 0x00, 0x1c, 0x6c, 0x00, 0x40, 0x08, 0x08, 0x01, 0x06, 0xc0, 0x83,
        0x60, 0x41, 0x9f, 0xfc, 0x00, 0x00, 0x1c, 0x6c, 0x04, 0x40, 0x08, 0x00,
        0x01, 0x00, 0x00, 0x83, 0x60, 0x40, 0x5f, 0xfc, 0x00, 0x00, 0x1c, 0x6c,
        0x06, 0x40, 0x00, 0x09, 0x00, 0x86, 0xc0, 0x83, 0x60, 0x41, 0xff, 0xfc,
        0x40, 0x1f, 0x9c, 0x6c, 0x00, 0x5d, 0x20, 0x36, 0x81, 0x86, 0xc0, 0x83,
        0x60, 0x41, 0xdf, 0x84, 0x00, 0x00, 0x9c, 0x6c, 0x00, 0x5d, 0x20, 0x00,
        0x01, 0x86, 0xc0, 0x83, 0x60, 0x41, 0x1f, 0xfc, 0x20, 0x1f, 0xdc, 0x6c,
        0x04, 0x9d, 0x30, 0x55, 0x01, 0x80, 0x01, 0x43, 0x60, 0x41, 0x1f, 0xfc,
        0x00, 0x00, 0x9c, 0x6c, 0x00, 0x40, 0x00, 0x05, 0x81, 0x06, 0xc0, 0x83,
        0x60, 0x41, 0x7f, 0xfc, 0x00, 0x00, 0x34, 0x00, 0x00, 0x40, 0x00, 0x00,
        0x00, 0x86, 0xc0, 0x83, 0x60, 0x40, 0x3f, 0xfc, 0x00, 0x00, 0x1c, 0x6c,
        0x00, 0xc0, 0x00, 0x7f, 0x80, 0x86, 0xc0, 0xaa, 0xa0, 0x40, 0x3f, 0xfc,
        0x40, 0x1f, 0x9c, 0x6c, 0x00, 0x5d, 0x20, 0x00, 0x01, 0x86, 0xc0, 0x83,
        0x60, 0x40, 0x3f, 0x84, 0x00, 0x00, 0x9c, 0x6c, 0x01, 0x1d, 0x30, 0x7f,
        0x80, 0x95, 0x40, 0xca, 0xa0, 0x40, 0x9f, 0xfc, 0x40, 0x1f, 0xa8, 0x00,
        0x00, 0x40, 0x00, 0x0c, 0x00, 0x86, 0xc0, 0x83, 0x60, 0x41, 0xdf, 0x84,
        0x40, 0x1f, 0x9c, 0x6c, 0x01, 0xd0, 0x30, 0x0d, 0x82, 0x86, 0xc0, 0xc3,
        0x60, 0x40, 0x3f, 0x80, 0x40, 0x1f, 0x9c, 0x6c, 0x01, 0xd0, 0x20, 0x0d,
        0x82, 0x86, 0xc0, 0xc3, 0x60, 0x40, 0x5f, 0x80, 0x40, 0x1f, 0x9c, 0x6c,
        0x01, 0xd0, 0x10, 0x0d, 0x82, 0x86, 0xc0, 0xc3, 0x60, 0x40, 0x9f, 0x80,
        0x40, 0x1f, 0x9c, 0x6c, 0x01, 0xd0, 0x00, 0x0d, 0x82, 0x86, 0xc0, 0xc3,
        0x60, 0x41, 0x1f, 0x81
    };
    
    std::string res;
    REQUIRE( !vdasm_print(instr, res) );
    REQUIRE(res == "MOV R0.xy, v[8].xyxx;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16, res) );
    REQUIRE(res == "SFL R0.z, v[8].x, v[8].x;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 2, res) );
    REQUIRE(res == "TXL R0, R0.xyxz, TEX0;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 3, res) );
    REQUIRE(res == "MOV o[1].xyz, c[466].yzwy;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 4, res) );
    REQUIRE(res == "MOV R1.x, c[466].x;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 5, res) );
    REQUIRE(res == "SGTC RC.x, c[467].z, R1.x;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 6, res) );
    REQUIRE(res == "MOV R1.xzw, v[0].xxzw;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 7, res) );
    REQUIRE(res == "MOV R0.w(NE.x), R0.x;");
    res.clear();    
    
    REQUIRE( !vdasm_print(instr + 16 * 8, res) );
    REQUIRE(res == "ADD R0.w, R0.w, -v[0].y;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 9, res) );
    REQUIRE(res == "MOV o[1].w, c[466].x;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 10, res) );
    REQUIRE(res == "MAD R1.y, R0.w, c[467].y, v[0].y;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 11, res) );
    REQUIRE(res == "MOV o[1].xyz(EQ.x), R0.xyzx;");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 12, res) );
    REQUIRE(res == "DP4 o[0].w, R1, c[259];");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 13, res) );
    REQUIRE(res == "DP4 o[0].z, R1, c[258];");
    res.clear();
    
    REQUIRE( !vdasm_print(instr + 16 * 14, res) );
    REQUIRE(res == "DP4 o[0].y, R1, c[257];");
    res.clear();
    
    REQUIRE( vdasm_print(instr + 16 * 15, res) );
    REQUIRE(res == "DP4 o[0].x, R1, c[256];");
    res.clear();
}

TEST_CASE() {
    unsigned char instr[] = {
        0x82, 0x00, 0x17, 0x00, 0xc8, 0x01, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01,                                                            
        0xc8, 0x00, 0x3f, 0xe1, 0x14, 0x00, 0x01, 0x40, 0xa0, 0x02, 0x1c, 0x9c,                                                            
        0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,                                                            
        0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                            
        0x1e, 0x7e, 0x7e, 0x00, 0xc8, 0x00, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01,                                                            
        0xc8, 0x00, 0x00, 0x01, 0x0a, 0x01, 0x01, 0x40, 0x00, 0x00, 0x1c, 0x9c,                                                            
        0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01
    };
    
    std::string res;
    int pos = 0, size;
    size = dasm_print(instr + pos, res);
    pos += size;
    REQUIRE(res == "TEXR R0.x, f[TEX0], TEX0;");
    res.clear();
}