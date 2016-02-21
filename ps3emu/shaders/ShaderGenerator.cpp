#include "ShaderGenerator.h"
#include "ShaderRewriter.h"
#include "../utils.h"
#include "../constants.h"

using namespace ShaderRewriter;

const char* flipIndex(int n) {
    switch (n) {
        case 0: return "[0].x";
        case 1: return "[0].y";
        case 2: return "[0].z";
        case 3: return "[0].w";
        case 4: return "[1].x";
        case 5: return "[1].y";
        case 6: return "[1].z";
        case 7: return "[1].w";
        case 8: return "[2].x";
        case 9: return "[2].y";
        case 10: return "[2].z";
        case 11: return "[2].w";
        case 12: return "[3].x";
        case 13: return "[3].y";
        case 14: return "[3].z";
        case 15: return "[3].w";
    }
    throw std::runtime_error("");
}

std::string GenerateFragmentShader(std::vector<uint8_t> const& bytecode,
                                   std::array<int, 16> const& samplerSizes,
                                   bool isFlatColorShading) {
    std::vector<std::unique_ptr<Statement>> sts;
    auto pos = 0u;
    int lastReg = 0;
    unsigned constIndex = 0;
    while (pos < bytecode.size()) {
        FragmentInstr fi;
        auto len = fragment_dasm_instr(&bytecode[0] + pos, fi);
        pos += len;
        for (auto& st : MakeStatement(fi, constIndex)) {
            lastReg = std::max(lastReg, GetLastRegisterNum(st.get()));   
            sts.emplace_back(std::move(st));
        }
        if (len == 32)
            constIndex++;
        if (fi.is_last)
            break;
    }
    
    std::string res;
    auto line = [&](std::string s) { res += s + "\n"; };
    line("#version 450 core");
    line("layout (location = 0) out vec4 color[4];");
    line(ssnprintf("%sin vec4 f_COL0;", isFlatColorShading ? "flat " : ""));
    if (constIndex) {
        line(ssnprintf("layout(std140, binding = %d) uniform FragmentConstants {",
                    FragmentShaderConstantBinding));
        line(ssnprintf("    vec4 c[%d];", constIndex));
        line("} fconst;");
    }
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
    line(ssnprintf("layout (std140, binding = %d) uniform FragmentSamplersInfo {",
                   FragmentShaderSamplesInfoBinding));
    line("    ivec4 flip[4];");
    line("} fragmentSamplersInfo;");
    for (auto i = 0u; i < samplerSizes.size(); ++i) {
        line(ssnprintf("layout (binding = %d) uniform sampler%dD s%d;",
                       i + FragmentTextureUnit, samplerSizes[i], i));
        line(ssnprintf("vec4 tex%d(vec4 uvp) {", i));
        line(ssnprintf("    uvp = fragmentSamplersInfo.flip%s == 0 ? uvp : vec4(uvp.x, 1 - uvp.y, uvp.zw);",
                       flipIndex(i)));
        line(ssnprintf("    return texture(s%d, uvp.xy);", i));
        line("}");
    }
    line("void main(void) {");
    line("    vec4 f_WPOS = gl_FragCoord;");
    line("    vec4 c[2];");
    line(ssnprintf("    vec4 r[%d];", lastReg + 1));
    line(ssnprintf("    vec4 h[%d];", 2 * (lastReg + 1)));
    for (auto& st : sts) {
        auto str = PrintStatement(st.get());
        line(str);
    }
    // MRT
    int regs[] = { 0, 2, 3, 4 };
    for (int i = 0; i < 4 && regs[i] <= lastReg; ++i) {
        line(ssnprintf("    color[%d] = r[%d];", i, regs[i]));
    }
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
    line(ssnprintf("layout (location = 0) in vec4 v_in_be[16];"));
    line(ssnprintf("layout(std140, binding = %d) uniform VertexConstants {", 
                   VertexShaderConstantBinding));
    line(ssnprintf("    vec4 c[%d];", VertexShaderConstantCount));
    line("} constants;");
    line(ssnprintf("layout (std140, binding = %d) uniform SamplersInfo {",
                   VertexShaderSamplesInfoBinding));
    line("    ivec4 wrapMode[4];");
    line("    vec4 borderColor[4];");
    line("} samplersInfo;");
    for (auto i = 0u; i < samplerSizes.size(); ++i) {
        line(ssnprintf("layout (binding = %d) uniform sampler%dD s%d;",
                       i + VertexTextureUnit, samplerSizes[i], i));
    }
    line("out gl_PerVertex {");
    line("    vec4 gl_Position;");
    //line("    float gl_PointSize;");
    //line("    float gl_ClipDistance[];");
    line("};");
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
    line("int wrap(float x, int type, float size) {");
    line("    int i = int(floor(x * size));");
    line("    int is = int(size);");
    line("    bool first = -is < i && i < is;");
    line("    if (type == 6) type = first ? 2 : 63;");
    line("    if (type == 7) type = first ? 2 : 4;");
    line("    if (type == 8) type = first ? 2 : 63;");
    line("    switch (type) {");
    line("        case 1: {");
    line("            i = i < 0 ? is - 1 - ((abs(i) - 1) % is) : i;");
    line("            i = i % is;");
    line("            break;");
    line("        }");
    line("        case 2: {");
    line("            if (i < 0) i = abs(i) - 1;");
    line("            if (i % (2 * is) >= is) {");
    line("                i = is - (i % is) - 1;");
    line("            } else {");
    line("                i = i % is;");
    line("            }");
    line("            break;");
    line("        }");
    line("        case 4: {");
    line("            i = i < 0 || i >= is ? -1 : i;");
    line("            break;");
    line("        }");
    line("        case 5:");
    line("        case 3: {");
    line("            i = i < 0 ? 0 : i >= is ? is - 1 : i;");
    line("            break;");
    line("        }");
    line("        case 63: {");
    line("            i = i < 0 || i >= is ? is - 1 : i;");
    line("            break;");
    line("        }");
    line("    }");
    line("    return i;");
    line("}");
    auto txl = [&](int size, int s) {
        line(ssnprintf("vec4 txl%d(vec4 uv) {", s, size));
        line(ssnprintf("    ivec%d size = textureSize(s%d, 0);", size, s));
        line(ssnprintf("    ivec4 type = samplersInfo.wrapMode[%d];", s));
        line(ssnprintf("    ivec4 iuv = ivec4("));
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
        line(ssnprintf("    vec4 texel = reverse4f(texture(s%d, (iuv.%s + 0.5) / size, uv.w));", s, sw));
        line(ssnprintf("    if (iuv.x == -1 || iuv.y == -1 || iuv.z == -1)"));
        line(ssnprintf("        texel = samplersInfo.borderColor[%d].abgr;", s));
        line(ssnprintf("    return texel;"));
        line(ssnprintf("};"));
    };
    for (auto i = 0u; i < std::min(size_t(2), samplerSizes.size()); ++i) {
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
    for (int i = 0; i < 16; ++i) {
        line(ssnprintf("    v_out[%d] = vec4(0,0,0,0);", i));
    }
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (!inputs[i].enabled)
            continue;
        auto suffix = inputs[i].typeSize == 4 ? "f" : "b";
        line(ssnprintf("    v_in[%d] = reverse%d%s(v_in_be[%d]);", i, inputs[i].rank, suffix, i));
    }
    std::vector<std::unique_ptr<Statement>> sts;
    std::array<VertexInstr, 2> instr;
    bool isLast = false;
    while (!isLast) {
        int count = vertex_dasm_instr(bytecode, instr);
        for (int n = 0; n < count; ++n) {
            for (auto& st : MakeStatement(instr[n], loadOffset)) {
                sts.emplace_back(std::move(st));
            }
            isLast |= instr[n].is_last;
        }
        loadOffset++;
        bytecode += 16;
    }
    
    sts = RewriteBranches(std::move(sts));
    
    for (auto& st : sts) {
        line(PrintStatement(st.get()));
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

uint32_t CalcVertexBytecodeSize(const uint8_t* bytecode) {
    uint32_t size = 0;
    std::array<VertexInstr, 2> instr;
    while (size < 512 * 16) {
        int count = vertex_dasm_instr(bytecode, instr);
        size += 16;
        bytecode += 16;
        for (int n = 0; n < count; ++n) {
            if (instr[n].is_last)
                return size;
        }
    }
    throw std::runtime_error("could not find the last instruction");
}

std::string PrintFragmentProgram(const uint8_t* instr) {
    std::string res;
    FragmentInstr fi;
    do {
        auto len = fragment_dasm_instr(instr, fi);
        static std::string line;
        fragment_dasm(fi, line);
        instr += len;
        res += line + "\n";
        
    } while(!fi.is_last);
    return res;
}

std::string PrintVertexProgram(const uint8_t* instr) {
    std::string res;
    std::array<VertexInstr, 2> vi;
    bool isLast = false;
    while (!isLast) {
        int count = vertex_dasm_instr(instr, vi);
        for (int n = 0; n < count; ++n) {
            res += vertex_dasm(vi[n]) + "\n";
            isLast |= vi[n].is_last;
        }
        instr += 16;
    }
    return res;
}
