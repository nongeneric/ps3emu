#include "FragmentShaderUpdateFunctor.h"

#include "RsxContext.h"
#include "Tracer.h"
#include "../MainMemory.h"

void FragmentShaderUpdateFunctor::updateBytecode(FragmentShader* shader) {
    // TODO: handle sizes
    std::array<int, 16> sizes = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    auto text = GenerateFragmentShader(_newbytecode, sizes, _context->isFlatShadeMode);
    *shader = FragmentShader(text.c_str());
    BOOST_LOG_TRIVIAL(trace) << "fragment shader log:\n" << shader->log();

    _bytecode = _newbytecode;
    _info = get_fragment_bytecode_info(&_bytecode[0]);
    updateConsts();
}

void FragmentShaderUpdateFunctor::updateConsts() {
    auto fconst = (std::array<float, 4>*)_constBuffer.mapped();
    for (auto i = 0u; i < _info.length; i += 16) {
        if (_info.constMap[i / 16]) {
            *fconst = read_fragment_imm_val(&_newbytecode[i]);
            fconst++;
        }
    }
    _constBuffer.flush(0, _info.length * 16);
}

FragmentShaderUpdateFunctor::FragmentShaderUpdateFunctor(uint32_t va, uint32_t size, RsxContext* rsxContext, MainMemory* mm)
    : _constBuffer(size / 2),
      _context(rsxContext),
      _mm(mm),
      va(va), size(size)
{
    assert(size % 16 == 0);
}

void FragmentShaderUpdateFunctor::updateWithBlob(FragmentShader* shader, std::vector<uint8_t>& blob) {
    if (blob.empty()) {
        _newbytecode.resize(size);
        _mm->readMemory(va, &_newbytecode[0], size);
    } else {
        assert(blob.size() == size);
        _newbytecode.resize(size);
        memcpy(&_newbytecode[0], &blob[0], size);
    }

    _context->tracer.pushBlob(&_newbytecode[0], size);
    TRACE2(UpdateFragmentCache, va, size);

    if (_bytecode.empty()) { // first update
        updateBytecode(shader);
        return;
    }

    bool constsChanged = false;
    bool bytecodeChanged = false;
    for (auto i = 0u; i < size; i += 16) {
        auto equals = memcmp(&_bytecode[i], &_newbytecode[i], 16) == 0;
        if (!equals) {
            if (_info.constMap[i / 16]) {
                constsChanged = true;
            } else {
                bytecodeChanged = true;
            }
        }
    }
    if (bytecodeChanged) {
        updateBytecode(shader);
    } else if (constsChanged) {
        updateConsts();
    }
}

void FragmentShaderUpdateFunctor::update(FragmentShader* shader) {
    std::vector<uint8_t> blob;
    return updateWithBlob(shader, blob);
}

void FragmentShaderUpdateFunctor::bindConstBuffer() {
    glcall(glBindBufferBase(GL_UNIFORM_BUFFER,
                            FragmentShaderConstantBinding,
                            _constBuffer.handle()));
}

std::vector<uint8_t> const& FragmentShaderUpdateFunctor::bytecode() {
    return _bytecode;
}

GLPersistentBuffer* FragmentShaderUpdateFunctor::constBuffer() {
    return &_constBuffer;
}
