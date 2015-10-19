#include "ShaderGenerator.h"
#include "ShaderRewriter.h"
#include "utils.h"

using namespace ShaderRewriter;

std::string GenerateShader(std::vector<uint8_t> const& bytecode) {
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
    line("in vec4 attr_TEX0;");
    line("in vec4 attr_COL0;");
    line("void main(void) {");
    
    line("    vec4 attr_WPOS = gl_FragCoord;");
    line("    vec4 c0 = vec4(0);");
    line("    vec4 c1 = vec4(0);");
    for (int i = 0; i <= lastReg; ++i) {
        line(ssnprintf("    vec4 r%d = vec4(0);", i));
    }
    
    for (auto& st : sts) {
        auto str = PrintStatement(st.get());
        line(str);
    }
    
    line("    color = r0;\n");
    line("}");
    return res;
}
