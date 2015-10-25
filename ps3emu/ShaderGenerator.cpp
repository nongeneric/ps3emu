#include "ShaderGenerator.h"
#include "ShaderRewriter.h"
#include "utils.h"
#include "constants.h"

using namespace ShaderRewriter;

std::string GenerateFragmentShader(std::vector<uint8_t> const& bytecode) {
    std::vector<std::unique_ptr<Statement>> sts;
    auto pos = 0u;
    int lastReg = 0;
    while (pos < bytecode.size()) {
        FragmentInstr fi;
        pos += fragment_dasm_instr(&bytecode[0] + pos, fi);
        for (auto& st : MakeStatement(fi)) {
            lastReg = std::max(lastReg, GetLastRegisterNum(st.get()));   
            sts.emplace_back(std::move(st));
        }
        if (fi.is_last)
            break;
    }
    
    std::string res;
    auto line = [&](std::string s) { res += s + "\n"; };
    line("#version 450 core");
    line("out vec4 color;");
    line("in vec4 f_COL0;");
    line("in vec4 f_COL1;");
    line("in vec4 f_FOGC;");
    line("in vec4 f_TEX0;");
    line("in vec4 f_TEX1;");
    line("in vec4 f_TEX2;");
    line("in vec4 f_TEX3;");
    line("in vec4 f_TEX4;");
    line("in vec4 f_TEX5;");
    line("in vec4 f_TEX6;");
    line("in vec4 f_TEX7;");
    line("in vec4 f_TEX8;");
    line("in vec4 f_TEX9;");
    line("in vec4 f_SSA;");
    line("void main(void) {");
    line("    vec4 f_WPOS = gl_FragCoord;");
    line("    vec4 c0 = vec4(0);");
    line("    vec4 c1 = vec4(0);");
    for (int i = 0; i <= lastReg; ++i) {
        line(ssnprintf("    vec4 r%d = vec4(0);", i));
        line(ssnprintf("    vec4 h%d = vec4(0);", 2*i));
        line(ssnprintf("    vec4 h%d = vec4(0);", 2*i + 1));
    }
    
    for (auto& st : sts) {
        auto str = PrintStatement(st.get());
        line(str);
    }
    
    line("    color = r0;\n");
    line("}");
    return res;
}

std::string GenerateVertexShader(const uint8_t* bytecode, 
                                 std::array<VertexShaderInputFormat, 16> const& inputs)
{   
    std::string res;
    auto line = [&](std::string s) { res += s + "\n"; };
    line("#version 450 core");
    for (int i = 0; i < 16; ++i) {
        line(ssnprintf("layout (location = %d) in vec4 v%din;", i, i));
    }
    line(ssnprintf("layout(std140, binding = %d) uniform VertexConstants {", 
                   VertexShaderConstantBinding));
    line(ssnprintf("    vec4 c[%d];", VertexShaderConstantCount));
    line("} constants;");
    line("out vec4 f_COL0;");
    line("out vec4 f_COL1;");
    line("out vec4 f_FOGC;");
    line("out vec4 f_TEX0;");
    line("out vec4 f_TEX1;");
    line("out vec4 f_TEX2;");
    line("out vec4 f_TEX3;");
    line("out vec4 f_TEX4;");
    line("out vec4 f_TEX5;");
    line("out vec4 f_TEX6;");
    line("out vec4 f_TEX7;");
    line("out vec4 f_TEX8;");
    line("out vec4 f_TEX9;");
    line("out vec4 f_SSA;");
    line("float reverse(float f) {");
    line("    unsigned int bits = floatBitsToUint(f);");
    line("    unsigned int rev = ((bits & 0xff) << 24)");
    line("                     | ((bits & 0xff00) << 8)");
    line("                     | ((bits & 0xff0000) >> 8)");
    line("                     | ((bits & 0xff000000) >> 24);");
    line("    return uintBitsToFloat(rev);");
    line("}");
    line("vec4 reverse4f(vec4 v) {");
    line("    return vec4 ( reverse(v.x),");
    line("                  reverse(v.y),");
    line("                  reverse(v.z),");
    line("                  reverse(v.w) );");
    line("}");
    line("vec4 reverse3f(vec4 v) {");
    line("    return vec4 ( reverse(v.x),");
    line("                  reverse(v.y),");
    line("                  reverse(v.z),");
    line("                  v.w );");
    line("}");
    line("vec4 reverse2f(vec4 v) {");
    line("    return vec4 ( reverse(v.x),");
    line("                  reverse(v.y),");
    line("                  v.z,");
    line("                  v.w );");
    line("}");
    line("vec4 reverse1f(vec4 v) {");
    line("    return vec4 ( reverse(v.x),");
    line("                  v.y,");
    line("                  v.z,");
    line("                  v.w );");
    line("}");
    line("vec4 reverse4b(vec4 v) {");
    line("    return v;"); // each component representation is in the wrong order,
                           // not the order of components
    line("}");
    line("void main(void) {");
    line("    vec4 v_in[16];");
    line("    vec4 v_out[16];");
    line("    vec4 r[16];");
    for (size_t i = 0; i < inputs.size(); ++i) {
        auto suffix = inputs[i].typeSize == 4 ? "f" : "b";
        line(ssnprintf("    v_in[%d] = reverse%d%s(v%din);", i, inputs[i].rank, suffix, i));
    }
    std::array<VertexInstr, 2> instr;
    bool isLast = false;
    int i = 0;
    while (!isLast) {
        int count = vertex_dasm_instr(bytecode + i, instr);
        for (int n = 0; n < count; ++n) {
            for (auto const& st : MakeStatement(instr[n])) {
                line(PrintStatement(st.get()));
            }
            isLast |= instr[n].is_last;
        }
        i += 16;
    }
    line("    gl_Position = v_out[0];");
    line("    f_COL0 = v_out[1];");
    line("    f_COL1 = v_out[4];");
    line("    f_FOGC = v_out[5];");
    line("    f_TEX0 = v_out[7];");
    line("    f_TEX1 = v_out[8];");
    line("    f_TEX2 = v_out[9];");
    line("    f_TEX3 = v_out[10];");
    line("    f_TEX4 = v_out[11];");
    line("    f_TEX5 = v_out[12];");
    line("    f_TEX6 = v_out[13];");
    line("    f_TEX7 = v_out[14];");
    line("    f_TEX8 = v_out[15];");
    line("    f_TEX9 = v_out[6];");
    line("}");
    
    return res;
}
