#include "../ps3emu/shaders/shader_dasm.h"
#include "../ps3emu/shaders/ShaderRewriter.h"
#include <catch/catch.hpp>

using namespace ShaderRewriter;

std::string printStatements(std::vector<Statement*>& stms) {
    std::string res;
    for (auto i = 0u; i < stms.size(); i++) {
        res += PrintStatement(stms[i]);
    }
    return res;
}

TEST_CASE("shader_rewriter_0") {
    ASTContext context;
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
    auto st = MakeStatement(context, fi, 0);
    auto str = printStatements(st);
    REQUIRE( str == "r[0].xy = (f_WPOS * (fconst.c[0].xxxx)).xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].xy = fract(r[0]).xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].xy = (r[0] * (fconst.c[0].xxxx)).xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "c[0].xy = r[0].xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].zw = floor((abs(r[0]).xxxy)).zw;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[1].w = r[0].w;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].x = (r[0].zzzz).x;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE(str ==
            "r[1].w = mix(r[1], (-r[0]), lessThan((c[0].yyyy), vec4(0))).w;");
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE(
        str ==
        "r[0].x = mix(r[0], ((-r[0]).zzzz), lessThan((c[0].xxxx), vec4(0))).x;");
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].y = (r[1].wwww).y;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].zw = ((r[0].xxxy) + (fconst.c[0].xxxx)).zw;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].xy = f_TEX0.xy;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[1].x = dot((r[0].zwzz).xy, (r[0].zwzz).xy);" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "c[0].x = vec4(lessThan(r[1], (fconst.c[0].xxxx))).x;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "r[0].zw = (fconst.c[0].yzyz).zw;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == "while (false) { }" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE(str =="r[0] = mix(r[0], (fconst.c[0].xxxy), notEqual((c[0].xxxx), vec4(0)));");
}

TEST_CASE("shader_rewriter_1") {
    ASTContext context;
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
    auto st = MakeStatement(context, res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == "v_out[1] = ((constants.c[466].zzzz) + v_in[3]);");
    
    count = vertex_dasm_instr(instr + 16, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[1].zw = (constants.c[466].xxxx).zw;");
    
    count = vertex_dasm_instr(instr + 16 * 2, res);
    REQUIRE( count == 2 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[1].xy = cos(((constants.c[467].xxxx).xxxx)).xy;");
    st = MakeStatement(context, res[1], 0);
    str = printStatements(st);
    REQUIRE(str == "v_out[7] = v_in[0];");
    
    count = vertex_dasm_instr(instr + 16 * 3, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0].zw = (constants.c[466].xxxx).zw;");
    
    count = vertex_dasm_instr(instr + 16 * 4, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0].x = sin(((constants.c[467].xxxx).xxxx)).x;");
    
    count = vertex_dasm_instr(instr + 16 * 5, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0].y = (r[1].yyyy).y;");
    
    count = vertex_dasm_instr(instr + 16 * 6, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[1].y = ((-r[0]).xxxx).y;");
    
    count = vertex_dasm_instr(instr + 16 * 7, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0] = ((v_in[0].yyyy) * r[0]);");
    
    count = vertex_dasm_instr(instr + 16 * 8, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0] = (((v_in[0].xxxx) * r[1]) + r[0]);");
    
    count = vertex_dasm_instr(instr + 16 * 9, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0] = (((v_in[0].zzzz) * (constants.c[466].xxyx)) + r[0]);");
    
    count = vertex_dasm_instr(instr + 16 * 10, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[0] = (((v_in[0].wwww) * (constants.c[466].xxxy)) + r[0]);");
}

TEST_CASE("shader_rewriter_2") {
    ASTContext context;
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
    auto st = MakeStatement(context, res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == "r[0] = txl0((r[0].xyxz));");
    
    count = vertex_dasm_instr(instr + 16 * 5, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "c[0].x = vec4(greaterThan((constants.c[467].zzzz), (r[1].xxxx))).x;");
    
    count = vertex_dasm_instr(instr + 16 * 7, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(
        str ==
        "r[0].w = mix(r[0], (r[0].xxxx), notEqual((c[0].xxxx), vec4(0))).w;");
    
    count = vertex_dasm_instr(instr + 16 * 12, res);
    REQUIRE( count == 1 );
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "v_out[0].w = dot(r[1], constants.c[259]);");
}

TEST_CASE("shader_rewriter_3") {
    ASTContext context;
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
    auto st = MakeStatement(context, fi, 0);
    auto str = printStatements(st);
    REQUIRE( str == "r[0].x = tex0(f_TEX0).x;" );
}

TEST_CASE("shader_rewriter_4") {
    ASTContext context;
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
    auto st = MakeStatement(context, res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == "v_out[1] = r[2];r[0] = r[2];");
}

TEST_CASE("shader_rewriter_5") {
    ASTContext context;
    unsigned char instr[] = {
        0xa2, 0x00, 0x06, 0x00, 0xc8, 0x01, 0x1c, 0x9d, 0xc8, 0x02, 0x50, 0x01, 
        0xc8, 0x00, 0x3f, 0xe1, 0x3e, 0x99, 0x99, 0x99, 0x00, 0x00, 0x00, 0x00, 
        0x3f, 0x4c, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 
    };
    
    FragmentInstr fi;
    auto pos = 0;
    
    pos += fragment_dasm_instr(instr + pos, fi);
    auto st = MakeStatement(context, fi, 0);
    auto str = printStatements(st);
    REQUIRE( str == "r[0].x = (0.5 * dot(f_TEX1, fconst.c[0]));" );
}

TEST_CASE("shader_rewriter_dph") {
    // DPH R0.y, v[0].xyzx, R4;
    ASTContext context;
    unsigned char instr[] = {
        0x00, 0x00, 0x1c, 0x6c, 0x01, 0x80, 0x00, 0x0c, 0x01, 0x06, 0xc4, 0x43, 
        0x60, 0x40, 0x9f, 0xfc, 
    };
    
    std::array<VertexInstr, 2> res;
    auto count = vertex_dasm_instr(instr, res);
    auto st = MakeStatement(context, res[0], 0);
    auto str = printStatements(st);
    REQUIRE( str == "r[0].y = dot(vec4((v_in[0].xyzx).xyz, 1), r[4]);" );
}

TEST_CASE("shader_rewriter_slexc_norm_kilr") {
    ASTContext context;
    unsigned char instr[] = {
        0x11, 0x7e, 0x4c, 0x80, 0x00, 0x04, 0x1c, 0x9c, 0x00, 0x02, 0x01, 0x68, 0xc8, 0x00, 0x00, 0x01, 0x80, 0x81, 0x3b, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x06, 0x7f, 0x52, 0x00, 0xc8, 0x00, 0x1f, 0xf5, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01, 
    };
    
    FragmentInstr fi;
    auto pos = 0;
    
    pos += fragment_dasm_instr(instr + pos, fi);
    auto st = MakeStatement(context, fi, 0);
    auto str = printStatements(st);
    REQUIRE( str == "c[0].w = clamp(vec4(lessThanEqual((r[1].xxxx), (fconst.c[0].xxxx))), -2, 2).w;" );
    
    pos += fragment_dasm_instr(instr + pos, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE( str == 
        "if (any(notEqual((c[0].wwww), vec4(0)))) {\n"
        "    discard;\n"
        "}"
    );
}

TEST_CASE("shader_rewriter_cond_y") {
    // MOV R3.y(NE), R5.x;
    ASTContext context;
    unsigned char instr[] = {
        0x00, 0x01, 0xb4, 0x6c, 0x00, 0x40, 0x00, 0x00, 0x0a, 0x86, 0xc0, 0x83, 0x60, 0x40, 0x9f, 0xfc,
    };
    
    std::array<VertexInstr, 2> res;
    auto count = vertex_dasm_instr(instr, res);
    auto st = MakeStatement(context, res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == "r[3].y = mix(r[3], (r[5].xxxx), notEqual((c[0]), vec4(0))).y;");
}

TEST_CASE("shader_rewriter_conditions") {
    // MAD R4(GT), v[0].w, c[270], R6;
    // MOV R5.xyz(EQ.x), c[465].xyzx;
    // MUL R14.xyz(NE.x), R4.w, R4.xyzx;
    // MOV R4(EQ.x), c[464];
    ASTContext context;
    unsigned char instr[] = {
        0x00, 0x02, 0x30, 0x6c, 0x01, 0x10, 0xe0, 0x7f, 0x81, 0x06, 0xc0, 0xc3, 0x63, 0x21, 0xff, 0xfc,
        0x00, 0x02, 0xa8, 0x00, 0x00, 0x5d, 0x10, 0x0c, 0x01, 0x86, 0xc0, 0x83, 0x60, 0x41, 0xdf, 0xfc,
        0x00, 0x07, 0x34, 0x00, 0x00, 0x80, 0x00, 0x7f, 0x88, 0x86, 0x04, 0x43, 0x60, 0x41, 0xdf, 0xfc,
        0x00, 0x02, 0x28, 0x00, 0x00, 0x5d, 0x00, 0x0d, 0x81, 0x86, 0xc0, 0x83, 0x60, 0x41, 0xff, 0xfc,
    };
    
    std::array<VertexInstr, 2> res;
    vertex_dasm_instr(instr, res);
    auto st = MakeStatement(context, res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == "r[4] = mix(r[4], (((v_in[0].wwww) * constants.c[270]) + r[6]), greaterThan((c[0]), vec4(0)));");
    
    vertex_dasm_instr(instr + 16, res);
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[5].xyz = mix(r[5], (constants.c[465].xyzx), equal((c[0].xxxx), vec4(0))).xyz;");
    
    vertex_dasm_instr(instr + 32, res);
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[14].xyz = mix(r[14], ((r[4].wwww) * (r[4].xyzx)), notEqual((c[0].xxxx), vec4(0))).xyz;");
    
    vertex_dasm_instr(instr + 48, res);
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == "r[4] = mix(r[4], constants.c[464], equal((c[0].xxxx), vec4(0)));");
}

TEST_CASE("shader_rewriter_vertex_branch") {
    // RI(EQ.x) L26;
    ASTContext context;
    unsigned char instr[] = {
        0x00, 0x1f, 0x88, 0x00, 0x48, 0x00, 0x00, 0x0d, 0x81, 0x06, 0xc0, 0x83, 0x40, 0x40, 0x1f, 0xfc,
    };
    std::array<VertexInstr, 2> res;
    vertex_dasm_instr(instr, res);
    auto st = MakeStatement(context, res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == 
        "if (any(equal((c[0].xxxx), vec4(0)))) {\n"
        "    nip = 26;\n"
        "    break;\n"
        "}"
    );
}

TEST_CASE("shader_rewriter_vertex_incompatible_lhs_rhs") {
    // DP4 R5.xy, v[0], c[258];
    // ARL A0.x, R2.x;
    // RCP R5.w, R5.w; MUL R6.w, c[A0.x+411].y, R6.w;
    ASTContext context;
    unsigned char instr[] = {
        0x00, 0x02, 0x9c, 0x6c, 0x01, 0xd0, 0x20, 0x0d, 0x81, 0x06, 0xc0, 0xc3, 0x60, 0x41, 0x9f, 0xfc,
        0x00, 0x00, 0x1c, 0x6c, 0x03, 0x40, 0x00, 0x00, 0x04, 0x86, 0xc0, 0x83, 0x60, 0x41, 0x1f, 0xfc,
        0x00, 0x03, 0x1c, 0x6c, 0x10, 0x99, 0xb0, 0x2a, 0x81, 0xbf, 0xc6, 0x5f, 0xe2, 0xa2, 0x22, 0xfe,
    };
    std::array<VertexInstr, 2> res;
    vertex_dasm_instr(instr, res);
    auto st = MakeStatement(context, res[0], 0);
    auto str = printStatements(st);
    REQUIRE(str == 
        "r[5].xy = vec4(dot(v_in[0], constants.c[258])).xy;"
    );
    
    vertex_dasm_instr(instr + 16, res);
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == 
        "a[0].xy = ivec4(floor((r[2].xxxx))).xy;"
    );
    
    vertex_dasm_instr(instr + 32, res);
    st = MakeStatement(context, res[0], 0);
    str = printStatements(st);
    REQUIRE(str == 
        "r[5].w = pow(((r[5].wwww).xxxx), vec4(-1)).w;"
    );
}

TEST_CASE("shader_rewriter_fragment_append_c") {
    ASTContext context;
    // MINRC R0.w, R0.x, {0x3f800000(1), 0x00000000(0), 0x00000000(0), 0x00000000(0)}.x;
    unsigned char instr[] = {
        0x11, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1c, 0x9c, 0x00, 0x02, 0x00, 
        0x00, 0xc8, 0x00, 0x00, 0x01, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    };
    
    FragmentInstr fi;
    auto pos = 0;
    
    pos += fragment_dasm_instr(instr + pos, fi);
    auto st = MakeStatement(context, fi, 0);
    auto str = printStatements(st);
    REQUIRE(str == "r[0].w = min((r[0].xxxx), (fconst.c[0].xxxx)).w;c[0].w = r[0].w;");
}

TEST_CASE("shader_rewriter_fragment_single_scalar_argument") {
    ASTContext context;
    // LG2R R0.w, R1.w;
    // EX2R R1.w, R2;
    unsigned char instr[] = {
        0x10, 0x00, 0x1d, 0x00, 0xfe, 0x04, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01, 
        0x10, 0x02, 0x1c, 0x00, 0xc8, 0x08, 0x1c, 0x9d, 0xc8, 0x00, 0x00, 0x01, 0xc8, 0x00, 0x00, 0x01
    };
    
    FragmentInstr fi;
    fragment_dasm_instr(instr, fi);
    auto st = MakeStatement(context, fi, 0);
    auto str = printStatements(st);
    REQUIRE(str == "r[0].w = log2(((r[1].wwww).xxxx)).w;");
    
    fragment_dasm_instr(instr + 16, fi);
    st = MakeStatement(context, fi, 0);
    str = printStatements(st);
    REQUIRE(str == "r[1].w = exp2((r[2].xxxx)).w;");
}

TEST_CASE("shader_rewriter_vertex_33") {
    // MOV o[1].w, c[463].x; BRI(TR) L47;
    ASTContext context;
    unsigned char instr[] = {
        0x40, 0x1f, 0x9c, 0x6c, 0x48, 0x5c, 0xf0, 0x00, 0x01, 0x86, 0xc0, 0x85, 0xe0, 0x40, 0x3f, 0x84,
    };
    std::array<VertexInstr, 2> res;
    vertex_dasm_instr(instr, res);
    auto st = MakeStatement(context, res[1], 0);
    auto str = printStatements(st);
    REQUIRE(str == "nip = 47;break;");
}

TEST_CASE("shader_rewriter_fragment_neg_abs") {
    ASTContext context;
    // 006|006: ADDH H0.z, -|H0.x|, {0x00000000(0), 0x00000000(0), 0x00000000(0), 0x00000000(0)}.y;
    unsigned char instr[] = {
        0x08, 0x80, 0x03, 0x40, 0x01, 0x00, 0x3c, 0x9e, 0xaa, 0x02, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    FragmentInstr fi;
    fragment_dasm_instr(instr, fi);
    auto st = MakeStatement(context, fi, 0);
    auto str = printStatements(st);
    REQUIRE(str == "h[0].z = (((-abs(h[0])).xxxx) + (fconst.c[0].yyyy)).z;");
}
