#include "ShaderGenerator.h"
#include "ShaderRewriter.h"
#include "../utils.h"
#include "../constants.h"

using namespace ShaderRewriter;

std::string GenerateFragmentShader(std::vector<uint8_t> const& bytecode, bool isFlatColorShading) {
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
    line(ssnprintf("%sin vec4 f_COL0;", isFlatColorShading ? "flat " : ""));
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
    line("    vec4 c[2];");
    line(ssnprintf("    vec4 r[%d];", lastReg + 1));
    line(ssnprintf("    vec4 h[%d];", 2 * (lastReg + 1)));
    for (auto& st : sts) {
        auto str = PrintStatement(st.get());
        line(str);
    }
    
    line("    color = r[0];\n");
    line("}");
    return res;
}

std::string GenerateVertexShader(const uint8_t* bytecode, 
                                 std::array<VertexShaderInputFormat, 16> const& inputs,
                                 std::array<int, 4> const& samplerSizes,
                                 unsigned loadOffset)
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
    line(ssnprintf("layout(std140, binding = %d) uniform SamplersInfo {",
                   VertexShaderSamplesInfoBinding));
    line("    ivec3 wrapMode;");
    line("    vec4 borderColor;");
    line("} samplersInfo[4];");
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
    for (auto i = 0u; i < samplerSizes.size(); ++i) {
        line(ssnprintf("layout (binding = %d) uniform sampler%dD s%d;", i, samplerSizes[i], i));
    }
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
    line("float wrap(float x, int type, float size) {");
    line("    bool first = -1 <= x && x <= 1;");
    line("    if (type == 6) type = first ? 2 : 3;");
    line("    if (type == 7) type = first ? 2 : 4;"); // 4 isn't handled here
    line("    if (type == 8) type = first ? 2 : 5;");
    line("    switch (type) {");
    line("        case 1: {");
    line("            x = mod(x, 1.0);");
    line("            break;");
    line("        }");
    line("        case 2: {");
    line("            float temp = floor(x);");
    line("            if (mod(temp, 2.0) != 0) {");
    line("                x = 1.0 - (mod(x, 1.0));");
    line("            } else {");
    line("                x = mod(x, 1.0);");
    line("            }");
    line("            break;");
    line("        }");
    line("        case 5:");
    line("        case 3: {");
    line("            float h = 1.0 / (2.0 * size);");
    line("            x = x < 0 ? h : x > 1 ? 1 - h : x;");
    line("            break;");
    line("        }");
    line("    }");
    line("    return x;");
    line("}");
    line("vec4 borderWrap(float x, vec4 color, vec4 border) {");
    line("    if (0 <= x && x <= 1) return color;");
    line("    return border;");
    line("}");
    auto txl = [&](int size, int s) {
        line(ssnprintf("vec4 txl%d(vec4 uv) {", s, size));
        line(ssnprintf("    ivec%d size = textureSize(s%d, 0);", size, s));
        line(ssnprintf("    ivec3 type = samplersInfo[%d].wrapMode;", s));
        line(ssnprintf("    uv = vec4("));
        for (int i = 0; i < size; ++i) {
            if (i != 0)
                line(",");
            auto c = i == 0 ? "x" : i == 1 ? "y" : "z";
            line(ssnprintf("        wrap(uv.%s, type[%d], size.%s)", c, i, c));
        }
        for (int i = 0; i < 4 - size; ++i) {
            line("        , 0");
        }
        line(");");
        auto sw = size == 1 ? "x" : size == 2 ? "xy" : "xyz";
        line(ssnprintf("    vec4 texel = reverse4f(texture(s%d, uv.%s));", s, sw));
        line(ssnprintf("    if (type[0] == 4 || (uv.x < -1 || uv.x > 1 && type[0] == 7))"));
        line(ssnprintf("        texel = borderWrap(uv.x, texel, samplersInfo[%d].borderColor);", s));
        if (size > 1) {
            line(ssnprintf("    if (type[1] == 4 || (uv.y < -1 || uv.y > 1 && type[1] == 7))"));
            line(ssnprintf("        texel = borderWrap(uv.y, texel, samplersInfo[%d].borderColor);", s));   
        }
        if (size > 2) {
            line(ssnprintf("    if (type[2] == 4 || (uv.z < -1 || uv.z > 1 && type[2] == 7))"));
            line(ssnprintf("        texel = borderWrap(uv.z, texel, samplersInfo[%d].borderColor);", s));   
        }
        line(ssnprintf("    return texel;"));
        line(ssnprintf("};"));
    };
    for (auto i = 0u; i < samplerSizes.size(); ++i) {
        txl(samplerSizes[i], i);   
    }
    line("void main(void) {");
    line("    vec4 v_in[16];");
    line("    vec4 v_out[16];");
    line("    ivec4 a[2];");
    line("    vec4 r[32];");
    line("    vec4 stack[8];");
    line("    int stack_ptr = 8;");
    line("    vec4 c[2];");
    line("    bool b[32];");
    line("    vec4 void_var;");
    for (size_t i = 0; i < inputs.size(); ++i) {
        auto suffix = inputs[i].typeSize == 4 ? "f" : "b";
        line(ssnprintf("    v_in[%d] = reverse%d%s(v%din);", i, inputs[i].rank, suffix, i));
    }
    std::array<VertexInstr, 2> instr;
    bool isLast = false;
    while (!isLast) {
        int count = vertex_dasm_instr(bytecode, instr);
        for (int n = 0; n < count; ++n) {
            for (auto const& st : MakeStatement(instr[n], loadOffset)) {
                line(PrintStatement(st.get()));
            }
            isLast |= instr[n].is_last;
        }
        loadOffset++;
        bytecode += 16;
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
