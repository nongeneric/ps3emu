#include "ShaderGenerator.h"
#include "ShaderRewriter.h"
#include "../utils.h"
#include "../constants.h"
#include <map>

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
    ASTContext context;
    std::vector<Statement*> sts;
    auto pos = 0u;
    int lastRReg = 0, lastHReg = 0;
    unsigned constIndex = 0;
    while (pos < bytecode.size()) {
        FragmentInstr fi;
        auto len = fragment_dasm_instr(&bytecode[0] + pos, fi);
        for (auto& st : MakeStatement(context, fi, constIndex)) {
            auto [r, h] = GetLastRegisterNum(st);
            lastRReg = std::max(lastRReg, r);
            lastHReg = std::max(lastHReg, h);
            st->address(pos);
            sts.emplace_back(std::move(st));
        }
        pos += len;
        if (len == 32)
            constIndex++;
        if (fi.is_last)
            break;
    }

    sts = RewriteIfStubs(context, sts);

    std::string res;
    auto line = [&](std::string s) { res += s + "\n"; };
    line("#version 450 core");
    line("layout (location = 0) out vec4 color[4];");
    line(sformat("{}in vec4 f_COL0;", isFlatColorShading ? "flat " : ""));
    if (constIndex) {
        line(sformat("layout(std140, binding = {}) uniform FragmentConstants {{",
                    FragmentShaderConstantBinding));
        line(sformat("    vec4 c[{}];", constIndex));
        line("} fconst;");
    }
    line("in vec4 f_COL1;");
    line("in vec4 f_FOGC;");
    line("in vec4 f_TEX0_varying;");
    line("in vec4 f_TEX1_varying;");
    line("in vec4 f_TEX2_varying;");
    line("in vec4 f_TEX3_varying;");
    line("in vec4 f_TEX4_varying;");
    line("in vec4 f_TEX5_varying;");
    line("in vec4 f_TEX6_varying;");
    line("in vec4 f_TEX7_varying;");
    line("in vec4 f_TEX8_varying;");
    line("in vec4 f_TEX9_varying;");
    line("in vec4 f_SSA;");
    line(sformat("layout (std140, binding = {}) uniform FragmentSamplersInfo {{",
                   FragmentShaderSamplesInfoBinding));
    line("    ivec4 flip[4];");
    line("    vec4 xOffset[4];");
    line("    vec4 yOffset[4];");
    line("    vec4 xScale[4];");
    line("    vec4 yScale[4];");
    line("    bvec4 pointSpriteControl[4];");
    line("    bvec4 outputFromH;");
    line("} fragmentSamplersInfo;");
    for (auto i = 0u; i < samplerSizes.size(); ++i) {
        if (samplerSizes[i] == 0)
            continue;
        if (samplerSizes[i] == 6) {
            line(sformat("layout (binding = {}) uniform samplerCube s{};",
                        i + FragmentTextureUnit, i));
            line(sformat("vec4 tex{}(vec4 uvp) {{ return texture(s{}, uvp.xyz); }}", i, i));
        } else {
            line(sformat("layout (binding = {}) uniform sampler{}D s{};",
                        i + FragmentTextureUnit, samplerSizes[i], i));
            line(sformat("vec4 tex{}(vec4 uvp) {{", i));
            line(sformat("    float xScale = fragmentSamplersInfo.xScale{};", flipIndex(i)));
            line(sformat("    float yScale = fragmentSamplersInfo.yScale{};", flipIndex(i)));
            line(sformat("    int isFlip = fragmentSamplersInfo.flip{};", flipIndex(i)));
            line(sformat("    uvp = isFlip == 0 ? uvp : vec4(xScale * uvp.x, 1 - yScale * uvp.y, uvp.zw);"));
            line(sformat("    return texture(s{}, uvp.xy);", i));
            line("}");
            line(sformat("vec4 tex{}Proj(vec4 uvp) {{ return textureProj(s{}, uvp.xyzw); }}", i, i));
        }
    }
    line("void main(void) {");
    line("    bool outputFromH = fragmentSamplersInfo.outputFromH.x;");
    line("    bvec4 t0 = fragmentSamplersInfo.pointSpriteControl[0];");
    line("    bvec4 t1 = fragmentSamplersInfo.pointSpriteControl[1];");
    line("    bvec4 t2 = fragmentSamplersInfo.pointSpriteControl[2];");
    line("    bvec4 t3 = fragmentSamplersInfo.pointSpriteControl[3];");
    line("    vec4 f_TEX0 = (t0[0] && t0[1]) ? vec4(gl_PointCoord, 0, 0) : f_TEX0_varying;");
    line("    vec4 f_TEX1 = (t0[0] && t0[2]) ? vec4(gl_PointCoord, 0, 0) : f_TEX1_varying;");
    line("    vec4 f_TEX2 = (t0[0] && t0[3]) ? vec4(gl_PointCoord, 0, 0) : f_TEX2_varying;");
    line("    vec4 f_TEX3 = (t0[0] && t1[0]) ? vec4(gl_PointCoord, 0, 0) : f_TEX3_varying;");
    line("    vec4 f_TEX4 = (t0[0] && t1[1]) ? vec4(gl_PointCoord, 0, 0) : f_TEX4_varying;");
    line("    vec4 f_TEX5 = (t0[0] && t1[2]) ? vec4(gl_PointCoord, 0, 0) : f_TEX5_varying;");
    line("    vec4 f_TEX6 = (t0[0] && t1[3]) ? vec4(gl_PointCoord, 0, 0) : f_TEX6_varying;");
    line("    vec4 f_TEX7 = (t0[0] && t2[0]) ? vec4(gl_PointCoord, 0, 0) : f_TEX7_varying;");
    line("    vec4 f_TEX8 = (t0[0] && t2[1]) ? vec4(gl_PointCoord, 0, 0) : f_TEX8_varying;");
    line("    vec4 f_TEX9 = (t0[0] && t2[2]) ? vec4(gl_PointCoord, 0, 0) : f_TEX9_varying;");
    line("    vec4 f_WPOS = gl_FragCoord;");
    line("    vec4 c[2];");
    line(sformat("    vec4 r[{}];", lastRReg + 1));
    line(sformat("    vec4 h[{}];", lastHReg + 1));
    for (auto i = 0; i < lastRReg + 1; ++i) {
        line(sformat("    r[{}] = vec4(0, 0, 0, 0);", i));
    }
    for (auto i = 0; i < lastHReg + 1; ++i) {
        line(sformat("    h[{}] = vec4(0, 0, 0, 0);", i));
    }
    for (auto& st : sts) {
        auto str = PrintStatement(st);
        line(str);
    }

    if (isMrt) {
        int regs[] = { 0, 2, 3, 4 };
        for (int i = 0; i < 4 && regs[i] <= lastRReg; ++i) {
            line(sformat("    color[{}] = r[{}];", i, regs[i]));
        }
    } else {
        line("    color[0] = outputFromH ? h[0] : r[0];");
        line("    color[1] = outputFromH ? h[0] : r[0];");
    }
    line("}");
    return res;
}

std::string GenerateVertexShader(const uint8_t* bytecode,
                                 std::array<VertexShaderInputFormat, 16> const& inputs,
                                 std::array<int, 4> const& samplerSizes,
                                 unsigned loadOffset,
                                 std::vector<unsigned>* usedConsts,
                                 bool feedback)
{
    for (auto size : samplerSizes) {
        assert(size != 0); (void)size;
    }

    std::string res;
    auto line = [&](auto&& s) { res += s; res += "\n"; };
    line("#version 450 core");
    line("#extension GL_NV_shader_buffer_load : require");
    line("#extension GL_ARB_gpu_shader_int64 : enable");
    line(sformat("layout(std140, binding = {}) uniform VertexConstants {{",
                   VertexShaderConstantBinding));
    line(sformat("    vec4 c[{}];", VertexShaderConstantCount));
    line("} constants;");
    line(sformat("layout (std140, binding = {}) uniform SamplersInfo {{",
                   VertexShaderSamplesInfoBinding));
    line("    ivec4 wrapMode[4];");
    line("    vec4 borderColor[4];");
    line("    vec4 disabledInputValues[16];");
    line("    uint enabledInputs[16];");
    line("    uint inputBufferBases[16];");
    line("    uint inputBufferStrides[16];");
    line("    uint inputBufferOps[16];");
    line("    uint inputBufferFrequencies[16];");
    line("    intptr_t inputBuffers[16];");
    line("} samplersInfo;");
    // note the uint that is read is LE
    line("uint reverse_uint(uint v) {");
    line("    return ((v & 0xff) << 24)");
    line("         | ((v & 0xff00) << 8)");
    line("         | ((v & 0xff0000) >> 8)");
    line("         | ((v & 0xff000000) >> 24);");
    line("}");
    line("float reverse(float f) {");
    line("    uint bits = floatBitsToUint(f);");
    line("    uint rev = ((bits & 0xff) << 24)");
    line("             | ((bits & 0xff00) << 8)");
    line("             | ((bits & 0xff0000) >> 8)");
    line("             | ((bits & 0xff000000) >> 24);");
    line("    return uintBitsToFloat(rev);");
    line("}");
    line(
        "uint readbyte(uint input_idx, uint offset) {\n"
        "    uint v = *(uint*)(samplersInfo.inputBuffers[input_idx] + (int64_t)offset);\n"
        "    uint mod = offset % 4;\n"
        "    return (v >> (mod * 8)) & 0xff;\n"
        "}");
    line(
        "uint readuint(uint input_idx, uint offset) {\n"
        "    uint le = *(uint*)(samplersInfo.inputBuffers[input_idx] + (int64_t)offset);\n"
        "    return reverse_uint(le);\n"
        "}");
    line(
        "float readfloat(uint input_idx, uint offset) {\n"
        "    return uintBitsToFloat(readuint(input_idx, offset));\n"
        "}");
    line(
        "vec4 read_f16vec(uint rank, uint input_idx, uint offset) {\n"
        "    vec4 vec = vec4( unpackHalf2x16(readuint(input_idx, offset)).yx,\n"
        "                     unpackHalf2x16(readuint(input_idx, offset + 4)).yx);\n"
        "    return vec4(vec.x,\n"
        "                rank > 1 ? vec.y : 0,\n"
        "                rank > 2 ? vec.z : 0,\n"
        "                rank > 3 ? vec.w : 1);\n"
        "}");
    line(
        "vec4 read_f32vec(uint rank, uint input_idx, uint offset) {\n"
        "    float* le = (float*)(samplersInfo.inputBuffers[input_idx] + (int64_t)offset);\n"
        "    return vec4(reverse(le[0]),\n"
        "                rank > 1 ? reverse(le[1]) : 0,\n"
        "                rank > 2 ? reverse(le[2]) : 0,\n"
        "                rank > 3 ? reverse(le[3]) : 1);\n"
        "}");
    line(
        "vec4 read_u8vec(uint rank, uint input_idx, uint offset) {\n"
        "    uint ule = *(uint*)(samplersInfo.inputBuffers[input_idx] + (int64_t)offset);\n"
        "    return vec4(float(ule & 0xff),\n"
        "                rank > 1 ? float((ule >> 8) & 0xff) : 0,\n"
        "                rank > 2 ? float((ule >> 16) & 0xff) : 0,\n"
        "                rank > 3 ? float(ule >> 24) : 255) / 255.0;\n"
        "}");
     line(
        "vec4 read_s16vec(uint rank, uint input_idx, uint offset) {\n"
        "    return vec4((readbyte(input_idx, offset) << 8) | readbyte(input_idx, offset + 1),\n"
        "                rank > 1 ? ((readbyte(input_idx, offset + 2) << 8) | readbyte(input_idx, offset + 3)) : 0,\n"
        "                rank > 2 ? ((readbyte(input_idx, offset + 4) << 8) | readbyte(input_idx, offset + 5)) : 0,\n"
        "                rank > 3 ? ((readbyte(input_idx, offset + 6) << 8) | readbyte(input_idx, offset + 7)) : 65535) / 65535.0;\n"
        "}");
     line(
        "vec4 read_x11y11z10nvec(uint rank, uint input_idx, uint offset) {\n"
        "    int u = int(readuint(input_idx, offset));\n"
        "    return vec4((float(bitfieldExtract(u, 21, 11)) + 0.5) / 1023.5,\n"
        "                rank > 1 ? (float(bitfieldExtract(u, 10, 11)) + 0.5) / 1023.5 : 0,\n"
        "                rank > 2 ? (float(bitfieldExtract(u, 0, 10)) + 0.5) / 511.5 : 0,\n"
        "                rank > 3 ? 1 : 0);\n"
        "}");
     line(
        "vec4 read_s1vec(uint rank, uint input_idx, uint offset) {\n"
        "    float u0 = int((readbyte(input_idx, offset) << 8) | readbyte(input_idx, offset + 1));\n"
        "    float u1 = int((readbyte(input_idx, offset + 2) << 8) | readbyte(input_idx, offset + 3));\n"
        "    float u2 = int((readbyte(input_idx, offset + 4) << 8) | readbyte(input_idx, offset + 5));\n"
        "    float u3 = int((readbyte(input_idx, offset + 6) << 8) | readbyte(input_idx, offset + 7));\n"
        "    return vec4((2.0 * u0 + 1.0) / 65535.0,\n"
        "                rank > 1 ? (2.0 * u1 + 1.0) / 65535.0 : 0,\n"
        "                rank > 2 ? (2.0 * u2 + 1.0) / 65535.0 : 0,\n"
        "                rank > 3 ? (2.0 * u3 + 1.0) / 65535.0 : 0);\n"
        "}");
    line(
        "uint get_input_offset(uint i) {\n"
        "    uint index = gl_VertexID;\n"
        "    uint freq = samplersInfo.inputBufferFrequencies[i];\n"
        "    if (freq != 0) {\n"
        "        if (samplersInfo.inputBufferOps[i] == 0) {\n"
        "            index /= freq;\n"
        "        } else {\n"
        "            index %= freq;\n"
        "        }\n"
        "    }\n"
        "    return samplersInfo.inputBufferBases[i] + samplersInfo.inputBufferStrides[i] * index;\n"
        "}");

    line(sformat("layout (std140, binding = {}) uniform ViewportInfo {{",
                   VertexShaderViewportMatrixBinding));
    line("    mat4 glInverseGcm;");
    line("} viewportInfo;");

    for (auto i = 0u; i < samplerSizes.size(); ++i) {
        line(sformat("layout (binding = {}) uniform sampler{}D s{};",
                       i + VertexTextureUnit, samplerSizes[i], i));
    }

    if (feedback) {
        line(sformat("layout(xfb_buffer = {}) out gl_PerVertex {{", TransformFeedbackBufferBinding));
        line("    layout(xfb_offset = 0) vec4 gl_Position;");
    } else {
        line("out gl_PerVertex {");
        line("    vec4 gl_Position;");
    }
    line("    float gl_PointSize;");
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
    line("");
    line("vec4 reverse4f(vec4 v) {");
    line("    return vec4 ( reverse(v.x),");
    line("                  reverse(v.y),");
    line("                  reverse(v.z),");
    line("                  reverse(v.w) );");
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
        line(sformat("vec4 txl{}(vec4 uv) {{", s, size));
        line(sformat("    ivec{} size = textureSize(s{}, 0);", size, s));
        line(sformat("    ivec4 type = samplersInfo.wrapMode[{}];", s));
        line(sformat("    ivec4 iuv = ivec4("));
        for (int i = 0; i < size; ++i) {
            if (i != 0)
                line(",");
            auto c = i == 0 ? "x" : i == 1 ? "y" : "z";
            line(sformat("        wrap(uv.{}, type[{}], size.{})", c, i, c));
        }
        for (int i = 0; i < 4 - size; ++i) {
            line("        , 0");
        }
        line(");");
        auto sw = size == 1 ? "x" : size == 2 ? "xy" : "xyz";
        line(sformat("    vec4 texel = texture(s{}, (iuv.{} + 0.5) / size, uv.w);", s, sw));
        line(sformat("    if (iuv.x == -1 || iuv.y == -1 || iuv.z == -1)"));
        line(sformat("        texel = samplersInfo.borderColor[{}].abgr;", s));
        line(sformat("    return texel;"));
        line("};");
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
    for (int i = 0; i < 32; ++i) {
        line(sformat("    r[{}] = vec4(0,0,0,1);", i));
    }
    for (int i = 0; i < 16; ++i) {
        line(sformat("    v_out[{}] = vec4(0,0,0,1);", i));
    }
    for (size_t i = 0; i < inputs.size(); ++i) {
        auto type = to_string(inputs[i].type);
        auto rank = inputs[i].type == VertexInputType::x11y11z10n ? 4 : inputs[i].rank;
        line(sformat(
                "if (samplersInfo.enabledInputs[{}] == 1) {{\n"
                "    v_in[{}] = read_{}vec({}, {}, get_input_offset({}));\n"
                "}} else {{ v_in[{}] = samplersInfo.disabledInputValues[{}]; }}",
                i, i, type, rank, i, i, i, i));
    }
    std::vector<Statement*> sts;
    std::array<VertexInstr, 2> instr;
    bool isLast = false;
    int num = 0;
    UsedConstsVisitor usedConstsVisitor;

    ASTContext context;
    while (!isLast) {
        int count = vertex_dasm_instr(bytecode, instr);
        for (int n = 0; n < count; ++n) {
            for (auto& st : MakeStatement(context, instr[n], num)) {
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

    sts = RewriteBranches(context, sts);

    for (auto& st : sts) {
        line(PrintStatement(st));
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
    line("    gl_PointSize = v_out[6].x;");
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
    std::map<unsigned, int> elses;
    std::map<unsigned, int> endifs;
    do {
        auto len = fragment_dasm_instr(instr, fi);
        static std::string line;
        line.clear();
        fragment_dasm(fi, line);
        for (auto i = 0; i < endifs[pos]; ++i) {
            res += "         ENDIF\n";
        }
        if (elses[pos]) {
            res += "         ELSE\n";
        }
        res += sformat("{:03}|{:03x}: {}\n", pos / 16, pos / 16, line);
        if (fi.opcode.instr == fragment_op_t::IFE) {
            elses[fi.elseLabel * 16]++;
            endifs[fi.endifLabel * 16]++;
        }
        instr += len;
        pos += len;
    } while(!fi.is_last);
    return res;
}

std::string PrintMask(int mask) {
    return sformat("{}{}{}{}",
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
                res += sformat("{:03}: {}\n", num, vertex_dasm(vi));
            } else {
                res += sformat("     {}\n", vertex_dasm(vi));
            }
            if (verbose) {
                res += "{\n";
                res += sformat("    is_last: {}\n", decoded.is_last);
                res += sformat("    is_complex_offset: {}\n", decoded.is_complex_offset);
                res += sformat("    output_reg_num: {}\n", decoded.output_reg_num);
                res += sformat("    dest_reg_num: {}\n", decoded.dest_reg_num);
                res += sformat("    addr_data_reg_num: {}\n", decoded.addr_data_reg_num);
                res += sformat("    mask1: {}\n", PrintMask(decoded.mask1));
                res += sformat("    mask2: {}\n", PrintMask(decoded.mask2));
                res += sformat("    v_displ: {:x}\n", decoded.v_displ);
                res += sformat("    input_v_num: {}\n", decoded.input_v_num);
                res += sformat("    opcode1: {}\n", decoded.opcode1);
                res += sformat("    opcode2: {}\n", decoded.opcode2);
                res += sformat("    disp_component: {:x}\n", decoded.disp_component);
                res += sformat("    cond_swizzle: {}\n", print_swizzle(decoded.cond_swizzle, false));
                res += sformat("    cond_relation: {}\n", print_cond(decoded.cond_relation));
                res += sformat("    flag2: {}\n", decoded.flag2);
                res += sformat("    has_cond: {}\n", decoded.has_cond);
                res += sformat("    is_addr_reg: {}\n", decoded.is_addr_reg);
                res += sformat("    is_cond_c1: {}\n", decoded.is_cond_c1);
                res += sformat("    is_sat: {}\n", decoded.is_sat);
                res += sformat("    v_index_has_displ: {}\n", decoded.v_index_has_displ);
                res += sformat("    output_has_complex_offset: {}\n", decoded.output_has_complex_offset);
                res += sformat("    sets_c: {}\n", decoded.sets_c);
                res += sformat("    mask_selector: {}\n", decoded.mask_selector);
                res += sformat("    label: {}\n\n", decoded.label);
                for (int i = 0; i < 3; ++i) {
                    auto& arg = decoded.args[i];
                    res += sformat("arg {}\n", i);
                    res += sformat("    is_neg: {}\n", arg.is_neg);
                    res += sformat("    is_abs: {}\n", arg.is_abs);
                    res += sformat("    reg_num: {}\n", arg.reg_num);
                    res += sformat("    reg_type: {}\n", arg.reg_type);
                    res += sformat("    swizzle: {}\n", print_swizzle(arg.swizzle, false));
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
        res += sformat("{:03}: {}\n", pos / 16, hex);
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
            res += sformat("{:03}: {}\n", (bytecode - initial) / 16, hex);
            isLast |= instr[n].is_last;
        }
        bytecode += 16;
    }
    return res;
}
