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
                                   bool isFlatColorShading,
                                   bool isMrt) {
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
        if (samplerSizes[i] == 0)
            continue;
        if (samplerSizes[i] == 6) {
            line(ssnprintf("layout (binding = %d) uniform samplerCube s%d;",
                        i + FragmentTextureUnit, i));
            line(ssnprintf("vec4 tex%d(vec4 uvp) { return texture(s%d, uvp.xyz); }", i, i));
        } else {
            line(ssnprintf("layout (binding = %d) uniform sampler%dD s%d;",
                        i + FragmentTextureUnit, samplerSizes[i], i));
            line(ssnprintf("vec4 tex%d(vec4 uvp) {", i));
            line(ssnprintf("    uvp = fragmentSamplersInfo.flip%s == 0 ? uvp : vec4(uvp.x, 1 - uvp.y, uvp.zw);",
                        flipIndex(i)));
            line(ssnprintf("    return texture(s%d, uvp.xy);", i));
            line("}");
            line(ssnprintf("vec4 tex%dProj(vec4 uvp) { return textureProj(s%d, uvp.xyzw); }", i, i));
        }
    }
    line("void main(void) {");
    line("    vec4 f_WPOS = gl_FragCoord;");
    line("    vec4 c[2];");
    line(ssnprintf("    vec4 r[%d];", lastReg + 1));
    line(ssnprintf("    vec4 h[%d];", 2 * (lastReg + 1)));
    for (auto i = 0; i < lastReg + 1; ++i) {
        line(ssnprintf("    r[%d] = vec4(0, 0, 0, 0);", i));
    }
    for (auto i = 0; i < 2 * (lastReg + 1); ++i) {
        line(ssnprintf("    h[%d] = vec4(0, 0, 0, 0);", i));
    }
    for (auto& st : sts) {
        auto str = PrintStatement(st.get());
        line(str);
    }
    
    if (isMrt) {
        int regs[] = { 0, 2, 3, 4 };
        for (int i = 0; i < 4 && regs[i] <= lastReg; ++i) {
            line(ssnprintf("    color[%d] = r[%d];", i, regs[i]));
        }
    } else {
        line("    color[0] = r[0];");
        line("    color[1] = r[0];");
    }
    line("}");
    return res;
}

std::string GenerateVertexShader(const uint8_t* bytecode, 
                                 std::array<VertexShaderInputFormat, 16> const& inputs,
                                 std::array<int, 4> const& samplerSizes,
                                 unsigned loadOffset,
                                 std::vector<unsigned>* usedConsts)
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
    
    line(ssnprintf("layout (std140, binding = %d) uniform ViewportInfo {",
                   VertexShaderViewportMatrixBinding));
    line("    mat4 glInverseGcm;");
    line("} viewportInfo;");

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
    line("    uint bits = floatBitsToUint(f);");
    line("    uint rev = ((bits & 0xff) << 24)");
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
    line("    int nip;");
    for (int i = 0; i < 16; ++i) {
        line(ssnprintf("    v_out[%d] = vec4(0,0,0,1);", i));
    }
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i].enabled) {
            auto suffix = inputs[i].typeSize == 4 ? "f" : "b";
            line(ssnprintf("    v_in[%d] = reverse%d%s(v_in_be[%d]);", i, inputs[i].rank, suffix, i));
        } else {
            line(ssnprintf("    v_in[%d] = v_in_be[%d];", i, i));
        }
    }
    std::vector<std::unique_ptr<Statement>> sts;
    std::array<VertexInstr, 2> instr;
    bool isLast = false;
    int num = 0;
    UsedConstsVisitor usedConstsVisitor;
    while (!isLast) {
        int count = vertex_dasm_instr(bytecode, instr);
        for (int n = 0; n < count; ++n) {
            for (auto& st : MakeStatement(instr[n], num)) {
                if (usedConsts) {
                    st->accept(&usedConstsVisitor);
                }
                sts.emplace_back(std::move(st));
            }
            isLast |= instr[n].is_last;
        }
        num++;
        bytecode += 16;
    }
    
    if (usedConsts) {
        auto const& consts = usedConstsVisitor.consts();
        std::copy(begin(consts), end(consts), std::back_inserter(*usedConsts));
    }
    
    sts = RewriteBranches(std::move(sts));
    
    for (auto& st : sts) {
        line(PrintStatement(st.get()));
    }

    line("    gl_Position = viewportInfo.glInverseGcm * v_out[0];");
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
    unsigned pos = 0;
    FragmentInstr fi;
    do {
        auto len = fragment_dasm_instr(instr, fi);
        static std::string line;
        line.clear();
        fragment_dasm(fi, line);
        res += ssnprintf("%03d: %s\n", pos / 16, line);
        instr += len;
        pos += len;
        
    } while(!fi.is_last);
    return res;
}

std::string PrintMask(int mask) {
    return ssnprintf("%d%d%d%d", 
                     (mask & 8) != 0, 
                     (mask & 4) != 0, 
                     (mask & 2) != 0, 
                     (mask & 1) != 0);
}

std::string PrintVertexProgram(const uint8_t* instr, bool verbose) {
    int num = 0;
    std::string res;
    std::array<VertexInstr, 2> vis;
    bool isLast = false;
    while (!isLast) {
        vertex_decoded_instr_t decoded;
        int count = vertex_dasm_instr(instr, vis, &decoded);
        for (int n = 0; n < count; ++n) {
            auto& vi = vis[n];
            if (n == 0) {
                res += ssnprintf("%03d: %s\n", num, vertex_dasm(vi));
            } else {
                res += ssnprintf("     %s\n", vertex_dasm(vi));
            }
            if (verbose) {
                res += "{\n";
                res += ssnprintf("    is_last: %d\n", decoded.is_last);
                res += ssnprintf("    is_complex_offset: %d\n", decoded.is_complex_offset);
                res += ssnprintf("    output_reg_num: %d\n", decoded.output_reg_num);
                res += ssnprintf("    dest_reg_num: %d\n", decoded.dest_reg_num);
                res += ssnprintf("    addr_data_reg_num: %d\n", decoded.addr_data_reg_num);
                res += ssnprintf("    mask1: %s\n", PrintMask(decoded.mask1));
                res += ssnprintf("    mask2: %s\n", PrintMask(decoded.mask2));
                res += ssnprintf("    v_displ: %x\n", decoded.v_displ);
                res += ssnprintf("    input_v_num: %d\n", decoded.input_v_num);
                res += ssnprintf("    opcode1: %d\n", decoded.opcode1);
                res += ssnprintf("    opcode2: %d\n", decoded.opcode2);
                res += ssnprintf("    disp_component: %x\n", decoded.disp_component);
                res += ssnprintf("    cond_swizzle: %s\n", print_swizzle(decoded.cond_swizzle, false));
                res += ssnprintf("    cond_relation: %s\n", print_cond(decoded.cond_relation));
                res += ssnprintf("    flag2: %d\n", decoded.flag2);
                res += ssnprintf("    has_cond: %d\n", decoded.has_cond);
                res += ssnprintf("    is_addr_reg: %d\n", decoded.is_addr_reg);
                res += ssnprintf("    is_cond_c1: %d\n", decoded.is_cond_c1);
                res += ssnprintf("    is_sat: %d\n", decoded.is_sat);
                res += ssnprintf("    v_index_has_displ: %d\n", decoded.v_index_has_displ);
                res += ssnprintf("    output_has_complex_offset: %d\n", decoded.output_has_complex_offset);
                res += ssnprintf("    sets_c: %d\n", decoded.sets_c);
                res += ssnprintf("    mask_selector: %d\n", decoded.mask_selector);
                res += ssnprintf("    label: %d\n\n", decoded.label);
                for (int i = 0; i < 3; ++i) {
                    auto& arg = decoded.args[i];
                    res += ssnprintf("arg %d\n", i);
                    res += ssnprintf("    is_neg: %d\n", arg.is_neg);
                    res += ssnprintf("    is_abs: %d\n", arg.is_abs);
                    res += ssnprintf("    reg_num: %d\n", arg.reg_num);
                    res += ssnprintf("    reg_type: %d\n", arg.reg_type);
                    res += ssnprintf("    swizzle: %s\n", print_swizzle(arg.swizzle, false));
                }
                res += "}\n\n";
            }
            isLast |= vi.is_last;
        }
        instr += 16;
        num++;
    }
    return res;
}

std::string PrintFragmentBytecode(const uint8_t* bytecode) {
    std::string res;
    unsigned pos = 0;
    FragmentInstr fi;
    do {
        auto len = fragment_dasm_instr(&bytecode[pos], fi);
        auto hex = print_hex(&bytecode[pos], len, true);
        res += ssnprintf("%03d: %s\n", pos / 16, hex);
        pos += len;
        
    } while(!fi.is_last);
    return res;
}

std::string PrintVertexBytecode(const uint8_t* bytecode) {
    bool isLast = false;
    std::array<VertexInstr, 2> instr;
    std::string res;
    const uint8_t* initial = bytecode;
    while (!isLast) {
        int count = vertex_dasm_instr(bytecode, instr);
        for (int n = 0; n < count; ++n) {
            auto hex = print_hex(bytecode, 16, true);
            res += ssnprintf("%03d: %s\n", (bytecode - initial) / 16, hex);
            isLast |= instr[n].is_last;
        }
        bytecode += 16;
    }
    return res;
}