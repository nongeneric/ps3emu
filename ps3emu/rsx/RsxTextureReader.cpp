#include "RsxTextureReader.h"
#include "ps3emu/BitField.h"
#include "ps3emu/utils.h"

#include <glad/glad.h>
#include <boost/algorithm/string.hpp>
#include <iostream>

const char* masterShaderSource =
R""(

#version 450 core

layout (local_size_x = 32, local_size_y = 32) in;
layout (std430, binding = 0) buffer Buf {
    uint u32[];
};
layout (std430, binding = 2) buffer Info {
    uint i_format;
    uint i_pitch;
    uint i_width;
    uint i_height;
    uint i_offset;
    uint i_isSwizzled;
    uint i_isSignedRemapClamped;
    uint i_isUnsignedRemapBiased;
    ivec4 i_fragmentGamma;
    ivec4 i_fragmentSign;
    ivec4 i_crossbarInput;
    ivec4 i_selectOutput;
};
layout (binding = 1, rgba32f) uniform image2D image_output;

uint endian_reverse(uint i) {
    return ((i & 0xff) << 24)
         | ((i & 0xff00) << 8)
         | ((i & 0xff0000) >> 8)
         | ((i & 0xff000000) >> 24);
}

uint read32(uint byteIndex) {
    byteIndex += i_offset;
    uint wordIndex = byteIndex / 4;
    int subwordIndex = int(byteIndex % 4);
    uint word = endian_reverse(u32[wordIndex]);
    if (subwordIndex == 0)
        return word;
    uint nextWord = endian_reverse(u32[wordIndex + 1]);
    uint left = bitfieldExtract(word, 0, 32 - subwordIndex * 8);
    uint right = bitfieldExtract(nextWord, 32 - subwordIndex * 8, subwordIndex * 8);
    return (left << (32 - subwordIndex * 8)) | right;
}

void main(void) {
    ivec2 src = ivec2(gl_GlobalInvocationID.xy);
    if (src.x >= i_width || src.y >= i_height)
        return;
    uint src_index = src.y * i_pitch + src.x * 4;

    uint val = 0;
    vec4 color = vec4(0);
    
    switch (i_format) {
        case GcmTextureFormat::A8R8G8B8:
            val = read32(src_index);
            color = vec4(
                bitfieldExtract(val, 16, 8),
                bitfieldExtract(val, 8, 8),
                bitfieldExtract(val, 0, 8),
                bitfieldExtract(val, 24, 8)
            ) / 255.;
            break;
    }
    
    ivec2 dest = src; // swizzle
    imageStore(image_output, dest, color);
}

)""
;

union RemapCrossbarSelectForm {
    uint32_t v;
    BitField<0, 16> order;
    BitField<16, 18> outB;
    BitField<18, 20> outG;
    BitField<20, 22> outR;
    BitField<22, 24> outA;
    BitField<24, 26> inB;
    BitField<26, 28> inG;
    BitField<28, 30> inR;
    BitField<30, 32> inA;
};

struct TextureInfoUniform {
    uint32_t format;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint32_t offset;
    uint32_t isSwizzled;
    uint32_t isSignedRemapClamped;
    uint32_t isUnsignedRemapBiased;
    std::array<uint32_t, 4> fragmentGamma; // rgba
    std::array<uint32_t, 4> fragmentSign; // rgba
    std::array<uint32_t, 4> crossbarInput; // rgba
    std::array<uint32_t, 4> selectOutput; // rgba
};

template<typename E>
void patchEnumValues(std::string& text) {
    auto values = enum_traits<E>::values();
    auto names = enum_traits<E>::names();
    auto name = enum_traits<E>::name();
    for (auto i = 0u; i < values.size(); ++i) {
        boost::replace_all(text,
                           ssnprintf("%s::%s", name, names[i]),
                           ssnprintf("%d", values[i]));
    }
}

void RsxTextureReader::init() {
    std::string text = masterShaderSource;
    patchEnumValues<GcmTextureFormat>(text);
    patchEnumValues<TextureRemapInput>(text);
    patchEnumValues<TextureRemapOutput>(text);
    _shader = Shader(GL_COMPUTE_SHADER, text.c_str());
    std::cout << _shader.log();
    _uniformBuffer = GLPersistentCpuBuffer(sizeof(TextureInfoUniform));
}

void RsxTextureReader::loadTexture(RsxTextureInfo const& info, GLuint buffer, GLuint texture) {
    auto uniform = (TextureInfoUniform*)_uniformBuffer.mapped();
    uniform->format = (uint32_t)info.format;
    uniform->pitch = info.pitch;
    uniform->width = info.width;
    uniform->height = info.height;
    uniform->offset = info.offset;
    uniform->isSwizzled = info.lnUn != GcmTextureLnUn::LN;
    uniform->isSignedRemapClamped =
        info.fragmentSignedRemap == CELL_GCM_TEXTURE_SIGNED_REMAP_CLAMPED;
    uniform->isUnsignedRemapBiased =
        info.fragmentUnsignedRemap == CELL_GCM_TEXTURE_UNSIGNED_REMAP_BIASED;
    uniform->fragmentGamma = {
        bool(info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_R),
        bool(info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_G),
        bool(info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_B),
        bool(info.fragmentGamma & CELL_GCM_TEXTURE_GAMMA_A)
    };
    uniform->fragmentSign = {
        info.fragmentRs,
        info.fragmentGs,
        info.fragmentBs,
        info.fragmentAs
    };
    RemapCrossbarSelectForm remapParams { info.fragmentRemapCrossbarSelect };
    uniform->crossbarInput = {
        remapParams.inR.u(),
        remapParams.inG.u(),
        remapParams.inB.u(),
        remapParams.inA.u()
    };
    uniform->selectOutput = {
        remapParams.outR.u(),
        remapParams.outG.u(),
        remapParams.outB.u(),
        remapParams.outA.u()
    };
    
    glUseProgram(_shader.handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _uniformBuffer.handle());
    glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute((info.width + 31) / 32, (info.height + 31) / 32, 1);
}
