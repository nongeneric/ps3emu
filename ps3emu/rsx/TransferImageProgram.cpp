#include "TransferImageProgram.h"
#include "ShaderUtils.h"

#include <fmt/format.h>

namespace {

const char* shaderSource =
R""(

#version 450 core

layout (local_size_x = 32, local_size_y = 32) in;
layout (std430, binding = 0) buffer SrcBuf {
    uint src_u32[];
};
layout (std430, binding = 1) buffer DstBuf {
    volatile uint dst_u32[];
};
layout (std430, binding = 2) buffer Info {
    uint i_src_format;
    uint i_src_w;
    uint i_src_h;
    uint i_src_pitch;
    uint i_src_offset;
    float i_src_x;
    float i_src_y;
    uint i_dst_swizzled;
    uint i_dst_swizzle_log_width;
    uint i_dst_swizzle_log_height;
    uint i_dst_format;
    uint i_dst_pitch;
    uint i_dst_w;
    uint i_dst_h;
    int i_dst_x;
    int i_dst_y;
    uint i_dst_offset;
    int i_clip_x;
    int i_clip_y;
    uint i_clip_w;
    uint i_clip_h;
    float i_dsdx;
    float i_dtdy;
};

uint endian_reverse(uint i) {
    return ((i & 0xff) << 24)
         | ((i & 0xff00) << 8)
         | ((i & 0xff0000) >> 8)
         | ((i & 0xff000000) >> 24);
}

uint read32_be(uint byteIndex) {
    uint wordIndex = byteIndex / 4;
    int subwordIndex = int(byteIndex % 4);
    uint word = endian_reverse(src_u32[wordIndex]);
    if (subwordIndex == 0)
        return word;
    uint nextWord = endian_reverse(src_u32[wordIndex + 1]);
    return (word << subwordIndex * 8) | (nextWord >> (32 - subwordIndex * 8));
}

uint read32_dst_be(uint byteIndex) {
    uint wordIndex = byteIndex / 4;
    int subwordIndex = int(byteIndex % 4);
    uint word = endian_reverse(dst_u32[wordIndex]);
    if (subwordIndex == 0)
        return word;
    uint nextWord = endian_reverse(src_u32[wordIndex + 1]);
    return (word << subwordIndex * 8) | (nextWord >> (32 - subwordIndex * 8));
}

void write32_aligned_be(uint byteIndex, uint val) {
    uint wordIndex = byteIndex / 4;
    dst_u32[wordIndex] = endian_reverse(val);
}

void write16_aligned_be(uint byteIndex, uint val) {
    uint new = 0;
    do {
        uint old = dst_u32[byteIndex / 4];
        uint old_be = endian_reverse(old);
        new = (byteIndex % 4 == 2) ? ((old_be & 0xffff0000) | val)
            : ((old_be & 0xffff) | (val << 16));
        new = endian_reverse(new);
        atomicCompSwap(dst_u32[byteIndex / 4], old, new);
    } while (dst_u32[byteIndex / 4] != new);
}

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

uint ext8(uint val, uint bits) {
    return (val << (8 - bits)) | (val >> (2 * bits - 8));
}

void main(void) {
    ivec2 dst = ivec2(gl_GlobalInvocationID.xy);
    uint dstPixelSize = i_dst_format == ScaleSettingsFormat::r5g6b5 ? 2 : 4;
    uint srcPixelSize = i_src_format == ScaleSettingsFormat::r5g6b5 ? 2 : 4;

    uint dstPixelOffset = i_dst_offset;
    if (i_dst_swizzled == 1) {
        dstPixelOffset += dstPixelSize * swizzleAddress(dst.x,
                                                        dst.y,
                                                        0,
                                                        i_dst_swizzle_log_width,
                                                        i_dst_swizzle_log_height,
                                                        0);
    } else {
        dstPixelOffset += dst.y * i_dst_pitch + dst.x * dstPixelSize;
    }

    int dstX0 = max(i_dst_x, i_clip_x);
    uint dstXn = min(i_dst_x + i_dst_w, i_clip_x + i_clip_w);
    int dstY0 = max(i_dst_y, i_clip_y);
    uint dstYn = min(i_dst_y + i_dst_h, i_clip_y + i_clip_h);
    int clipDiffX = i_clip_x > i_dst_x ? i_clip_x - i_dst_x : 0;
    int clipDiffY = i_clip_y > i_dst_y ? i_clip_y - i_dst_y : 0;

    if (dst.x < dstX0 || dst.y < dstY0)
        return;
    if (dst.x >= dstXn || dst.y >= dstYn)
        return;

    int srcX = int(clamp(i_src_x + (dst.x - dstX0 + clipDiffX) * i_dsdx,
                         .0f, i_src_w - 1));
    int srcY = int(clamp(i_src_y + (dst.y - dstY0 + clipDiffY) * i_dtdy,
                         .0f, i_src_h - 1));
    uint srcPixelOffset = i_src_offset + srcY * i_src_pitch + srcX * srcPixelSize;

    uint srcPixel = read32_be(srcPixelOffset);
    if (srcPixelSize == 2) {
        srcPixel >>= 16;
    }
    uint dstPixel = srcPixel;

    if (dstPixelSize != srcPixelSize) {
        if (srcPixelSize == 2) {
            uint r = ext8(bitfieldExtract(srcPixel, 11, 6), 6);
            uint g = ext8(bitfieldExtract(srcPixel, 5, 6), 6);
            uint b = ext8(bitfieldExtract(srcPixel, 0, 5), 5);
            dstPixel = (r << 16) | (g << 8) | b;
        } else {
            uint r = (((srcPixel >> 16) & 0xff) * (1 << 5)) / 255;
            uint g = (((srcPixel >> 8) & 0xff) * (1 << 6)) / 255;
            uint b = ((srcPixel & 0xff) * (1 << 5)) / 255;
            dstPixel = (r << 11) | (g << 5) | b;
        }
    }

    if (dstPixelSize == 4) {
        write32_aligned_be(dstPixelOffset, dstPixel);
    } else {
        write16_aligned_be(dstPixelOffset, dstPixel);
    }
}

)"";
}

#pragma pack(1)
struct Uniform {
    uint32_t i_src_format;
    uint32_t i_src_w;
    uint32_t i_src_h;
    uint32_t i_src_pitch;
    uint32_t i_src_offset;
    float i_src_x;
    float i_src_y;
    uint32_t i_dst_swizzled;
    uint32_t i_dst_swizzle_log_width;
    uint32_t i_dst_swizzle_log_height;
    uint32_t i_dst_format;
    uint32_t i_dst_pitch;
    uint32_t i_dst_w;
    uint32_t i_dst_h;
    int32_t i_dst_x;
    int32_t i_dst_y;
    uint32_t i_dst_offset;
    int32_t i_clip_x;
    int32_t i_clip_y;
    uint32_t i_clip_w;
    uint32_t i_clip_h;
    float i_dsdx;
    float i_dtdy;
};
#pragma pack()

TransferImageProgram::TransferImageProgram() {
    std::string text = shaderSource;
    patchEnumValues<ScaleSettingsFormat>(text);
    _shader = Shader(GL_COMPUTE_SHADER, text.c_str());
    if (!_shader.log().empty()) {
        fmt::print("{}{}\n", _shader.source(), _shader.log());
        assert(false);
    }
    _uniformBuffer = GLPersistentCpuBuffer(sizeof(Uniform));
}

void TransferImageProgram::transfer(GLuint srcBuffer,
                                    GLuint dstBuffer,
                                    const ScaleSettings& scale,
                                    const SurfaceSettings& surface,
                                    const SwizzleSettings& swizzle) {
    auto uniform = (Uniform*)_uniformBuffer.mapped();
    uniform->i_src_format = (uint32_t)scale.format;
    uniform->i_src_w = scale.inW;
    uniform->i_src_h = scale.inH;
    uniform->i_src_pitch = scale.pitch;
    uniform->i_src_offset = scale.offset;
    uniform->i_src_x = scale.inX;
    uniform->i_src_y = scale.inY;
    uniform->i_dst_swizzled = scale.type == ScaleSettingsSurfaceType::Swizzle;
    uniform->i_dst_swizzle_log_width = swizzle.logWidth;
    uniform->i_dst_swizzle_log_height = swizzle.logHeight;
    uniform->i_dst_format = (uint32_t)(uniform->i_dst_swizzled ? swizzle.format : surface.format);
    uniform->i_dst_pitch = surface.pitch;
    uniform->i_dst_w = scale.outW;
    uniform->i_dst_h = scale.outH;
    uniform->i_dst_x = scale.outX;
    uniform->i_dst_y = scale.outY;
    uniform->i_dst_offset = uniform->i_dst_swizzled ? swizzle.offset : surface.destOffset;
    uniform->i_clip_x = scale.clipX;
    uniform->i_clip_y = scale.clipY;
    uniform->i_clip_w = scale.clipW;
    uniform->i_clip_h = scale.clipH;
    uniform->i_dsdx = scale.dsdx;
    uniform->i_dtdy = scale.dtdy;

    assert(surface.pitch % 2 == 0);
    assert(surface.destOffset % 4 == 0);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, srcBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dstBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _uniformBuffer.handle());

    glUseProgram(_shader.handle());

    auto destXn = std::min<int16_t>(scale.outX + scale.outW, scale.clipX + scale.clipW);
    auto destYn = std::min<int16_t>(scale.outY + scale.outH, scale.clipY + scale.clipH);
    glDispatchCompute((destXn + 31) / 32, (destYn + 31) / 32, 1);

    glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glFinish(); // TODO: see the DrawArrays comment on synchronization
}
