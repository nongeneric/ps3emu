#pragma once

#include "../shaders/shader_dasm.h"
#include "GLBuffer.h"
#include "GLShader.h"

class MainMemory;
struct RsxContext;
struct SurfaceInfo;
class FragmentShaderUpdateFunctor {
    GLPersistentCpuBuffer _constBuffer;
    std::vector<uint8_t> _bytecode;
    std::vector<uint8_t> _newbytecode;
    FragmentProgramInfo _info;
    RsxContext* _context;
    MainMemory* _mm;
    
    void updateBytecode(FragmentShader* shader);
    void updateConsts();
public:
    uint32_t va;
    uint32_t size;
    bool mrt;
    FragmentShaderUpdateFunctor(uint32_t va, 
                                uint32_t size,
                                bool mrt,
                                RsxContext* rsxContext,
                                MainMemory* mm);
    void updateWithBlob(FragmentShader* shader, std::vector<uint8_t>& blob);
    void update(FragmentShader* shader);
    void bindConstBuffer();
    std::vector<uint8_t> const& bytecode();
    GLPersistentCpuBuffer* constBuffer();
};

bool isMrt(SurfaceInfo const& surface);
std::array<int, 16> getFragmentSamplerSizes(const RsxContext* context);
