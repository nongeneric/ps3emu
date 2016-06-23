#include "ps3tool.h"
#include "ps3emu/shaders/shader_dasm.h"
#include "ps3emu/shaders/ShaderGenerator.h"

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
    auto f = fopen(command.binary.c_str(), "r");
    if (!f) {
        throw std::runtime_error("can't open shader file\n");
    }
    fseek(f, 0, SEEK_END);

    std::vector<uint8_t> vec(ftell(f));
    fseek(f, 0, SEEK_SET);
    fread(&vec[0], vec.size(), 1, f);
    fclose(f);

    auto header = (ShaderHeader*)&vec[0];
    auto code = &vec[0] + header->codeOffset;

    auto dasm = PrintVertexProgram(code, false);
    printf("%s\n\n", dasm.c_str());
    auto glsl = GenerateVertexShader(code, {}, {}, 0);
    printf("%s", glsl.c_str());
}
