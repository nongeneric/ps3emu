#pragma once

#include "../shaders/shader_dasm.h"
#include "GLBuffer.h"
#include "GLShader.h"

class MainMemory;
class RsxContext;
class FragmentShaderUpdateFunctor {
    GLBuffer _constBuffer;
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
    FragmentShaderUpdateFunctor(uint32_t va, 
                                uint32_t size,
                                RsxContext* rsxContext,
                                MainMemory* mm);
    void updateWithBlob(FragmentShader* shader, std::vector<uint8_t>& blob);
    void update(FragmentShader* shader);
    void bindConstBuffer();
    std::vector<uint8_t> const& bytecode();
    GLBuffer* constBuffer();
};