#include "../ps3emu/shaders/shader_dasm.h"
#include "../ps3emu/shaders/ShaderGenerator.h"

#include <stdio.h>
#include <cstdint>
#include <vector>
#include <boost/endian/arithmetic.hpp>

using namespace boost::endian;

struct ShaderHeader {
    uint8_t padding[28];
    big_uint32_t codeOffset;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("specify binary shader path\n");
        return 1;
    }
    
    auto shaderPath = argv[1];
    auto verbose = argc > 2 && !strcmp(argv[2], "-v");
    auto f = fopen(shaderPath, "r");
    if (!f) {
        printf("can't open shader file\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    
    std::vector<uint8_t> vec(ftell(f));
    fseek(f, 0, SEEK_SET);
    fread(&vec[0], vec.size(), 1, f);
    fclose(f);
    
    auto header = (ShaderHeader*)&vec[0];
    auto code = &vec[0] + header->codeOffset;
    
    auto dasm = PrintVertexProgram(code, verbose);
    printf("%s\n\n", dasm.c_str());
    auto glsl = GenerateVertexShader(code, {}, {}, 0);
    printf("%s", glsl.c_str());
}
