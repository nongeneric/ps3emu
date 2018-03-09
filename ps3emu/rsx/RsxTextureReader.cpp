#include "GcmConstants.h"
#include "RsxTextureReader.h"
#include "ps3emu/BitField.h"
#include "ps3emu/utils.h"
#include "ps3emu/log.h"
#include "ps3emu/ImageUtils.h"

#include <glad/glad.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <tuple>
#include <stdio.h>

const char* commonShaderSource =
R""(

#version 450 core
#extension GL_ARB_bindless_texture : require

layout (local_size_x = 32, local_size_y = 32) in;
layout (std430, binding = 0) buffer Buf {
    uint u32[];
};
layout (std430, binding = 2) buffer Info {
    uvec4 i_fragmentGamma;
    uvec4 i_fragmentSign;
    uvec4 i_crossbarInput;
    uvec4 i_selectOutput;
    uvec2 i_images[16 * 6];
    uint i_format;
    uint i_pitch;
    uint i_width;
    uint i_height;
    uint i_levels;
    uint i_offset;
    uint i_isSwizzled;
    uint i_isSignedRemapClamped;
    uint i_isUnsignedRemapBiased;
    uint i_texelSize;
    bool i_cube;
};

uint endian_reverse(uint i) {
    return ((i & 0xff) << 24)
         | ((i & 0xff00) << 8)
         | ((i & 0xff0000) >> 8)
         | ((i & 0xff000000) >> 24);
}

uint read_aligned_le_32(uint byteIndex) {
    byteIndex += i_offset;
    uint wordIndex = byteIndex / 4;
    return u32[wordIndex];
}

uint read32(uint byteIndex) {
    byteIndex += i_offset;
    uint wordIndex = byteIndex / 4;
    int subwordIndex = int(byteIndex % 4);
    uint word = endian_reverse(u32[wordIndex]);
    if (subwordIndex == 0)
        return word;
    uint nextWord = endian_reverse(u32[wordIndex + 1]);
    return (word << subwordIndex * 8) | (nextWord >> (32 - subwordIndex * 8));
}

uint ext8(uint val, uint bits) {
    return (val << (8 - bits)) | (val >> (2 * bits - 8));
}

int getLevel(inout int y,
             out uint offset,
             inout uint levelWidth,
             inout uint levelHeight,
             inout uint levelPitch) {
    offset = 0;
    for (int level = 0; level < int(i_levels); level++) {
        if (uint(y) < levelHeight)
            return level;
        y -= int(levelHeight);
        offset += levelPitch * levelHeight;
        levelWidth = max(levelWidth / 2, 1);
        levelHeight = max(levelHeight / 2, 1);
        if (i_isSwizzled != 0)
            levelPitch = levelWidth * i_texelSize;
    }
    return -1;
}

uint getImageSize(uint pitch, uint height) {
    uint offset = 0;
    for (uint level = 0; level < i_levels; level++) {
        offset += pitch * height;
        height = max(height / 2, 1);
        if (i_isSwizzled != 0)
            pitch = max(pitch / 2, 1);
    }
    return offset;
}

uint align(uint n, uint alignment) {
    return (n + alignment - 1) & (~(alignment - 1));
}

uint getLayerOffset(uint pitch, uint height) {
    if (i_cube) {
        uint layerSize = align(getImageSize(pitch, height), 128);
        return layerSize * gl_GlobalInvocationID.z;
    } else {
        return 0;
    }
}

)"";

const char* destFirstShaderSource =
R""(

uint swizzleAddress(uint x, uint y, uint z, uint w, uint h, uint d) {
    uint offset = 0;
    uint shift = 0;
    while (w != 0 || h != 0 || d != 0) {
        if (w != 0) {
            offset |= (x & 1) << shift;
            x >>= 1;
            ++shift;
            --w;
        }
        if (h != 0) {
            offset |= (y & 1) << shift;
            y >>= 1;
            ++shift;
            --h;
        }
        if (d != 0) {
            offset |= (z & 1) << shift;
            z >>= 1;
            ++shift;
            --d;
        }
    }
    return offset;
}

float remap(uint f, uint sign, uint gamma) {
    float r = f;
    if (sign == 1) {
        r = f > 0x80 ? -((~f & 0xff) + 1) : f;
        if (i_isSignedRemapClamped == 1) {
            r = (r + 128.f) / 127.5f - 1.f;
        } else {
            r /= 127;
        }
    } else if (gamma == 1) {
        r = (r + 128.f) / 255.f;
        r = r <= 0.03928f ? r / 12.92f : pow((r + 0.055f) / 1.055f, 2.4f);
    } else {
        if (i_isUnsignedRemapBiased == 1) {
            r = (r - 128.f) / 255.f;
        } else {
            r /= 255.f;
        }
    }
    return r;
}

uint crossbar(ivec4 vec, uint _input) {
    switch (_input) {
        case TextureRemapInput::FromR: return vec.r;
        case TextureRemapInput::FromG: return vec.g;
        case TextureRemapInput::FromB: return vec.b;
        case TextureRemapInput::FromA: return vec.a;
        default: return 0;
    }
}

float select(float f, uint _output) {
    switch (_output) {
        case TextureRemapOutput::Zero: return 0.f;
        case TextureRemapOutput::One: return 1.f;
        case TextureRemapOutput::Remap: return f;
        default: return 0;
    }
}

void main(void) {
    ivec2 dest = ivec2(gl_GlobalInvocationID.xy);
    
    uint levelOffset;
    uint levelWidth = i_width;
    uint levelHeight = i_height;
    uint levelPitch = i_pitch;
    int level = getLevel(dest.y, levelOffset, levelWidth, levelHeight, levelPitch);
    
    levelOffset += getLayerOffset(i_pitch, i_height);
    
    if (dest.x >= levelWidth || level == -1)
        return;
    
    ivec2 src = dest;
    if (i_isSwizzled != 0) {
        uint logw = uint(log2(i_width));
        uint logh = uint(log2(i_height));
        uint swizzled = swizzleAddress(dest.x, dest.y, 0, logw, logh, 0);
        src = ivec2(swizzled & ((1 << logw) - 1), swizzled >> logw);
    }
    
    uint src_index = 0;
    uint val = 0;
    vec4 color = vec4(0);
    ivec4 icolor = ivec4(0);
    bool doremap = false;
    
    switch (i_format) {
        case GcmTextureFormat::A8R8G8B8:
            src_index = src.y * levelPitch + src.x * 4;
            val = read32(levelOffset + src_index);
            icolor = ivec4(
                bitfieldExtract(val, 16, 8),
                bitfieldExtract(val, 8, 8),
                bitfieldExtract(val, 0, 8),
                bitfieldExtract(val, 24, 8)
            );
            doremap = true;
            break;
        case GcmTextureFormat::D8R8G8B8:
            src_index = src.y * levelPitch + src.x * 4;
            val = read32(levelOffset + src_index);
            icolor = ivec4(
                bitfieldExtract(val, 16, 8),
                bitfieldExtract(val, 8, 8),
                bitfieldExtract(val, 0, 8),
                0xff
            );
            doremap = true;
            break;
        case GcmTextureFormat::A1R5G5B5:
            src_index = src.y * levelPitch + src.x * 2;
            val = read32(levelOffset + src_index) >> 16;
            icolor = ivec4(
                ext8(bitfieldExtract(val, 10, 5), 5),
                ext8(bitfieldExtract(val, 5, 5), 5),
                ext8(bitfieldExtract(val, 0, 5), 5),
                bitfieldExtract(val, 15, 1) == 1 ? 0xff : 0
            );
            doremap = true;
            break;
        case GcmTextureFormat::R5G5B5A1:
            src_index = src.y * levelPitch + src.x * 2;
            val = read32(levelOffset + src_index) >> 16;
            icolor = ivec4(
                ext8(bitfieldExtract(val, 11, 5), 5),
                ext8(bitfieldExtract(val, 6, 5), 5),
                ext8(bitfieldExtract(val, 1, 5), 5),
                bitfieldExtract(val, 0, 1) == 1 ? 0xff : 0
            );
            doremap = true;
            break;
        case GcmTextureFormat::A4R4G4B4:
            src_index = src.y * levelPitch + src.x * 2;
            val = read32(levelOffset + src_index) >> 16;
            icolor = ivec4(
                ext8(bitfieldExtract(val, 8, 4), 4),
                ext8(bitfieldExtract(val, 4, 4), 4),
                ext8(bitfieldExtract(val, 0, 4), 4),
                ext8(bitfieldExtract(val, 12, 4), 4)
            );
            doremap = true;
            break;
        case GcmTextureFormat::R5G6B5:
            src_index = src.y * levelPitch + src.x * 2;
            val = read32(levelOffset + src_index) >> 16;
            icolor = ivec4(
                ext8(bitfieldExtract(val, 11, 5), 5),
                ext8(bitfieldExtract(val, 5, 6), 6),
                ext8(bitfieldExtract(val, 0, 5), 5),
                0xff
            );
            doremap = true;
            break;
        case GcmTextureFormat::D1R5G5B5:
            src_index = src.y * levelPitch + src.x * 2;
            val = read32(levelOffset + src_index) >> 16;
            icolor = ivec4(
                ext8(bitfieldExtract(val, 10, 5), 5),
                ext8(bitfieldExtract(val, 5, 5), 5),
                ext8(bitfieldExtract(val, 0, 5), 5),
                0xff
            );
            doremap = true;
            break;
        case GcmTextureFormat::R6G5B5:
            src_index = src.y * levelPitch + src.x * 2;
            val = read32(levelOffset + src_index) >> 16;
            icolor = ivec4(
                ext8(bitfieldExtract(val, 10, 6), 6),
                ext8(bitfieldExtract(val, 5, 5), 5),
                ext8(bitfieldExtract(val, 0, 5), 5),
                0xff
            );
            doremap = true;
            break;
        case GcmTextureFormat::B8:
            src_index = src.y * levelPitch + src.x;
            val = read32(levelOffset + src_index) >> 24;
            icolor = ivec4(
                val,
                val,
                val,
                0xff
            );
            doremap = true;
            break;
        case GcmTextureFormat::W32_Z32_Y32_X32_FLOAT:
            src_index = src.y * levelPitch + src.x * 16;
            color = vec4(
                uintBitsToFloat(read32(levelOffset + src_index)),
                uintBitsToFloat(read32(levelOffset + src_index + 4)),
                uintBitsToFloat(read32(levelOffset + src_index + 8)),
                uintBitsToFloat(read32(levelOffset + src_index + 12))
            );
            break;
        case GcmTextureFormat::W16_Z16_Y16_X16_FLOAT:
            src_index = src.y * levelPitch + src.x * 8;
            color = vec4(
                unpackHalf2x16(read32(levelOffset + src_index)),
                unpackHalf2x16(read32(levelOffset + src_index + 4))
            );
            break;
        case GcmTextureFormat::Y16_X16_FLOAT:
            src_index = src.y * levelPitch + src.x * 4;
            color = vec4(
                unpackHalf2x16(read32(levelOffset + src_index)),
                vec2(0, 0)
            );
            break;
        case GcmTextureFormat::DEPTH16:
            src_index = src.y * levelPitch + src.x * 2;
            val = read32(levelOffset + src_index) >> 16;
            color = vec4(
                float(val) / 255.1,
                vec3(0, 0, 0)
            );
            break;
    }
    
    if (doremap) {
        icolor = ivec4(crossbar(icolor, i_crossbarInput.r),
                       crossbar(icolor, i_crossbarInput.g),
                       crossbar(icolor, i_crossbarInput.b),
                       crossbar(icolor, i_crossbarInput.a));
        color = vec4(remap(icolor.r, i_fragmentSign.r, i_fragmentGamma.r),
                     remap(icolor.g, i_fragmentSign.g, i_fragmentGamma.g),
                     remap(icolor.b, i_fragmentSign.b, i_fragmentGamma.b),
                     remap(icolor.a, i_fragmentSign.a, i_fragmentGamma.a));
        color = vec4(select(color.r, i_selectOutput.r),
                     select(color.g, i_selectOutput.g),
                     select(color.b, i_selectOutput.b),
                     select(color.a, i_selectOutput.a));
    }
    
    layout(rgba32f) image2D image = layout(rgba32f)
        image2D(i_images[level + gl_GlobalInvocationID.z * i_levels]);
    imageStore(image, dest, color);
}

)""
;

const char* sourceFirstShaderSource =
R""(
    
uvec4 readRGB565(uint val) {
    return uvec4(ext8(bitfieldExtract(val, 11, 5), 5),
                 ext8(bitfieldExtract(val, 5, 6), 6),
                 ext8(bitfieldExtract(val, 0, 5), 5),
                 0xff);
}

uvec4 c[4];

void decodeDXT1Color(uint word, bool force4Colors) {
    uint u16_0 = word & 0xffff;
    uint u16_1 = word >> 16;
    c[0] = readRGB565(u16_0);
    c[1] = readRGB565(u16_1);
    if (force4Colors || u16_0 > u16_1) {
        c[2] = uvec4((2 * c[0].rgb + c[1].rgb) / 3, 0xff);
        c[3] = uvec4((2 * c[1].rgb + c[0].rgb) / 3, 0xff);
    } else {
        c[2] = uvec4((c[0].rgb + c[1].rgb) / 2, 0xff);
        c[3] = uvec4(0);
    }
}

uint reverse16(uint u) {
    return ((u >> 8) & 0x00ff00ff) | ((u << 8) & 0xff00ff00);
}

void main(void) {
    bool isDXT1 = i_format == GcmTextureFormat::COMPRESSED_DXT1;
    bool isDXT23 = i_format == GcmTextureFormat::COMPRESSED_DXT23;
    uint blockWidth = i_width / 4;
    uint blockHeight = i_height / 4;
    uint pitch = i_pitch / 4;
    ivec2 src = ivec2(gl_GlobalInvocationID.xy);
    
    uint levelOffset;
    uint levelWidth = blockWidth;
    uint levelHeight = blockHeight;
    uint layerOffset = getLayerOffset(pitch, blockHeight);
    int level = getLevel(src.y, levelOffset, levelWidth, levelHeight, pitch);
    
    if (src.x >= blockWidth || level == -1)
        return;
    
    uint blockOffset = layerOffset + levelOffset + src.y * pitch + src.x * i_texelSize;
    
    uvec4 colors[16];
    
    uint w0 = read_aligned_le_32(blockOffset);
    uint w1 = read_aligned_le_32(blockOffset + 4);
    
    if (isDXT1) {
        decodeDXT1Color(w0, false);
        for (int i = 0; i < 16; i++) {
            colors[i] = c[w1 & 3];
            w1 >>= 2;
        }
    } else if (isDXT23) {
        colors[15].a = ext8(w1 >> 28, 4);
        colors[14].a = ext8((w1 >> 24) & 0xf, 4);
        colors[13].a = ext8((w1 >> 20) & 0xf, 4);
        colors[12].a = ext8((w1 >> 16) & 0xf, 4);
        colors[11].a = ext8((w1 >> 12) & 0xf, 4);
        colors[10].a = ext8((w1 >> 8) & 0xf, 4);
        colors[9].a = ext8((w1 >> 4) & 0xf, 4);
        colors[8].a = ext8(w1 & 0xf, 4);
        colors[7].a = ext8(w0 >> 28, 4);
        colors[6].a = ext8((w0 >> 24) & 0xf, 4);
        colors[5].a = ext8((w0 >> 20) & 0xf, 4);
        colors[4].a = ext8((w0 >> 16) & 0xf, 4);
        colors[3].a = ext8((w0 >> 12) & 0xf, 4);
        colors[2].a = ext8((w0 >> 8) & 0xf, 4);
        colors[1].a = ext8((w0 >> 4) & 0xf, 4);
        colors[0].a = ext8(w0 & 0xf, 4);
        uint w2 = read_aligned_le_32(blockOffset + 8);
        uint w3 = read_aligned_le_32(blockOffset + 12);
        decodeDXT1Color(w2, true);
        for (int i = 0; i < 16; i++) {
            colors[i].rgb = c[w3 & 3].rgb;
            w3 >>= 2;
        }
    } else {
        uint a[8];
        a[0] = w0 & 0xff;
        a[1] = (w0 >> 8) & 0xff;
        if (a[0] > a[1]) {
            a[2] = (6 * a[0] + 1 * a[1]) / 7;
            a[3] = (5 * a[0] + 2 * a[1]) / 7;
            a[4] = (4 * a[0] + 3 * a[1]) / 7;
            a[5] = (3 * a[0] + 4 * a[1]) / 7;
            a[6] = (2 * a[0] + 5 * a[1]) / 7;
            a[7] = (1 * a[0] + 6 * a[1]) / 7;
        } else {
            a[2] = (4 * a[0] + 1 * a[1]) / 5;
            a[3] = (3 * a[0] + 2 * a[1]) / 5;
            a[4] = (2 * a[0] + 3 * a[1]) / 5;
            a[5] = (1 * a[0] + 4 * a[1]) / 5;
            a[6] = 0;
            a[7] = 0xff;
        }
        uint index = w0 >> 16;
        for (int i = 0; i < 5; ++i) {
            colors[i].a = a[index & 7];
            index >>= 3;
        }
        index |= w1 << 1;
        for (int i = 5; i < 16; ++i) {
            colors[i].a = a[index & 7];
            index >>= 3;
        }
        uint w2 = read_aligned_le_32(blockOffset + 8);
        uint w3 = read_aligned_le_32(blockOffset + 12);

        decodeDXT1Color(w2, false);
        for (int i = 0; i < 16; i++) {
            colors[i].rgb = c[w3 & 3].rgb;
            w3 >>= 2;
        }
    }
    
    layout(rgba32f) image2D image = layout(rgba32f)
        image2D(i_images[level + gl_GlobalInvocationID.z * i_levels]);
    ivec2 dest = src * 4;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            ivec2 out_dest = dest + ivec2(x, y);
            if (out_dest.x >= levelWidth * 4 || out_dest.y >= levelHeight * 4)
                continue;
            vec4 color = vec4(colors[4 * y + x]) / 255.f;
            imageStore(image, out_dest, color);
        }
    }
}

)"";


const char* debugFillShaderSource =
R""(
    
void main(void) {
    ivec2 dest = ivec2(gl_GlobalInvocationID.xy);
    
    uint levelOffset;
    uint levelWidth = i_width;
    uint levelHeight = i_height;
    uint levelPitch = i_pitch;
    int level = getLevel(dest.y, levelOffset, levelWidth, levelHeight, levelPitch);
    
    if (dest.x >= levelWidth || level == -1)
        return;
    
    layout(rgba32f) image2D image = layout(rgba32f)
        image2D(i_images[level + gl_GlobalInvocationID.z * i_levels]);
    level += 1;
    vec4 color = vec4(level & 1, (level >> 1) & 1, (level >> 2) & 1, 1);
    if ((dest.y / 3) % 2 == 0)
        color = vec4(1,1,1,1);
    imageStore(image, dest, color);
}

)"";

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

#pragma pack(1)
struct TextureInfoUniform {
    std::array<uint32_t, 4> fragmentGamma; // rgba
    std::array<uint32_t, 4> fragmentSign; // rgba
    std::array<uint32_t, 4> crossbarInput; // rgba
    std::array<uint32_t, 4> selectOutput; // rgba
    std::array<uint64_t, 16 * 6> images;
    uint32_t format;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint32_t levels;
    uint32_t offset;
    uint32_t isSwizzled;
    uint32_t isSignedRemapClamped;
    uint32_t isUnsignedRemapBiased;
    uint32_t texelSize;
    uint32_t cube;
};
#pragma pack()

template<typename E>
void patchEnumValues(std::string& text) {
    auto values = enum_traits<E>::values();
    auto names = enum_traits<E>::names();
    auto name = enum_traits<E>::name();
    std::vector<std::tuple<std::string, unsigned>> pairs;
    for (auto i = 0u; i < values.size(); ++i) {
        pairs.push_back(std::make_tuple(std::string(names[i]), (unsigned)values[i]));
    }
    std::sort(begin(pairs), end(pairs), [&](auto l, auto r) {
        return std::get<0>(l).size() > std::get<0>(r).size();
    });
    for (auto i = 0u; i < pairs.size(); ++i) {
        boost::replace_all(text,
                           ssnprintf("%s::%s", name, std::get<0>(pairs[i])),
                           ssnprintf("%d", std::get<1>(pairs[i])));
    }
}

void initShader(Shader& shader, std::string text) {
    patchEnumValues<GcmTextureFormat>(text);
    patchEnumValues<TextureRemapInput>(text);
    patchEnumValues<TextureRemapOutput>(text);
    shader = Shader(GL_COMPUTE_SHADER, text.c_str());
    if (!shader.log().empty()) {
        std::cout << shader.source();
        std::cout << shader.log() << std::endl;
        assert(false);
    }
}

void RsxTextureReader::init() {
    std::string text = std::string(commonShaderSource) + destFirstShaderSource;
    initShader(_destFirstShader, text);
    text = std::string(commonShaderSource) + sourceFirstShaderSource;
    initShader(_sourceFirstShader, text);
    _uniformBuffer = GLPersistentCpuBuffer(sizeof(TextureInfoUniform));
#ifdef DEBUG
    text = std::string(commonShaderSource) + debugFillShaderSource;
    initShader(_debugFillShader, text);
#endif
}

unsigned getTexelSize(GcmTextureFormat format) {
    switch (format) {
        case GcmTextureFormat::A8R8G8B8:
        case GcmTextureFormat::D8R8G8B8:
        case GcmTextureFormat::DEPTH24_D8:
        case GcmTextureFormat::DEPTH24_D8_FLOAT:
        case GcmTextureFormat::Y16_X16:
        case GcmTextureFormat::X32_FLOAT:
        case GcmTextureFormat::Y16_X16_FLOAT:
            return 4;
        case GcmTextureFormat::A1R5G5B5:
        case GcmTextureFormat::R5G5B5A1:
        case GcmTextureFormat::A4R4G4B4:
        case GcmTextureFormat::R5G6B5:
        case GcmTextureFormat::D1R5G5B5:
        case GcmTextureFormat::R6G5B5:
        case GcmTextureFormat::G8B8:        
        case GcmTextureFormat::DEPTH16:
        case GcmTextureFormat::DEPTH16_FLOAT:
        case GcmTextureFormat::X16:
        case GcmTextureFormat::COMPRESSED_HILO8:
        case GcmTextureFormat::COMPRESSED_HILO_S8:
            return 2;
        case GcmTextureFormat::B8:
            return 1;
        case GcmTextureFormat::W16_Z16_Y16_X16_FLOAT:
        case GcmTextureFormat::COMPRESSED_DXT1:
            return 8;
        case GcmTextureFormat::W32_Z32_Y32_X32_FLOAT:
        case GcmTextureFormat::COMPRESSED_DXT23:
        case GcmTextureFormat::COMPRESSED_DXT45:
            return 16;
        default: assert(false);
    }
    return {};
}

void RsxTextureReader::loadTexture(RsxTextureInfo const& info,
                                   GLuint buffer,
                                   std::vector<uint64_t> const& levelHandles) {
    auto uniform = (TextureInfoUniform*)_uniformBuffer.mapped();
    uniform->format = (uint32_t)info.format;
    uniform->width = info.width;
    uniform->height = info.height;
    uniform->levels = info.mipmap;
    uniform->offset = info.offset;
    uniform->isSwizzled = !(info.lnUn & GcmTextureLnUn::LN);
    uniform->texelSize = getTexelSize(info.format);
    uniform->pitch =
        uniform->isSwizzled ? uniform->texelSize * info.width : info.pitch;
    uniform->isSignedRemapClamped =
        info.fragmentSignedRemap == CELL_GCM_TEXTURE_SIGNED_REMAP_CLAMPED;
    uniform->isUnsignedRemapBiased =
        info.fragmentUnsignedRemap == CELL_GCM_TEXTURE_UNSIGNED_REMAP_BIASED;
    uniform->cube = info.fragmentCubemap;
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
    
    auto layers = info.fragmentCubemap ? 6 : 1;
    
    assert(info.mipmap <= 12);
    assert(info.mipmap == levelHandles.size() / layers);
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _uniformBuffer.handle());
    
    for (auto i = 0u; i < levelHandles.size(); ++i) {
        uniform->images[i] = levelHandles[i];
    }
    
#ifdef DEBUG
    glUseProgram(_debugFillShader.handle());
    glDispatchCompute((info.width + 31) / 32, (info.height * 2 + 31) / 32, layers);
#endif
    
    if (info.format == GcmTextureFormat::COMPRESSED_DXT1 ||
        info.format == GcmTextureFormat::COMPRESSED_DXT23 ||
        info.format == GcmTextureFormat::COMPRESSED_DXT45) {
        glUseProgram(_sourceFirstShader.handle());
        // height + 10 because several last levels might be 1 block long each
        // this means when counting in blocks, 2*height might be slightly not enough
        glDispatchCompute((info.width / 4 + 31) / 32,
                          (info.height * 2 / 4 + 10 + 31) / 32,
                          layers);
    } else {
        glUseProgram(_destFirstShader.handle());
        // twice the height to take into account all mipmap levels
        glDispatchCompute((info.width + 31) / 32, (info.height * 2 + 31) / 32, layers);
    }

    glUseProgram(0);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    glFinish(); // TODO: see the DrawArrays comment on synchronization
}
