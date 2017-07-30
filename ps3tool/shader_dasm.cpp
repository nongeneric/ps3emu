#include "ps3tool.h"
#include "ps3emu/shaders/shader_dasm.h"
#include "ps3emu/shaders/ShaderGenerator.h"
#include "ps3emu/fileutils.h"

#include <stdio.h>
#include <cstdint>
#include <vector>
#include <boost/endian/arithmetic.hpp>

using namespace boost::endian;

struct ShaderHeader {
    uint8_t padding[28];
    big_uint32_t codeOffset;
};

void HandleShaderDasm(ShaderDasmCommand const& command) {
    auto vec = read_all_bytes(command.binary);
    uint8_t* code;
    if (command.naked) {
        code = &vec[0];
    } else {
        auto header = (ShaderHeader*)&vec[0];
        code = &vec[0] + header->codeOffset;
    }

    if (command.vertex) {
        auto bytecode = PrintVertexBytecode(code);
        printf("%s\n", bytecode.c_str());
        auto dasm = PrintVertexProgram(code, false);
        printf("%s\n", dasm.c_str());
        auto glsl = GenerateVertexShader(code, {}, {}, 0);
        printf("%s", glsl.c_str());
    } else {
        auto bytecode = PrintFragmentBytecode(code);
        printf("%s\n", bytecode.c_str());
        auto asmText = PrintFragmentProgram(code);
        printf("%s", asmText.c_str());
    }
}
