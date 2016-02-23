#include "../ps3emu/shaders/shader_dasm.h"
#include "../ps3emu/shaders/ShaderRewriter.h"
#include <catch.hpp>

using namespace ShaderRewriter;

std::string printStatements(std::vector<std::unique_ptr<Statement>>& stms) {
    std::string res;
    for (auto i = 0u; i < stms.size(); i++) {
        res += PrintStatement(stms[i].get());
    }
    return res;
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
    
    FragmentInstr fi;
    auto pos = 0;
    
    pos += fragment_dasm_instr(instr + pos, fi);
    auto st = MakeStatement(fi, 0);
    auto str = printStatements(st);
    REQUIRE( str == "r[0].xy = (f_WPOS * (fconst.c[0].xxxx)).xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].xy = fract(r[0]).xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].xy = (r[0] * (fconst.c[0].xxxx)).xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "c[0].xy = r[0].xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].zw = floor((abs(r[0]).xxxy)).zw;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[1].w = r[0].w;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].x = (r[0].zzzz).x;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == 
        "if (((c[0].yyyy).x < 0)) {\n"
        "    r[1].w = (-r[0]).w;\n"
        "}"
    );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == 
        "if (((c[0].xxxx).x < 0)) {\n"
        "    r[0].x = ((-r[0]).zzzz).x;\n"
        "}"
    );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].y = (r[1].wwww).y;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].zw = ((r[0].xxxy) + (fconst.c[0].xxxx)).zw;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].xy = f_TEX0.xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[1].x = dot((r[0].zwzz).xy, (r[0].zwzz).xy).x;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "c[0].x = float(lessThan(r[1], (fconst.c[0].xxxx)).x);" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].zw = (fconst.c[0].yzyz).zw;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == "" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(fi, 0);
    str = printStatements(st);
    REQUIRE( str == 
        "if (((c[0].xxxx).x != 0)) {\n"
        "    r[0] = (fconst.c[0].xxxy);\n"
        "}"
    );
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
    
    std::array<VertexInstr, 2> res;
    
    auto count = vertex_dasm_instr(instr, res);
    REQUIRE( count == 1 );
    auto st = MakeStatement(res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == "v_out[1] = ((constants.c[466].zzzz) + v_in[3]);");
    
    count = vertex_dasm_instr(instr + 16, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[1].zw = (constants.c[466].xxxx).zw;");
    
    count = vertex_dasm_instr(instr + 16 * 2, res);
    REQUIRE( count == 2 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[1].xy = cos((constants.c[467].xxxx)).xy;");
    st = MakeStatement(res[1], 0);
    str = printStatements(st);
    REQUIRE(str == "v_out[7] = v_in[0];");
    
    count = vertex_dasm_instr(instr + 16 * 3, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0].zw = (constants.c[466].xxxx).zw;");
    
    count = vertex_dasm_instr(instr + 16 * 4, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0].x = sin((constants.c[467].xxxx)).x;");
    
    count = vertex_dasm_instr(instr + 16 * 5, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0].y = (r[1].yyyy).y;");
    
    count = vertex_dasm_instr(instr + 16 * 6, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[1].y = ((-r[0]).xxxx).y;");
    
    count = vertex_dasm_instr(instr + 16 * 7, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0] = ((v_in[0].yyyy) * r[0]);");
    
    count = vertex_dasm_instr(instr + 16 * 8, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0] = (((v_in[0].xxxx) * r[1]) + r[0]);");
    
    count = vertex_dasm_instr(instr + 16 * 9, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0] = (((v_in[0].zzzz) * (constants.c[466].xxyx)) + r[0]);");
    
    count = vertex_dasm_instr(instr + 16 * 10, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0] = (((v_in[0].wwww) * (constants.c[466].xxxy)) + r[0]);");
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
    
    std::array<VertexInstr, 2> res;
    
    auto count = vertex_dasm_instr(instr + 16 * 2, res);
    REQUIRE( count == 1 );
    auto st = MakeStatement(res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == "r[0] = txl0((r[0].xyxz));");
    
    count = vertex_dasm_instr(instr + 16 * 5, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "c[0].x = float(greaterThan((constants.c[467].zzzz), (r[1].xxxx)).x);");
    
    count = vertex_dasm_instr(instr + 16 * 7, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == 
        "if (((c[0].xxxx).x != 0)) {\n"
        "    r[0].w = (r[0].xxxx).w;\n"
        "}"
    );
    
    count = vertex_dasm_instr(instr + 16 * 12, res);
    REQUIRE( count == 1 );
    st = MakeStatement(res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "v_out[0].w = dot(r[1], constants.c[259]).x;");
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
    
    FragmentInstr fi;
    auto pos = 0;
    
    pos += fragment_dasm_instr(instr + pos, fi);
    auto st = MakeStatement(fi, 0);
    auto str = printStatements(st);
    REQUIRE( str == "r[0].x = tex0(f_TEX0).x;" );
}

TEST_CASE() {
    unsigned char instr[] = {
        0x00, 0x00, 0x1c, 0x6c, 0x00, 0x40, 0x08, 0x08, 0x01, 0x06, 0xc0, 0x83,
        0x60, 0x41, 0x9f, 0xfc, 0x00, 0x00, 0x1c, 0x6c, 0x04, 0x40, 0x08, 0x00,
        0x01, 0x00, 0x00, 0x83, 0x60, 0x40, 0x5f, 0xfc, 0x00, 0x00, 0x1c, 0x6c,
        0x06, 0x40, 0x00, 0x09, 0x00, 0x86, 0xc0, 0x83, 0x60, 0x41, 0x1f, 0xfc,
        0x00, 0x00, 0x9c, 0x6c, 0x00, 0x90, 0x10, 0x2a, 0x81, 0x06, 0xc0, 0xc3,
        0x60, 0x41, 0xff, 0xfc, 0x00, 0x00, 0x9c, 0x6c, 0x01, 0x10, 0x00, 0x00,
        0x01, 0x06, 0xc0, 0xc3, 0x60, 0xa1, 0xff, 0xfc, 0x00, 0x00, 0x9c, 0x6c,
        0x01, 0x10, 0x20, 0x55, 0x01, 0x06, 0xc0, 0xc3, 0x60, 0xa1, 0xff, 0xfc,
        0x00, 0x00, 0x9c, 0x6c, 0x01, 0x10, 0x30, 0x7f, 0x81, 0x06, 0xc0, 0xc3,
        0x60, 0xa1, 0xff, 0xfc, 0x40, 0x1f, 0x9c, 0x6c, 0x00, 0x5d, 0x30, 0x00,
        0x01, 0x86, 0xc0, 0x83, 0x60, 0x41, 0x7f, 0x84, 0x20, 0x1f, 0xdc, 0x6c,
        0x04, 0x9d, 0x30, 0x00, 0x02, 0xaa, 0x80, 0xc3, 0x60, 0x41, 0x1f, 0xfc,
        0x00, 0x00, 0x9c, 0x6c, 0x00, 0x9d, 0x30, 0x0d, 0x82, 0x84, 0x00, 0xc3,
        0x60, 0x41, 0xff, 0xfc, 0x40, 0x1f, 0x9c, 0x6c, 0x00, 0x40, 0x00, 0x0d,
        0x82, 0x86, 0xc0, 0x83, 0x60, 0x41, 0xff, 0x80, 0x00, 0x00, 0x1c, 0x6c,
        0x00, 0x40, 0x00, 0x00, 0x02, 0x86, 0xc0, 0x83, 0x60, 0x40, 0x9f, 0xfc,
        0x40, 0x1f, 0x8c, 0x00, 0x48, 0x40, 0x00, 0x00, 0x00, 0x86, 0xc0, 0x83,
        0x40, 0x40, 0x9f, 0x84, 0x00, 0x00, 0x1c, 0x6c, 0x00, 0x9d, 0x20, 0x2a,
        0x80, 0x80, 0x00, 0xc3, 0x60, 0x41, 0x1f, 0xfc, 0x20, 0x1f, 0xdc, 0x6c,
        0x00, 0x40, 0x00, 0x00, 0x00, 0x86, 0xc0, 0x83, 0x60, 0x41, 0x1f, 0xfc,
        0x00, 0x20, 0x1c, 0x6c, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x86, 0xc0, 0x83,
        0x60, 0x40, 0x9f, 0xfc, 0x00, 0x00, 0x1c, 0x6c, 0x00, 0x40, 0x00, 0x2a,
        0x80, 0x86, 0xc0, 0x83, 0x60, 0x41, 0x1f, 0xfc, 0x00, 0x00, 0x24, 0x6c,
        0x00, 0x40, 0x00, 0xaa, 0x80, 0x86, 0xc0, 0x83, 0x60, 0x41, 0x1f, 0xfc,
        0x20, 0x1f, 0xdc, 0x6c, 0x04, 0x9d, 0x20, 0x00, 0x00, 0xd5, 0x40, 0xc3,
        0x60, 0x41, 0x1f, 0xfc, 0x00, 0x00, 0x9c, 0x6c, 0x00, 0xdd, 0x20, 0x2a,
        0x81, 0x86, 0xc0, 0x80, 0x00, 0x21, 0x1f, 0xfc, 0x40, 0x00, 0x1c, 0x6c,
        0x00, 0x5d, 0x30, 0x7f, 0x81, 0x86, 0xc0, 0x83, 0x60, 0x41, 0xff, 0x84,
        0x00, 0x1f, 0x88, 0x00, 0x48, 0x00, 0x00, 0x0d, 0x81, 0x06, 0xc0, 0x83,
        0x40, 0x40, 0x1f, 0xfc, 0x20, 0x1f, 0xdc, 0x6c, 0x04, 0x9d, 0x30, 0x00,
        0x02, 0x80, 0x00, 0xc3, 0x60, 0x41, 0x1f, 0xfc, 0x00, 0x01, 0x1c, 0x6c,
        0x00, 0xdd, 0x20, 0x55, 0x01, 0x86, 0xc0, 0x83, 0x60, 0x21, 0xff, 0xfc,
        0x00, 0x00, 0x9c, 0x6c, 0x00, 0xdd, 0x30, 0x80, 0x01, 0x86, 0xc0, 0x80,
        0x00, 0xa1, 0x1f, 0xfc, 0x40, 0x00, 0x14, 0x00, 0x48, 0x40, 0x00, 0x0d,
        0x84, 0x86, 0xc0, 0x82, 0xc0, 0x41, 0xff, 0x84, 0x00, 0x1f, 0x9c, 0x6c,
        0x00, 0x00, 0x00, 0x0d, 0x81, 0x06, 0xc0, 0x83, 0x60, 0x40, 0x1f, 0xfd
    };
    
    std::array<VertexInstr, 2> res;
    
    auto count = vertex_dasm_instr(instr + 16 * 25, res);
    REQUIRE( count == 2 );
    auto st = MakeStatement(res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == "v_out[1] = r[2];r[0] = r[2];");
}