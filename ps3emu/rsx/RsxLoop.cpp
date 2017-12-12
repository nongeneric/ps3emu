#include "Rsx.h"
#include "RsxLoopMeta.h"
#include "../utils.h"
#include "../BitField.h"
#include "../MainMemory.h"
#include "../libs/graphics/graphics.h"
#include "Tracer.h"
#include "../log.h"
#include "ps3emu/state.h"
#include "GcmConstants.h"
#include <vector>
#include <map>
#include <assert.h>

using namespace boost::chrono;

uint32_t swap16(uint32_t v) {
    return (v >> 16) | (v << 16);
}

uint32_t swap02(uint32_t val) {
    return (val & 0xff00ff00) | ((val >> 16) & 0xff) | (((val >> 0) & 0xff) << 16);
}

std::array<float, 4> parseColor(uint32_t raw) {
    union {
        uint32_t val;
        BitField<0, 8> a;
        BitField<8, 16> r;
        BitField<16, 24> g;
        BitField<24, 32> b;
    } arg = { raw };
    std::array<float, 4> color {
        (float)arg.a.u() / 255, 
        (float)arg.r.u() / 255, 
        (float)arg.g.u() / 255,
        (float)arg.b.u() / 255
    };
    return color;
}

float fixedSint32ToFloat(int32_t val) {
    return (float)val / 1048576.f;
}

float fixedUint16ToFloat(uint16_t val) {
    return (float)val / 16.f;
}

float fixedUint9ToFloat(uint32_t val) {
    return (float)val / 8.f;
}

int64_t Rsx::interpret(uint32_t get, const uint32_t* read) {
    MethodHeader header { fast_endian_reverse(read[0]) };
    
    if (header.val == 0) {
        assert(header.count.u() == 0);
        INFO(rsx) << ssnprintf("%08x/%08x | rsx nop", get, _put.load());
        return 4;
    }

    _currentCount = header.count.u();
    _currentGet = read;
    _currentGetValue = get;
    auto offset = header.offset.u();

    if (header.prefix.u() == 1) {
        auto offset = header.jumpoffset.u();
        if (offset != get) { // don't log on busy wait
            INFO(rsx) << ssnprintf("rsx jump to %x", offset);
        }
        return offset - get;
    }

    if (header.callsuffix.u() == 2) {
        auto offset = header.calloffset.u() << 2;
        _ret = get + 4;
        INFO(rsx) << ssnprintf("rsx call %x to %x", get, offset);
        return offset - get;
    }

    if (header.val == 0x20000) {
        INFO(rsx) << ssnprintf("rsx ret to %x", _ret.load());
        if (!get) {
            ERROR(rsx) << "rsx ret to 0, command buffer corruption is likely";
        }
        auto offset = _ret - get;
        _ret = 0;
        return offset;
    }

    auto& entry = _methodMap[offset / 4];

    RangeCloser closer;
    if (log_should(log_warning, log_rsx, log_perf)) {
        closer = RangeCloser(&_perfMap[&entry]);
    }

    __itt_task_begin(_profilerDomain, __itt_null, __itt_null, entry.task);
    (this->*(entry.handler))(entry.index);
    __itt_task_end(_profilerDomain);

    auto len = (header.count.u() + 1) * 4;
    assert(len != 0);
    return len;
}

void Rsx::setPut(uint32_t put) {
    _put = put;
}

void Rsx::setGet(uint32_t get) {
    _get = get;
}

uint32_t Rsx::getRef() {
    return _ref;
}

void Rsx::setRef(uint32_t ref) {
    _ref = ref;
}

void Rsx::loop() {
    initGcm();
    _get = 0;
    _put = 0;
    _ret = 0;
    _ref = 0xffffffff;
    _isFlipInProgress = false;
    log_set_thread_name("rsx_loop");
    INFO(rsx) << "rsx loop started, waiting for updates";
    if (_mode == RsxOperationMode::Replay) {
        replayLoop();
    } else {
        runLoop();
    }
    shutdownGcm();
}

void Rsx::runLoop() {
    for (;;) {
        while (_get != _put || _ret) {
            auto localGet = _get.load();
            auto ptr = (uint32_t*)g_state.mm->getMemoryPointer(rsxOffsetToEa(MemoryLocation::Main, localGet), 0);
            auto len = interpret(localGet, ptr);
            _get += len;
        }
        if (_shutdown && _get == _put) {
            waitForIdle();
            return;
        }
    }
}

void Rsx::replayLoop() {
    for (;;) {
        auto info = _replayQueue.receive(0);
        auto command = info.command;
        auto id = (CommandId)command.id;
        if (id == CommandId::StopReplay)
            break;
        std::vector<uint32_t> argValues;
        std::transform(begin(command.args), 
                       end(command.args), 
                       std::back_inserter(argValues),
                       [](auto& arg) { return arg.value; });
        _currentReplayBlob = command.blob;

        auto past = boost::chrono::steady_clock::now();

        switch (id) {
#define X(x) case CommandId::x: replay(&Rsx::x, this, argValues); break;
            CommandIdX
#undef X    
            default: break;
       }
       
       if (info.action) {
           info.action(boost::chrono::steady_clock::now() - past);
       }
       
       if (info.notifyCompletion) {
           _completionQueue.send(true);
       }
    }
}

void Rsx::shutdown() {
    if (!_initialized)
        return;
    
    if (!_shutdown) {
        INFO(rsx) << "waiting for shutdown";
        {
            auto lock = boost::unique_lock(_mutex);
            _shutdown = true;
            _cv.notify_all();
        }
        _thread->join();
        INFO(rsx) << "shutdown";
    }
}

void Rsx::encodeJump(ps3_uintptr_t va, uint32_t destOffset) {
    assert((destOffset & 3) == 0);
    MethodHeader header { 0 };
    header.prefix.set(1);
    header.jumpoffset.set(destOffset);
    g_state.mm->store32(va, header.val, g_state.granule);
}

void Rsx::sendCommand(GcmCommandReplayInfo info) {
    _replayQueue.send(info);
}

bool Rsx::receiveCommandCompletion() {
    return _completionQueue.receive(0);
}

void Rsx::unknown_impl(int index) {
    throw std::runtime_error("unknown method offset");
}

uint32_t Rsx::readarg(int n) {
    assert(n != 0);
    return fast_endian_reverse(_currentGet[n]);
}

void Rsx::initMethodMap() {
    _methodMap.resize(0x3ac1);
    std::fill(begin(_methodMap), end(_methodMap), MethodMapEntry{&Rsx::unknown_impl, "unknown", 0});

#define SINGLE(offset) \
    _methodMap.at(offset / 4) = {&Rsx:: offset##_impl, #offset, 0};

#define INDEXED(offset, step, count) \
    assert(offset % 4 == 0); \
    assert(step % 4 == 0); \
    for (int i = offset; i < offset + step * count; i += step) \
        _methodMap.at(i / 4) = {&Rsx:: offset##_impl, #offset, (i - offset) / step};

    SINGLE(CELL_GCM_NV406E_SET_REFERENCE)
    SINGLE(CELL_GCM_NV406E_SET_CONTEXT_DMA_SEMAPHORE)
    SINGLE(CELL_GCM_NV406E_SEMAPHORE_OFFSET)
    SINGLE(CELL_GCM_NV406E_SEMAPHORE_ACQUIRE)
    SINGLE(CELL_GCM_NV406E_SEMAPHORE_RELEASE)
    SINGLE(CELL_GCM_NV4097_SET_OBJECT)
    SINGLE(CELL_GCM_NV4097_NO_OPERATION)
    SINGLE(CELL_GCM_NV4097_NOTIFY)
    SINGLE(CELL_GCM_NV4097_WAIT_FOR_IDLE)
    SINGLE(CELL_GCM_NV4097_PM_TRIGGER)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_NOTIFIES)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_A)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_B)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_B)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_STATE)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_A)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_ZETA)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_A)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_B)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_SEMAPHORE)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_REPORT)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_CLIP_ID)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_CULL_DATA)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_C)
    SINGLE(CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_D)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_CLIP_HORIZONTAL)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_CLIP_VERTICAL)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_FORMAT)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_PITCH_A)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_COLOR_AOFFSET)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_ZETA_OFFSET)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_COLOR_BOFFSET)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_PITCH_B)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_COLOR_TARGET)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_PITCH_Z)
    SINGLE(CELL_GCM_NV4097_INVALIDATE_ZCULL)
    SINGLE(CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP)
    SINGLE(CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP1)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_PITCH_C)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_PITCH_D)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_COLOR_COFFSET)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_COLOR_DOFFSET)
    SINGLE(CELL_GCM_NV4097_SET_WINDOW_OFFSET)
    SINGLE(CELL_GCM_NV4097_SET_WINDOW_CLIP_TYPE)
    SINGLE(CELL_GCM_NV4097_SET_WINDOW_CLIP_HORIZONTAL)
    SINGLE(CELL_GCM_NV4097_SET_WINDOW_CLIP_VERTICAL)
    SINGLE(CELL_GCM_NV4097_SET_DITHER_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_ALPHA_TEST_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_ALPHA_FUNC)
    SINGLE(CELL_GCM_NV4097_SET_ALPHA_REF)
    SINGLE(CELL_GCM_NV4097_SET_BLEND_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_BLEND_FUNC_SFACTOR)
    SINGLE(CELL_GCM_NV4097_SET_BLEND_FUNC_DFACTOR)
    SINGLE(CELL_GCM_NV4097_SET_BLEND_COLOR)
    SINGLE(CELL_GCM_NV4097_SET_BLEND_EQUATION)
    SINGLE(CELL_GCM_NV4097_SET_COLOR_MASK)
    SINGLE(CELL_GCM_NV4097_SET_STENCIL_TEST_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_STENCIL_MASK)
    SINGLE(CELL_GCM_NV4097_SET_STENCIL_FUNC)
    SINGLE(CELL_GCM_NV4097_SET_STENCIL_FUNC_REF)
    SINGLE(CELL_GCM_NV4097_SET_STENCIL_FUNC_MASK)
    SINGLE(CELL_GCM_NV4097_SET_STENCIL_OP_FAIL)
    SINGLE(CELL_GCM_NV4097_SET_STENCIL_OP_ZFAIL)
    SINGLE(CELL_GCM_NV4097_SET_STENCIL_OP_ZPASS)
    SINGLE(CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_BACK_STENCIL_MASK)
    SINGLE(CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC)
    SINGLE(CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_REF)
    SINGLE(CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_MASK)
    SINGLE(CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL)
    SINGLE(CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZFAIL)
    SINGLE(CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZPASS)
    SINGLE(CELL_GCM_NV4097_SET_SHADE_MODE)
    SINGLE(CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT)
    SINGLE(CELL_GCM_NV4097_SET_COLOR_MASK_MRT)
    SINGLE(CELL_GCM_NV4097_SET_LOGIC_OP_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_LOGIC_OP)
    SINGLE(CELL_GCM_NV4097_SET_BLEND_COLOR2)
    SINGLE(CELL_GCM_NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MIN)
    SINGLE(CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MAX)
    SINGLE(CELL_GCM_NV4097_SET_CLIP_MIN)
    SINGLE(CELL_GCM_NV4097_SET_CLIP_MAX)
    SINGLE(CELL_GCM_NV4097_SET_CONTROL0)
    SINGLE(CELL_GCM_NV4097_SET_LINE_WIDTH)
    SINGLE(CELL_GCM_NV4097_SET_LINE_SMOOTH_ENABLE)
    INDEXED(CELL_GCM_NV4097_SET_ANISO_SPREAD, 4, CELL_GCM_MAX_VERTEX_TEXTURE)
    SINGLE(CELL_GCM_NV4097_SET_SCISSOR_HORIZONTAL)
    SINGLE(CELL_GCM_NV4097_SET_SCISSOR_VERTICAL)
    SINGLE(CELL_GCM_NV4097_SET_FOG_MODE)
    SINGLE(CELL_GCM_NV4097_SET_FOG_PARAMS)
    SINGLE(CELL_GCM_NV4097_SET_SHADER_PROGRAM)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET, 32, CELL_GCM_MAX_VERTEX_TEXTURE)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FORMAT)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS, 32, CELL_GCM_MAX_VERTEX_TEXTURE)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0, 32, CELL_GCM_MAX_VERTEX_TEXTURE)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3, 32, CELL_GCM_MAX_VERTEX_TEXTURE)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER, 32, CELL_GCM_MAX_VERTEX_TEXTURE)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT, 32, CELL_GCM_MAX_VERTEX_TEXTURE)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR, 32, CELL_GCM_MAX_VERTEX_TEXTURE)
    SINGLE(CELL_GCM_NV4097_SET_VIEWPORT_HORIZONTAL)
    SINGLE(CELL_GCM_NV4097_SET_VIEWPORT_VERTICAL)
    SINGLE(CELL_GCM_NV4097_SET_POINT_CENTER_MODE)
    SINGLE(CELL_GCM_NV4097_ZCULL_SYNC)
    SINGLE(CELL_GCM_NV4097_SET_VIEWPORT_OFFSET)
    SINGLE(CELL_GCM_NV4097_SET_VIEWPORT_SCALE)
    SINGLE(CELL_GCM_NV4097_SET_POLY_OFFSET_POINT_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_POLY_OFFSET_LINE_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_POLY_OFFSET_FILL_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_DEPTH_FUNC)
    SINGLE(CELL_GCM_NV4097_SET_DEPTH_MASK)
    SINGLE(CELL_GCM_NV4097_SET_DEPTH_TEST_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR)
    SINGLE(CELL_GCM_NV4097_SET_POLYGON_OFFSET_BIAS)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA_SCALED4S_M)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_CONTROL2, 4, 16)
    INDEXED(CELL_GCM_NV4097_SET_TEX_COORD_CONTROL, 4, 16)
    INDEXED(CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM, 16, 1)
    SINGLE(CELL_GCM_NV4097_SET_SPECULAR_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_TWO_SIDE_LIGHT_EN)
    SINGLE(CELL_GCM_NV4097_CLEAR_ZCULL_SURFACE)
    SINGLE(CELL_GCM_NV4097_SET_PERFORMANCE_PARAMS)
    SINGLE(CELL_GCM_NV4097_SET_FLAT_SHADE_OP)
    SINGLE(CELL_GCM_NV4097_SET_EDGE_FLAG)
    SINGLE(CELL_GCM_NV4097_SET_USER_CLIP_PLANE_CONTROL)
    SINGLE(CELL_GCM_NV4097_SET_POLYGON_STIPPLE)
    SINGLE(CELL_GCM_NV4097_SET_POLYGON_STIPPLE_PATTERN)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA3F_M)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 4, 16)
    SINGLE(CELL_GCM_NV4097_INVALIDATE_VERTEX_CACHE_FILE)
    SINGLE(CELL_GCM_NV4097_INVALIDATE_VERTEX_FILE)
    SINGLE(CELL_GCM_NV4097_PIPE_NOP)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_OFFSET)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_INDEX)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 4, 16)
    SINGLE(CELL_GCM_NV4097_CLEAR_REPORT_VALUE)
    SINGLE(CELL_GCM_NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE)
    SINGLE(CELL_GCM_NV4097_GET_REPORT)
    SINGLE(CELL_GCM_NV4097_SET_ZCULL_STATS_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_BEGIN_END)
    SINGLE(CELL_GCM_NV4097_ARRAY_ELEMENT16)
    SINGLE(CELL_GCM_NV4097_ARRAY_ELEMENT32)
    SINGLE(CELL_GCM_NV4097_DRAW_ARRAYS)
    SINGLE(CELL_GCM_NV4097_INLINE_ARRAY)
    SINGLE(CELL_GCM_NV4097_SET_INDEX_ARRAY_ADDRESS)
    SINGLE(CELL_GCM_NV4097_SET_INDEX_ARRAY_DMA)
    SINGLE(CELL_GCM_NV4097_DRAW_INDEX_ARRAY)
    SINGLE(CELL_GCM_NV4097_SET_FRONT_POLYGON_MODE)
    SINGLE(CELL_GCM_NV4097_SET_BACK_POLYGON_MODE)
    SINGLE(CELL_GCM_NV4097_SET_CULL_FACE)
    SINGLE(CELL_GCM_NV4097_SET_FRONT_FACE)
    SINGLE(CELL_GCM_NV4097_SET_POLY_SMOOTH_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_CULL_FACE_ENABLE)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_CONTROL3, 4, CELL_GCM_MAX_TEXIMAGE_COUNT)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA2F_M)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA2S_M)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA4UB_M)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA4S_M)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_OFFSET, 32, CELL_GCM_MAX_TEXIMAGE_COUNT)
    SINGLE(CELL_GCM_NV4097_SET_TEXTURE_FORMAT)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_ADDRESS, 32, CELL_GCM_MAX_TEXIMAGE_COUNT)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_CONTROL0, 32, CELL_GCM_MAX_TEXIMAGE_COUNT)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_CONTROL1, 32, CELL_GCM_MAX_TEXIMAGE_COUNT)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_FILTER, 32, CELL_GCM_MAX_TEXIMAGE_COUNT)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT, 32, CELL_GCM_MAX_TEXIMAGE_COUNT)
    INDEXED(CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR, 32, CELL_GCM_MAX_TEXIMAGE_COUNT)
    INDEXED(CELL_GCM_NV4097_SET_VERTEX_DATA4F_M, 16, CELL_GCM_MAX_TEXIMAGE_COUNT)
    SINGLE(CELL_GCM_NV4097_SET_COLOR_KEY_COLOR)
    SINGLE(CELL_GCM_NV4097_SET_SHADER_CONTROL)
    SINGLE(CELL_GCM_NV4097_SET_INDEXED_CONSTANT_READ_LIMITS)
    SINGLE(CELL_GCM_NV4097_SET_SEMAPHORE_OFFSET)
    SINGLE(CELL_GCM_NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE)
    SINGLE(CELL_GCM_NV4097_TEXTURE_READ_SEMAPHORE_RELEASE)
    SINGLE(CELL_GCM_NV4097_SET_ZMIN_MAX_CONTROL)
    SINGLE(CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL)
    SINGLE(CELL_GCM_NV4097_SET_SURFACE_COMPRESSION)
    SINGLE(CELL_GCM_NV4097_SET_ZCULL_EN)
    SINGLE(CELL_GCM_NV4097_SET_SHADER_WINDOW)
    SINGLE(CELL_GCM_NV4097_SET_ZSTENCIL_CLEAR_VALUE)
    SINGLE(CELL_GCM_NV4097_SET_COLOR_CLEAR_VALUE)
    SINGLE(CELL_GCM_NV4097_CLEAR_SURFACE)
    SINGLE(CELL_GCM_NV4097_SET_CLEAR_RECT_HORIZONTAL)
    SINGLE(CELL_GCM_NV4097_SET_CLEAR_RECT_VERTICAL)
    SINGLE(CELL_GCM_NV4097_SET_CLIP_ID_TEST_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_RESTART_INDEX_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_RESTART_INDEX)
    SINGLE(CELL_GCM_NV4097_SET_LINE_STIPPLE)
    SINGLE(CELL_GCM_NV4097_SET_LINE_STIPPLE_PATTERN)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_DATA1F_M)
    SINGLE(CELL_GCM_NV4097_SET_TRANSFORM_EXECUTION_MODE)
    SINGLE(CELL_GCM_NV4097_SET_RENDER_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_LOAD)
    SINGLE(CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_START)
    SINGLE(CELL_GCM_NV4097_SET_ZCULL_CONTROL0)
    SINGLE(CELL_GCM_NV4097_SET_ZCULL_CONTROL1)
    SINGLE(CELL_GCM_NV4097_SET_SCULL_CONTROL)
    SINGLE(CELL_GCM_NV4097_SET_POINT_SIZE)
    SINGLE(CELL_GCM_NV4097_SET_POINT_PARAMS_ENABLE)
    SINGLE(CELL_GCM_NV4097_SET_POINT_SPRITE_CONTROL)
    SINGLE(CELL_GCM_NV4097_SET_TRANSFORM_TIMEOUT)
    SINGLE(CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD)
    SINGLE(CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT)
    SINGLE(CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION)
    SINGLE(CELL_GCM_NV4097_SET_ATTRIB_COLOR)
    SINGLE(CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD)
    SINGLE(CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_EX)
    SINGLE(CELL_GCM_NV4097_SET_ATTRIB_UCLIP0)
    SINGLE(CELL_GCM_NV4097_SET_ATTRIB_UCLIP1)
    SINGLE(CELL_GCM_NV4097_INVALIDATE_L2)
    SINGLE(CELL_GCM_NV4097_SET_REDUCE_DST_COLOR)
    SINGLE(CELL_GCM_NV4097_SET_NO_PARANOID_TEXTURE_FETCHES)
    SINGLE(CELL_GCM_NV4097_SET_SHADER_PACKER)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_ATTRIB_INPUT_MASK)
    SINGLE(CELL_GCM_NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK)
    SINGLE(CELL_GCM_NV4097_SET_TRANSFORM_BRANCH_BITS)
    SINGLE(CELL_GCM_NV0039_SET_OBJECT)
    SINGLE(CELL_GCM_NV0039_SET_CONTEXT_DMA_NOTIFIES)
    SINGLE(CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_IN)
    SINGLE(CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_OUT)
    SINGLE(CELL_GCM_NV0039_OFFSET_IN)
    SINGLE(CELL_GCM_NV0039_OFFSET_OUT)
    SINGLE(CELL_GCM_NV0039_PITCH_IN)
    SINGLE(CELL_GCM_NV0039_PITCH_OUT)
    SINGLE(CELL_GCM_NV0039_LINE_LENGTH_IN)
    SINGLE(CELL_GCM_NV0039_LINE_COUNT)
    SINGLE(CELL_GCM_NV0039_FORMAT)
    SINGLE(CELL_GCM_NV0039_BUFFER_NOTIFY)
    SINGLE(CELL_GCM_NV3062_SET_OBJECT)
    SINGLE(CELL_GCM_NV3062_SET_CONTEXT_DMA_NOTIFIES)
    SINGLE(CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE)
    SINGLE(CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN)
    SINGLE(CELL_GCM_NV3062_SET_COLOR_FORMAT)
    SINGLE(CELL_GCM_NV3062_SET_PITCH)
    SINGLE(CELL_GCM_NV3062_SET_OFFSET_SOURCE)
    SINGLE(CELL_GCM_NV3062_SET_OFFSET_DESTIN)
    SINGLE(CELL_GCM_NV309E_SET_OBJECT)
    SINGLE(CELL_GCM_NV309E_SET_CONTEXT_DMA_NOTIFIES)
    SINGLE(CELL_GCM_NV309E_SET_CONTEXT_DMA_IMAGE)
    SINGLE(CELL_GCM_NV309E_SET_FORMAT)
    SINGLE(CELL_GCM_NV309E_SET_OFFSET)
    SINGLE(CELL_GCM_NV308A_SET_OBJECT)
    SINGLE(CELL_GCM_NV308A_SET_CONTEXT_DMA_NOTIFIES)
    SINGLE(CELL_GCM_NV308A_SET_CONTEXT_COLOR_KEY)
    SINGLE(CELL_GCM_NV308A_SET_CONTEXT_CLIP_RECTANGLE)
    SINGLE(CELL_GCM_NV308A_SET_CONTEXT_PATTERN)
    SINGLE(CELL_GCM_NV308A_SET_CONTEXT_ROP)
    SINGLE(CELL_GCM_NV308A_SET_CONTEXT_BETA1)
    SINGLE(CELL_GCM_NV308A_SET_CONTEXT_BETA4)
    SINGLE(CELL_GCM_NV308A_SET_CONTEXT_SURFACE)
    SINGLE(CELL_GCM_NV308A_SET_COLOR_CONVERSION)
    SINGLE(CELL_GCM_NV308A_SET_OPERATION)
    SINGLE(CELL_GCM_NV308A_SET_COLOR_FORMAT)
    SINGLE(CELL_GCM_NV308A_POINT)
    SINGLE(CELL_GCM_NV308A_SIZE_OUT)
    SINGLE(CELL_GCM_NV308A_SIZE_IN)
    SINGLE(CELL_GCM_NV308A_COLOR)
    SINGLE(CELL_GCM_NV3089_SET_OBJECT)
    SINGLE(CELL_GCM_NV3089_SET_CONTEXT_DMA_NOTIFIES)
    SINGLE(CELL_GCM_NV3089_SET_CONTEXT_DMA_IMAGE)
    SINGLE(CELL_GCM_NV3089_SET_CONTEXT_PATTERN)
    SINGLE(CELL_GCM_NV3089_SET_CONTEXT_ROP)
    SINGLE(CELL_GCM_NV3089_SET_CONTEXT_BETA1)
    SINGLE(CELL_GCM_NV3089_SET_CONTEXT_BETA4)
    SINGLE(CELL_GCM_NV3089_SET_CONTEXT_SURFACE)
    SINGLE(CELL_GCM_NV3089_SET_COLOR_CONVERSION)
    SINGLE(CELL_GCM_NV3089_SET_COLOR_FORMAT)
    SINGLE(CELL_GCM_NV3089_SET_OPERATION)
    SINGLE(CELL_GCM_NV3089_CLIP_POINT)
    SINGLE(CELL_GCM_NV3089_CLIP_SIZE)
    SINGLE(CELL_GCM_NV3089_IMAGE_OUT_POINT)
    SINGLE(CELL_GCM_NV3089_IMAGE_OUT_SIZE)
    SINGLE(CELL_GCM_NV3089_DS_DX)
    SINGLE(CELL_GCM_NV3089_DT_DY)
    SINGLE(CELL_GCM_NV3089_IMAGE_IN_SIZE)
    SINGLE(CELL_GCM_NV3089_IMAGE_IN_FORMAT)
    SINGLE(CELL_GCM_NV3089_IMAGE_IN_OFFSET)
    SINGLE(CELL_GCM_NV3089_IMAGE_IN)
    SINGLE(CELL_GCM_DRIVER_INTERRUPT)
    SINGLE(CELL_GCM_DRIVER_QUEUE)
    SINGLE(CELL_GCM_DRIVER_FLIP)
    SINGLE(CELL_GCM_unknown_240)
    SINGLE(CELL_GCM_unknown_e000)

#undef SINGLE
#undef INDEXED

    for (auto& m : _methodMap) {
        m.task = __itt_string_handle_create(m.name + 9);
    }
}

void Rsx::parseTextureAddress(int argi, int index) {
    union {
        uint32_t val;
        BitField<0, 4> zfunc;
        BitField<4, 8> signedRemap;
        BitField<8, 12> gamma;
        BitField<12, 16> wrapr;
        BitField<16, 20> unsignedRemap;
        BitField<20, 24> wrapt;
        BitField<24, 28> anisoBias;
        BitField<28, 32> wraps;
    } arg = { readarg(argi) };
    TextureAddress(
        index,
        arg.wraps.u(),
        arg.wrapt.u(),
        arg.wrapr.u(),
        arg.unsignedRemap.u(),
        arg.zfunc.u(),
        arg.gamma.u(),
        arg.anisoBias.u(),
        arg.signedRemap.u()
    );
}

void Rsx::parseTextureControl0(int argi, int index) {
    union {
        uint32_t val;
        BitField<0, 1> enable;
        BitField<1, 5> minlod_i;
        BitField<5, 13> minlod_d;
        BitField<13, 17> maxlod_i;
        BitField<17, 25> maxlod_d;
        BitField<25, 28> maxaniso;
        BitField<28, 30> alphakill;
    } arg = { readarg(argi) };
    TextureControl0(
        index,
        arg.alphakill.u(),
        arg.maxaniso.u(),
        arg.maxlod_i.u() + (float)arg.maxlod_d.u() / 255.f,
        arg.minlod_i.u() + (float)arg.minlod_d.u() / 255.f,
        arg.enable.u()
    );
}

void Rsx::parseTextureFilter(int argi, int index) {
    union {
        uint32_t val;
        BitField<0, 1> bs;
        BitField<1, 2> gs;
        BitField<2, 3> rs;
        BitField<3, 4> as;
        BitField<4, 8> mag;
        BitField<8, 16> min;
        BitField<16, 19> conv;
        BitField<19, 24> biasInteger;
        BitField<24, 32> biasDecimal;
    } arg = { readarg(argi) };
    TextureFilter(
        index,
        arg.biasInteger.s() + arg.biasDecimal.u() / 255.f,
        arg.min.u(),
        arg.mag.u(),
        arg.conv.u(),
        arg.as.u(),
        arg.rs.u(),
        arg.gs.u(),
        arg.bs.u()
    );
}

void Rsx::parseTextureBorderColor(int argi, int index) {
    auto c = parseColor(readarg(argi));
    TextureBorderColor(index, c[0], c[1], c[2], c[3]);
}

void Rsx::parseTextureImageRect(int argi, int index) {
    auto arg = readarg(argi);
    TextureImageRect(index, arg >> 16, arg & 0xffff);
}

void Rsx::CELL_GCM_NV406E_SET_REFERENCE_impl(int index) {
    SetReference(readarg(1));
}

void Rsx::CELL_GCM_NV406E_SET_CONTEXT_DMA_SEMAPHORE_impl(int index) {
    ChannelSetContextDmaSemaphore(readarg(1));
}

void Rsx::CELL_GCM_NV406E_SEMAPHORE_OFFSET_impl(int index) {
    ChannelSemaphoreOffset(readarg(1));
}

void Rsx::CELL_GCM_NV406E_SEMAPHORE_ACQUIRE_impl(int index) {
    ChannelSemaphoreAcquire(readarg(1));
}

void Rsx::CELL_GCM_NV406E_SEMAPHORE_RELEASE_impl(int index) {
    SemaphoreRelease(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_OBJECT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_OBJECT";
}

void Rsx::CELL_GCM_NV4097_NO_OPERATION_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_NO_OPERATION";
}

void Rsx::CELL_GCM_NV4097_NOTIFY_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_NOTIFY";
}

void Rsx::CELL_GCM_NV4097_WAIT_FOR_IDLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_WAIT_FOR_IDLE";
}

void Rsx::CELL_GCM_NV4097_PM_TRIGGER_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_PM_TRIGGER";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_NOTIFIES_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_NOTIFIES";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_A_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_A";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_B_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_B";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_B_impl(int index) {
    ContextDmaColorB(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_STATE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_STATE";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_A_impl(int index) {
    ContextDmaColorA(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_ZETA_impl(int index) {
    ContextDmaZeta(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_A_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_A";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_B_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_B";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_SEMAPHORE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_SEMAPHORE";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_REPORT_impl(int index) {
    ContextDmaReport(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_CLIP_ID_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_CLIP_ID";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_CULL_DATA_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CONTEXT_DMA_CULL_DATA";
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_C_impl(int index) {
    if (_currentCount == 1) {
        ContextDmaColorC_1(readarg(1));
    } else {
        ContextDmaColorC_2(readarg(1), readarg(2));
    }
}

void Rsx::CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_D_impl(int index) {
    ContextDmaColorD(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_CLIP_HORIZONTAL_impl(int index) {
    auto arg1 = readarg(1);
    auto arg2 = readarg(2);
    SurfaceClipHorizontal(arg1 & 0xff, arg1 >> 16, arg2 & 0xff, arg2 >> 16);
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_CLIP_VERTICAL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_CLIP_VERTICAL";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_FORMAT_impl(int index) {
    union {
        uint32_t val;
        BitField<0, 8> height;
        BitField<8, 16> width;
        BitField<16, 20> antialias;
        BitField<20, 24> type;
        BitField<24, 27> depthFormat;
        BitField<27, 32> colorFormat;
    } arg = {readarg(1)};

    SurfaceFormat(enum_cast<GcmSurfaceColor>(arg.colorFormat.u()),
                  enum_cast<SurfaceDepthFormat>(arg.depthFormat.u()),
                  arg.antialias.u(),
                  arg.type.u(),
                  arg.width.u(),
                  arg.height.u(),
                  readarg(2),
                  readarg(3),
                  readarg(4),
                  readarg(5),
                  readarg(6));
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_PITCH_A_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_PITCH_A";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_COLOR_AOFFSET_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_COLOR_AOFFSET";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_ZETA_OFFSET_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_ZETA_OFFSET";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_COLOR_BOFFSET_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_COLOR_BOFFSET";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_PITCH_B_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_PITCH_B";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_COLOR_TARGET_impl(int index) {
    SurfaceColorTarget(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_PITCH_Z_impl(int index) {
    SurfacePitchZ(readarg(1));
}

void Rsx::CELL_GCM_NV4097_INVALIDATE_ZCULL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_INVALIDATE_ZCULL";
}

void Rsx::CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP";
}

void Rsx::CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP1_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP1";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_PITCH_C_impl(int index) {
    SurfacePitchC(readarg(1), readarg(2), readarg(3), readarg(4));
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_PITCH_D_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_PITCH_D";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_COLOR_COFFSET_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_COLOR_COFFSET";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_COLOR_DOFFSET_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SURFACE_COLOR_DOFFSET";
}

void Rsx::CELL_GCM_NV4097_SET_WINDOW_OFFSET_impl(int index) {
    auto arg1 = readarg(1);
    WindowOffset(arg1 & 0xff, arg1 >> 16);
}

void Rsx::CELL_GCM_NV4097_SET_WINDOW_CLIP_TYPE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_WINDOW_CLIP_TYPE";
}

void Rsx::CELL_GCM_NV4097_SET_WINDOW_CLIP_HORIZONTAL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_WINDOW_CLIP_HORIZONTAL";
}

void Rsx::CELL_GCM_NV4097_SET_WINDOW_CLIP_VERTICAL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_WINDOW_CLIP_VERTICAL";
}

void Rsx::CELL_GCM_NV4097_SET_DITHER_ENABLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_DITHER_ENABLE";
}

void Rsx::CELL_GCM_NV4097_SET_ALPHA_TEST_ENABLE_impl(int index) {
    AlphaTestEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_ALPHA_FUNC_impl(int index) {
    AlphaFunc(enum_cast<GcmOperator>(readarg(1)), readarg(2));
}

void Rsx::CELL_GCM_NV4097_SET_ALPHA_REF_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ALPHA_REF";
}

void Rsx::CELL_GCM_NV4097_SET_BLEND_ENABLE_impl(int index) {
    BlendEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_BLEND_FUNC_SFACTOR_impl(int index) {
    auto arg1 = readarg(1);
    auto arg2 = readarg(2);
    BlendFuncSFactor(enum_cast<GcmBlendFunc>(arg1 & 0xffff),
                     enum_cast<GcmBlendFunc>(arg1 >> 16),
                     enum_cast<GcmBlendFunc>(arg2 & 0xffff),
                     enum_cast<GcmBlendFunc>(arg2 >> 16));
}

void Rsx::CELL_GCM_NV4097_SET_BLEND_FUNC_DFACTOR_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BLEND_FUNC_DFACTOR";
}

void Rsx::CELL_GCM_NV4097_SET_BLEND_COLOR_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BLEND_COLOR";
}

void Rsx::CELL_GCM_NV4097_SET_BLEND_EQUATION_impl(int index) {
    auto arg = readarg(1);
    BlendEquation(enum_cast<GcmBlendEquation>(arg & 0xffff),
                  enum_cast<GcmBlendEquation>(arg >> 16));
}

void Rsx::CELL_GCM_NV4097_SET_COLOR_MASK_impl(int index) {
    ColorMask(enum_cast<GcmColorMask>(readarg(1)));
}

void Rsx::CELL_GCM_NV4097_SET_STENCIL_TEST_ENABLE_impl(int index) {
    StencilTestEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_STENCIL_MASK_impl(int index) {
    StencilMask(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_STENCIL_FUNC_impl(int index) {
    StencilFunc(enum_cast<GcmOperator>(readarg(1)), readarg(2), readarg(3));
}

void Rsx::CELL_GCM_NV4097_SET_STENCIL_FUNC_REF_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_STENCIL_FUNC_REF";
}

void Rsx::CELL_GCM_NV4097_SET_STENCIL_FUNC_MASK_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_STENCIL_FUNC_MASK";
}

void Rsx::CELL_GCM_NV4097_SET_STENCIL_OP_FAIL_impl(int index) {
    StencilOpFail(readarg(1), readarg(2), readarg(3));
}

void Rsx::CELL_GCM_NV4097_SET_STENCIL_OP_ZFAIL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_STENCIL_OP_ZFAIL";
}

void Rsx::CELL_GCM_NV4097_SET_STENCIL_OP_ZPASS_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_STENCIL_OP_ZPASS";
}

void Rsx::CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE";
}

void Rsx::CELL_GCM_NV4097_SET_BACK_STENCIL_MASK_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BACK_STENCIL_MASK";
}

void Rsx::CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC";
}

void Rsx::CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_REF_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_REF";
}

void Rsx::CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_MASK_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_MASK";
}

void Rsx::CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL";
}

void Rsx::CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZFAIL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZFAIL";
}

void Rsx::CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZPASS_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZPASS";
}

void Rsx::CELL_GCM_NV4097_SET_SHADE_MODE_impl(int index) {
    ShadeMode(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT";
}

void Rsx::CELL_GCM_NV4097_SET_COLOR_MASK_MRT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_COLOR_MASK_MRT";
}

void Rsx::CELL_GCM_NV4097_SET_LOGIC_OP_ENABLE_impl(int index) {
    LogicOpEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_LOGIC_OP_impl(int index) {
    LogicOp(enum_cast<GcmLogicOp>(readarg(1)));
}

void Rsx::CELL_GCM_NV4097_SET_BLEND_COLOR2_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_BLEND_COLOR2";
}

void Rsx::CELL_GCM_NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE";
}

void Rsx::CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MIN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MIN";
}

void Rsx::CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MAX_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MAX";
}

void Rsx::CELL_GCM_NV4097_SET_CLIP_MIN_impl(int index) {
    ClipMin(union_cast<uint32_t, float>(readarg(1)),
            union_cast<uint32_t, float>(readarg(2)));
}

void Rsx::CELL_GCM_NV4097_SET_CLIP_MAX_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CLIP_MAX";
}

void Rsx::CELL_GCM_NV4097_SET_CONTROL0_impl(int index) {
    Control0(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_LINE_WIDTH_impl(int index) {
    LineWidth(fixedUint9ToFloat(readarg(1)));
}

void Rsx::CELL_GCM_NV4097_SET_LINE_SMOOTH_ENABLE_impl(int index) {
    LineSmoothEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_ANISO_SPREAD_impl(int index) {
    union {
        uint32_t val;
        BitField<11, 12> vReduceSamplesEnable;
        BitField<13, 16> vSpacingSelect;
        BitField<19, 20> hReduceSamplesEnable;
        BitField<21, 24> hSpacingSelect;
        BitField<27, 28> reduceSamplesEnable;
        BitField<29, 32> spacingSelect;
    } arg = { readarg(1) };
    AnisoSpread(
        index,
        arg.reduceSamplesEnable.u(),
        arg.hReduceSamplesEnable.u(),
        arg.vReduceSamplesEnable.u(),
        arg.spacingSelect.u(),
        arg.hSpacingSelect.u(),
        arg.vSpacingSelect.u()
    );
}

void Rsx::CELL_GCM_NV4097_SET_SCISSOR_HORIZONTAL_impl(int index) {
    auto arg1 = readarg(1);
    auto arg2 = readarg(2);
    ScissorHorizontal(arg1 & 0xffff, arg1 >> 16, arg2 & 0xffff, arg2 >> 16);
}

void Rsx::CELL_GCM_NV4097_SET_SCISSOR_VERTICAL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SCISSOR_VERTICAL";
}

void Rsx::CELL_GCM_NV4097_SET_FOG_MODE_impl(int index) {
    FogMode(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_FOG_PARAMS_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_FOG_PARAMS";
}

void Rsx::CELL_GCM_NV4097_SET_SHADER_PROGRAM_impl(int index) {
    auto arg = readarg(1);
    ShaderProgram(arg & ~0b11ul, (arg & 0b11) - 1);
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET_impl(int index) {
    union {
        uint32_t val;
        BitField<8, 16> mipmap;
        BitField<16, 24> format;
        BitField<24, 28> dimension;
        BitField<28, 32> location;
    } f = {readarg(2)};
    auto lnUnMask = (uint8_t)(GcmTextureLnUn::LN | GcmTextureLnUn::UN);
    auto formatMask = ~lnUnMask;
    auto format = enum_cast<GcmTextureFormat>(f.format.u() & formatMask);
    auto lnUn = enum_cast<GcmTextureLnUn>(f.format.u() & lnUnMask);
    VertexTextureOffset(index,
                        readarg(1),
                        f.mipmap.u(),
                        format,
                        lnUn,
                        f.dimension.u(),
                        f.location.u() - 1);
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FORMAT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FORMAT";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS_impl(int index) {
    auto arg = readarg(1);
    VertexTextureAddress(index, arg & 0xff, (arg >> 8) & 0xff);
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0_impl(int index) {
    union {
        uint32_t val;
        BitField<0, 1> enable;
        BitField<1, 5> minlod_i;
        BitField<5, 13> minlod_d;
        BitField<13, 17> maxlod_i;
        BitField<17, 25> maxlod_d;
    } arg = {readarg(1)};
    VertexTextureControl0(index,
                          arg.enable.u(),
                          arg.minlod_i.u() + (float)arg.minlod_d.u() / 255.f,
                          arg.maxlod_i.u() + (float)arg.maxlod_d.u() / 255.f);
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3_impl(int index) {
    VertexTextureControl3(index, readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER_impl(int index) {
    union {
        uint32_t val;
        BitField<19, 24> integer;
        BitField<24, 32> decimal;
    } arg = {readarg(1)};
    VertexTextureFilter(index, arg.integer.s() + (float)arg.decimal.u() / 256.f);
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT_impl(int index) {
    auto arg = readarg(1);
    VertexTextureImageRect(index, arg >> 16, arg & 0xffff);
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR_impl(int index) {
    auto c = parseColor(readarg(1));
    VertexTextureBorderColor(index, c[0], c[1], c[2], c[3]);
}

void Rsx::CELL_GCM_NV4097_SET_VIEWPORT_HORIZONTAL_impl(int index) {
    auto arg1 = readarg(1);
    auto arg2 = readarg(2);
    ViewportHorizontal(arg1 & 0xff, arg1 >> 16, arg2 & 0xff, arg2 >> 16);
}

void Rsx::CELL_GCM_NV4097_SET_VIEWPORT_VERTICAL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VIEWPORT_VERTICAL";
}

void Rsx::CELL_GCM_NV4097_SET_POINT_CENTER_MODE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_POINT_CENTER_MODE";
}

void Rsx::CELL_GCM_NV4097_ZCULL_SYNC_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_ZCULL_SYNC";
}

void Rsx::CELL_GCM_NV4097_SET_VIEWPORT_OFFSET_impl(int index) {
    ViewportOffset(union_cast<uint32_t, float>(readarg(1)),
                   union_cast<uint32_t, float>(readarg(2)),
                   union_cast<uint32_t, float>(readarg(3)),
                   union_cast<uint32_t, float>(readarg(4)),
                   union_cast<uint32_t, float>(readarg(5)),
                   union_cast<uint32_t, float>(readarg(6)),
                   union_cast<uint32_t, float>(readarg(7)),
                   union_cast<uint32_t, float>(readarg(8)));
}

void Rsx::CELL_GCM_NV4097_SET_VIEWPORT_SCALE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VIEWPORT_SCALE";
}

void Rsx::CELL_GCM_NV4097_SET_POLY_OFFSET_POINT_ENABLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_POLY_OFFSET_POINT_ENABLE";
}

void Rsx::CELL_GCM_NV4097_SET_POLY_OFFSET_LINE_ENABLE_impl(int index) {
    PolyOffsetLineEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_POLY_OFFSET_FILL_ENABLE_impl(int index) {
    PolyOffsetFillEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_DEPTH_FUNC_impl(int index) {
    DepthFunc(enum_cast<GcmOperator>(readarg(1)));
}

void Rsx::CELL_GCM_NV4097_SET_DEPTH_MASK_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_DEPTH_MASK";
}

void Rsx::CELL_GCM_NV4097_SET_DEPTH_TEST_ENABLE_impl(int index) {
    DepthTestEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR";
}

void Rsx::CELL_GCM_NV4097_SET_POLYGON_OFFSET_BIAS_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_POLYGON_OFFSET_BIAS";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA_SCALED4S_M_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_DATA_SCALED4S_M";
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_CONTROL2_impl(int index) {
    union {
        uint32_t val;
        BitField<24, 25> aniso;
        BitField<25, 26> iso;
        BitField<26, 32> slope;
    } arg = {readarg(1)};
    TextureControl2(index, arg.slope.u(), arg.iso.u(), arg.slope.u());
}

void Rsx::CELL_GCM_NV4097_SET_TEX_COORD_CONTROL_impl(int index) {
    TexCoordControl(index, readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_impl(int index) {
    assert(_currentCount <= 32);
    TransformProgram(_currentGetValue + 4, _currentCount);
}

void Rsx::CELL_GCM_NV4097_SET_SPECULAR_ENABLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SPECULAR_ENABLE";
}

void Rsx::CELL_GCM_NV4097_SET_TWO_SIDE_LIGHT_EN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_TWO_SIDE_LIGHT_EN";
}

void Rsx::CELL_GCM_NV4097_CLEAR_ZCULL_SURFACE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_CLEAR_ZCULL_SURFACE";
}

void Rsx::CELL_GCM_NV4097_SET_PERFORMANCE_PARAMS_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_PERFORMANCE_PARAMS";
}

void Rsx::CELL_GCM_NV4097_SET_FLAT_SHADE_OP_impl(int index) {
    FlatShadeOp(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_EDGE_FLAG_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_EDGE_FLAG";
}

void Rsx::CELL_GCM_NV4097_SET_USER_CLIP_PLANE_CONTROL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_USER_CLIP_PLANE_CONTROL";
}

void Rsx::CELL_GCM_NV4097_SET_POLYGON_STIPPLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_POLYGON_STIPPLE";
}

void Rsx::CELL_GCM_NV4097_SET_POLYGON_STIPPLE_PATTERN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_POLYGON_STIPPLE_PATTERN";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA3F_M_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_DATA3F_M";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_OFFSET_impl(int index) {
    for (auto i = 0; i < _currentCount; ++i, ++index) {
        union {
            uint32_t val;
            BitField<0, 1> location;
            BitField<1, 32> offset;
        } arg = {readarg(1 + i)};
        VertexDataArrayOffset(index, arg.location.u(), arg.offset.u());
    }
}

void Rsx::CELL_GCM_NV4097_INVALIDATE_VERTEX_CACHE_FILE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_INVALIDATE_VERTEX_CACHE_FILE";
}

void Rsx::CELL_GCM_NV4097_INVALIDATE_VERTEX_FILE_impl(int index) {
    // no-op
}

void Rsx::CELL_GCM_NV4097_PIPE_NOP_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_PIPE_NOP";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_OFFSET_impl(int index) {
    VertexDataBaseOffset(readarg(1), readarg(2));
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_INDEX_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_INDEX";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT_impl(int index) {
    for (auto i = 0; i < _currentCount; ++i, ++index) {
        union {
            uint32_t val;
            BitField<0, 16> frequency;
            BitField<16, 24> stride;
            BitField<24, 28> size;
            BitField<28, 32> type;
        } arg = { readarg(1 + i) };
        VertexDataArrayFormat(
            index,
            arg.frequency.u(),
            arg.stride.u(),
            arg.size.u(),
            enum_cast<VertexInputType>(arg.type.u())
        );
    }
}

void Rsx::CELL_GCM_NV4097_CLEAR_REPORT_VALUE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_CLEAR_REPORT_VALUE";
}

void Rsx::CELL_GCM_NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE";
}

void Rsx::CELL_GCM_NV4097_GET_REPORT_impl(int index) {
    auto arg = readarg(1);
    GetReport(arg >> 24, arg & 0xffffff);
}

void Rsx::CELL_GCM_NV4097_SET_ZCULL_STATS_ENABLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ZCULL_STATS_ENABLE";
}

void Rsx::CELL_GCM_NV4097_SET_BEGIN_END_impl(int index) {
    auto mode = enum_cast<GcmPrimitive>(readarg(1));
    _drawActive = (unsigned)mode;
    if (!_drawActive) {
        if (_drawArrayFirst != -1u) {
            DrawArrays(_drawArrayFirst, _drawCount);
        } else if (_drawIndexFirst != -1u) {
            DrawIndexArray(_drawIndexFirst, _drawCount);
        }
        _drawArrayFirst = -1u;
        _drawIndexFirst = -1u;
        _drawCount = 0;
    }
    BeginEnd(mode);
}

void Rsx::CELL_GCM_NV4097_ARRAY_ELEMENT16_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_ARRAY_ELEMENT16";
}

void Rsx::CELL_GCM_NV4097_ARRAY_ELEMENT32_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_ARRAY_ELEMENT32";
}

void Rsx::CELL_GCM_NV4097_DRAW_ARRAYS_impl(int index) {
    auto arg = readarg(1);
    if (_drawArrayFirst == -1u) {
        _drawArrayFirst = arg & 0xffffff;
    }
    for (auto i = 1; i <= _currentCount; ++i) {
        arg = readarg(i);
        _drawCount += (arg >> 24) + 1;
    }
}

void Rsx::CELL_GCM_NV4097_INLINE_ARRAY_impl(int index) {
    InlineArray(_currentGetValue + 4, _currentCount);
}

void Rsx::CELL_GCM_NV4097_SET_INDEX_ARRAY_ADDRESS_impl(int index) {
    if (_currentCount == 1) {
        IndexArrayAddress1(readarg(1));
    } else {
        assert(_currentCount == 2);
        auto arg2 = readarg(2);
        IndexArrayAddress(arg2 & 0xf, readarg(1), enum_cast<GcmDrawIndexArrayType>(arg2 >> 4));
    }
}

void Rsx::CELL_GCM_NV4097_SET_INDEX_ARRAY_DMA_impl(int index) {
    auto arg = readarg(1);
    IndexArrayDma(arg & 0xf, enum_cast<GcmDrawIndexArrayType>(arg >> 4));
    assert(_currentCount == 1);
}

void Rsx::CELL_GCM_NV4097_DRAW_INDEX_ARRAY_impl(int index) {
    auto arg = readarg(1);
    if (_drawIndexFirst == -1u) {
        _drawIndexFirst = arg & 0xffffff;
    }
    for (auto i = 1; i <= _currentCount; ++i) {
        arg = readarg(i);
        _drawCount += (arg >> 24) + 1;
    }
}

void Rsx::CELL_GCM_NV4097_SET_FRONT_POLYGON_MODE_impl(int index) {
    FrontPolygonMode(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_BACK_POLYGON_MODE_impl(int index) {
    BackPolygonMode(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_CULL_FACE_impl(int index) {
    CullFace(enum_cast<GcmCullFace>(readarg(1)));
}

void Rsx::CELL_GCM_NV4097_SET_FRONT_FACE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_FRONT_FACE";
}

void Rsx::CELL_GCM_NV4097_SET_POLY_SMOOTH_ENABLE_impl(int index) {
    PolySmoothEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_CULL_FACE_ENABLE_impl(int index) {
    CullFaceEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_CONTROL3_impl(int index) {
    union {
        uint32_t val;
        BitField<0, 12> depth;
        BitField<12, 32> pitch;
    } f = {readarg(1)};
    TextureControl3(index, f.depth.u(), f.pitch.u());
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA2F_M_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_DATA2F_M";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA2S_M_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_DATA2S_M";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA4UB_M_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_DATA4UB_M";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA4S_M_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_DATA4S_M";
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_OFFSET_impl(int index) {
    assert(_currentCount == 2 || _currentCount == 8);
    union {
        uint32_t val;
        BitField<8, 16> mipmap;
        BitField<16, 24> format;
        BitField<24, 28> dimension;
        BitField<28, 29> border;
        BitField<29, 30> cubemap;
        BitField<30, 32> location;
    } f = {readarg(2)};
    auto lnUnMask = (uint8_t)(GcmTextureLnUn::LN | GcmTextureLnUn::UN);
    auto formatMask = ~lnUnMask;
    auto format = enum_cast<GcmTextureFormat>(f.format.u() & formatMask);
    auto lnUn = enum_cast<GcmTextureLnUn>(f.format.u() & lnUnMask);
    TextureOffset(index,
                  readarg(1),
                  f.mipmap.u(),
                  format,
                  lnUn,
                  f.dimension.u(),
                  f.border.u(),
                  f.cubemap.u(),
                  f.location.u() - 1);
    if (_currentCount == 8) {
        parseTextureAddress(3, index);
        parseTextureControl0(4, index);
        TextureControl1(index, readarg(5));
        parseTextureFilter(6, index);
        parseTextureImageRect(7, index);
        parseTextureBorderColor(8, index);
    }
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_FORMAT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_TEXTURE_FORMAT";
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_ADDRESS_impl(int index) {
    parseTextureAddress(1, index);
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_CONTROL0_impl(int index) {
    parseTextureControl0(1, index);
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_CONTROL1_impl(int index) {
    TextureControl1(index, readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_FILTER_impl(int index) {
    parseTextureFilter(1, index);
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT_impl(int index) {
    parseTextureImageRect(1, index);
}

void Rsx::CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR_impl(int index) {
    parseTextureBorderColor(1, index);
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA4F_M_impl(int index) {
    VertexData4fM(
        index,
        union_cast<uint32_t, float>(readarg(1)),
        union_cast<uint32_t, float>(readarg(2)),
        union_cast<uint32_t, float>(readarg(3)),
        union_cast<uint32_t, float>(readarg(4))
    );
}

void Rsx::CELL_GCM_NV4097_SET_COLOR_KEY_COLOR_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_COLOR_KEY_COLOR";
}

void Rsx::CELL_GCM_NV4097_SET_SHADER_CONTROL_impl(int index) {
    auto arg = readarg(1);
    ShaderControl(arg & 0xe, (arg & 0x40) == 0, arg & 0x80, arg >> 24);
}

void Rsx::CELL_GCM_NV4097_SET_INDEXED_CONSTANT_READ_LIMITS_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_INDEXED_CONSTANT_READ_LIMITS";
}

void Rsx::CELL_GCM_NV4097_SET_SEMAPHORE_OFFSET_impl(int index) {
    SemaphoreOffset(readarg(1));
}

void Rsx::CELL_GCM_NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE_impl(int index) {
    auto value = swap02(readarg(1));
    BackEndWriteSemaphoreRelease(value);
}

void Rsx::CELL_GCM_NV4097_TEXTURE_READ_SEMAPHORE_RELEASE_impl(int index) {
    TextureReadSemaphoreRelease(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_ZMIN_MAX_CONTROL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ZMIN_MAX_CONTROL";
}

void Rsx::CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL";
}

void Rsx::CELL_GCM_NV4097_SET_SURFACE_COMPRESSION_impl(int index) {
    SurfaceCompression(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_ZCULL_EN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ZCULL_EN";
}

void Rsx::CELL_GCM_NV4097_SET_SHADER_WINDOW_impl(int index) {
    auto arg = readarg(1);
    ShaderWindow(arg & 0xfff, (arg >> 12) & 0xf, (arg >> 16) & 0xffff);
}

void Rsx::CELL_GCM_NV4097_SET_ZSTENCIL_CLEAR_VALUE_impl(int index) {
    ZStencilClearValue(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_COLOR_CLEAR_VALUE_impl(int index) {
    ColorClearValue(readarg(1));
}

void Rsx::CELL_GCM_NV4097_CLEAR_SURFACE_impl(int index) {
    ClearSurface(enum_cast<GcmClearMask>(readarg(1)));
}

void Rsx::CELL_GCM_NV4097_SET_CLEAR_RECT_HORIZONTAL_impl(int index) {
    auto arg1 = readarg(1);
    auto arg2 = readarg(2);
    ClearRectHorizontal(arg1 & 0xff, arg1 >> 16, arg2 & 0xff, arg2 >> 16);
}

void Rsx::CELL_GCM_NV4097_SET_CLEAR_RECT_VERTICAL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_CLEAR_RECT_VERTICAL";
}

void Rsx::CELL_GCM_NV4097_SET_CLIP_ID_TEST_ENABLE_impl(int index) {
    ClipIdTestEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_RESTART_INDEX_ENABLE_impl(int index) {
    RestartIndexEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_RESTART_INDEX_impl(int index) {
    RestartIndex(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_LINE_STIPPLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_LINE_STIPPLE";
}

void Rsx::CELL_GCM_NV4097_SET_LINE_STIPPLE_PATTERN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_LINE_STIPPLE_PATTERN";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_DATA1F_M_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_VERTEX_DATA1F_M";
}

void Rsx::CELL_GCM_NV4097_SET_TRANSFORM_EXECUTION_MODE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_TRANSFORM_EXECUTION_MODE";
}

void Rsx::CELL_GCM_NV4097_SET_RENDER_ENABLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_RENDER_ENABLE";
}

void Rsx::CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_LOAD_impl(int index) {
    if (_currentCount == 1) {
        TransformProgramLoad(readarg(1), readarg(1));
    } else {
        assert(_currentCount == 2);
        TransformProgramLoad(readarg(1), readarg(2));
    }
}

void Rsx::CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_START_impl(int index) {
    TransformProgramStart(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_ZCULL_CONTROL0_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ZCULL_CONTROL0";
}

void Rsx::CELL_GCM_NV4097_SET_ZCULL_CONTROL1_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ZCULL_CONTROL1";
}

void Rsx::CELL_GCM_NV4097_SET_SCULL_CONTROL_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SCULL_CONTROL";
}

void Rsx::CELL_GCM_NV4097_SET_POINT_SIZE_impl(int index) {
    PointSize(union_cast<uint32_t, float>(readarg(1)));
}

void Rsx::CELL_GCM_NV4097_SET_POINT_PARAMS_ENABLE_impl(int index) {
    PointParamsEnable(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_POINT_SPRITE_CONTROL_impl(int index) {
    auto arg = readarg(1);
    PointSpriteControl(arg & 1, (arg >> 1) & 3, enum_cast<PointSpriteTex>(arg & 0xffff00));
}

void Rsx::CELL_GCM_NV4097_SET_TRANSFORM_TIMEOUT_impl(int index) {
    auto arg = readarg(1);
    TransformTimeout(arg & 0xffff, arg >> 16);
}

void Rsx::CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD_impl(int index) {
    TransformConstantLoad(readarg(1), _currentGetValue + 4 * 2, _currentCount - 1);
}

void Rsx::CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT";
}

void Rsx::CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION_impl(int index) {
    FrequencyDividerOperation(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_ATTRIB_COLOR_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ATTRIB_COLOR";
}

void Rsx::CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD";
}

void Rsx::CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_EX_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_EX";
}

void Rsx::CELL_GCM_NV4097_SET_ATTRIB_UCLIP0_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ATTRIB_UCLIP0";
}

void Rsx::CELL_GCM_NV4097_SET_ATTRIB_UCLIP1_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_ATTRIB_UCLIP1";
}

void Rsx::CELL_GCM_NV4097_INVALIDATE_L2_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_INVALIDATE_L2";
}

void Rsx::CELL_GCM_NV4097_SET_REDUCE_DST_COLOR_impl(int index) {
    ReduceDstColor(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_NO_PARANOID_TEXTURE_FETCHES_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_NO_PARANOID_TEXTURE_FETCHES";
}

void Rsx::CELL_GCM_NV4097_SET_SHADER_PACKER_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_SHADER_PACKER";
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_ATTRIB_INPUT_MASK_impl(int index) {
    VertexAttribInputMask(enum_cast<InputMask>(readarg(1)));
}

void Rsx::CELL_GCM_NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK_impl(int index) {
    VertexAttribOutputMask(readarg(1));
}

void Rsx::CELL_GCM_NV4097_SET_TRANSFORM_BRANCH_BITS_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV4097_SET_TRANSFORM_BRANCH_BITS";
}

void Rsx::CELL_GCM_NV0039_SET_OBJECT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV0039_SET_OBJECT";
}

void Rsx::CELL_GCM_NV0039_SET_CONTEXT_DMA_NOTIFIES_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV0039_SET_CONTEXT_DMA_NOTIFIES";
}

void Rsx::CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_IN_impl(int index) {
    DmaBufferIn(readarg(1), readarg(2));
}

void Rsx::CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_OUT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_OUT";
}

void Rsx::CELL_GCM_NV0039_OFFSET_IN_impl(int index) {
    if (_currentCount == 1) {
        OffsetIn_1(readarg(1));
    } else {
        assert(_currentCount == 8);
        auto arg = readarg(7);
        OffsetIn_9(readarg(1),
                   readarg(2),
                   readarg(3),
                   readarg(4),
                   readarg(5),
                   readarg(6),
                   arg & 0xff,
                   arg >> 8,
                   readarg(8));
    }
}

void Rsx::CELL_GCM_NV0039_OFFSET_OUT_impl(int index) {
    OffsetOut(readarg(1));
}

void Rsx::CELL_GCM_NV0039_PITCH_IN_impl(int index) {
    auto arg = readarg(5);
    PitchIn(readarg(1), readarg(2), readarg(3), readarg(4), arg & 0xff, arg >> 8);
}

void Rsx::CELL_GCM_NV0039_PITCH_OUT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV0039_PITCH_OUT";
}

void Rsx::CELL_GCM_NV0039_LINE_LENGTH_IN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV0039_LINE_LENGTH_IN";
}

void Rsx::CELL_GCM_NV0039_LINE_COUNT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV0039_LINE_COUNT";
}

void Rsx::CELL_GCM_NV0039_FORMAT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV0039_FORMAT";
}

void Rsx::CELL_GCM_NV0039_BUFFER_NOTIFY_impl(int index) {
    BufferNotify(readarg(1));
}

void Rsx::CELL_GCM_NV3062_SET_OBJECT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3062_SET_OBJECT";
}

void Rsx::CELL_GCM_NV3062_SET_CONTEXT_DMA_NOTIFIES_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3062_SET_CONTEXT_DMA_NOTIFIES";
}

void Rsx::CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE";
}

void Rsx::CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN_impl(int index) {
    ContextDmaImageDestin(readarg(1));
}

void Rsx::CELL_GCM_NV3062_SET_COLOR_FORMAT_impl(int index) {
    auto format = readarg(1);
    auto arg2 = readarg(2);
    auto srcPitch = arg2 & 0xffff; (void)srcPitch;
    auto destPitch = arg2 >> 16;
    if (_currentCount == 2) {
        assert(srcPitch == destPitch);
        ColorFormat_2(format, destPitch);
    } else {
        assert(_currentCount == 4);
        if (readarg(3)) {
            assert(false);
        }
        ColorFormat_3(format, destPitch, readarg(4));
    }
}

void Rsx::CELL_GCM_NV3062_SET_PITCH_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3062_SET_PITCH";
}

void Rsx::CELL_GCM_NV3062_SET_OFFSET_SOURCE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3062_SET_OFFSET_SOURCE";
}

void Rsx::CELL_GCM_NV3062_SET_OFFSET_DESTIN_impl(int index) {
    OffsetDestin(readarg(1));
}

void Rsx::CELL_GCM_NV309E_SET_OBJECT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV309E_SET_OBJECT";
}

void Rsx::CELL_GCM_NV309E_SET_CONTEXT_DMA_NOTIFIES_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV309E_SET_CONTEXT_DMA_NOTIFIES";
}

void Rsx::CELL_GCM_NV309E_SET_CONTEXT_DMA_IMAGE_impl(int index) {
    ContextDmaImage(readarg(1));
}

void Rsx::CELL_GCM_NV309E_SET_FORMAT_impl(int index) {
    auto arg = readarg(1);
    Nv309eSetFormat(arg & 0xffff, (arg >> 16) & 0xff, (arg >> 24) & 0xff, readarg(2));
}

void Rsx::CELL_GCM_NV309E_SET_OFFSET_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV309E_SET_OFFSET";
}

void Rsx::CELL_GCM_NV308A_SET_OBJECT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_OBJECT";
}

void Rsx::CELL_GCM_NV308A_SET_CONTEXT_DMA_NOTIFIES_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_CONTEXT_DMA_NOTIFIES";
}

void Rsx::CELL_GCM_NV308A_SET_CONTEXT_COLOR_KEY_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_CONTEXT_COLOR_KEY";
}

void Rsx::CELL_GCM_NV308A_SET_CONTEXT_CLIP_RECTANGLE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_CONTEXT_CLIP_RECTANGLE";
}

void Rsx::CELL_GCM_NV308A_SET_CONTEXT_PATTERN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_CONTEXT_PATTERN";
}

void Rsx::CELL_GCM_NV308A_SET_CONTEXT_ROP_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_CONTEXT_ROP";
}

void Rsx::CELL_GCM_NV308A_SET_CONTEXT_BETA1_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_CONTEXT_BETA1";
}

void Rsx::CELL_GCM_NV308A_SET_CONTEXT_BETA4_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_CONTEXT_BETA4";
}

void Rsx::CELL_GCM_NV308A_SET_CONTEXT_SURFACE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_CONTEXT_SURFACE";
}

void Rsx::CELL_GCM_NV308A_SET_COLOR_CONVERSION_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_COLOR_CONVERSION";
}

void Rsx::CELL_GCM_NV308A_SET_OPERATION_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_OPERATION";
}

void Rsx::CELL_GCM_NV308A_SET_COLOR_FORMAT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SET_COLOR_FORMAT";
}

void Rsx::CELL_GCM_NV308A_POINT_impl(int index) {
    auto arg1 = readarg(1);
    auto arg2 = readarg(2);
    auto arg3 = readarg(3);
    Point(
        arg1 & 0xffff, arg1 >> 16,
        arg2 & 0xffff, arg2 >> 16,
        arg3 & 0xffff, arg3 >> 16
    );
}

void Rsx::CELL_GCM_NV308A_SIZE_OUT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SIZE_OUT";
}

void Rsx::CELL_GCM_NV308A_SIZE_IN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV308A_SIZE_IN";
}

void Rsx::CELL_GCM_NV308A_COLOR_impl(int index) {
    auto ptr = rsxOffsetToEa(MemoryLocation::Main, _currentGetValue + 4);
    Color(ptr, _currentCount);
}

void Rsx::CELL_GCM_NV3089_SET_OBJECT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_SET_OBJECT";
}

void Rsx::CELL_GCM_NV3089_SET_CONTEXT_DMA_NOTIFIES_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_SET_CONTEXT_DMA_NOTIFIES";
}

void Rsx::CELL_GCM_NV3089_SET_CONTEXT_DMA_IMAGE_impl(int index) {
    Nv3089ContextDmaImage(readarg(1));
}

void Rsx::CELL_GCM_NV3089_SET_CONTEXT_PATTERN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_SET_CONTEXT_PATTERN";
}

void Rsx::CELL_GCM_NV3089_SET_CONTEXT_ROP_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_SET_CONTEXT_ROP";
}

void Rsx::CELL_GCM_NV3089_SET_CONTEXT_BETA1_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_SET_CONTEXT_BETA1";
}

void Rsx::CELL_GCM_NV3089_SET_CONTEXT_BETA4_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_SET_CONTEXT_BETA4";
}

void Rsx::CELL_GCM_NV3089_SET_CONTEXT_SURFACE_impl(int index) {
    Nv3089ContextSurface(readarg(1));
}

void Rsx::CELL_GCM_NV3089_SET_COLOR_CONVERSION_impl(int index) {
    auto xy = readarg(4);
    auto hw = readarg(5);
    auto outXy = readarg(6);
    auto outHw = readarg(7);
    Nv3089SetColorConversion(readarg(1),
                             readarg(2),
                             readarg(3),
                             xy & 0xffff,
                             xy >> 16,
                             hw & 0xffff,
                             hw >> 16,
                             outXy & 0xffff,
                             outXy >> 16,
                             outHw & 0xffff,
                             outHw >> 16,
                             fixedSint32ToFloat(readarg(8)),
                             fixedSint32ToFloat(readarg(9)));
}

void Rsx::CELL_GCM_NV3089_SET_COLOR_FORMAT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_SET_COLOR_FORMAT";
}

void Rsx::CELL_GCM_NV3089_SET_OPERATION_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_SET_OPERATION";
}

void Rsx::CELL_GCM_NV3089_CLIP_POINT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_CLIP_POINT";
}

void Rsx::CELL_GCM_NV3089_CLIP_SIZE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_CLIP_SIZE";
}

void Rsx::CELL_GCM_NV3089_IMAGE_OUT_POINT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_IMAGE_OUT_POINT";
}

void Rsx::CELL_GCM_NV3089_IMAGE_OUT_SIZE_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_IMAGE_OUT_SIZE";
}

void Rsx::CELL_GCM_NV3089_DS_DX_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_DS_DX";
}

void Rsx::CELL_GCM_NV3089_DT_DY_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_DT_DY";
}

void Rsx::CELL_GCM_NV3089_IMAGE_IN_SIZE_impl(int index) {
    auto hw = readarg(1);
    auto poi = readarg(2);
    auto uv = readarg(4);
    ImageInSize(hw & 0xffff,
                hw >> 16,
                poi & 0xffff,
                poi >> 16,
                poi >> 24,
                readarg(3),
                fixedUint16ToFloat(uv & 0xffff),
                fixedUint16ToFloat(uv >> 16));
}

void Rsx::CELL_GCM_NV3089_IMAGE_IN_FORMAT_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_IMAGE_IN_FORMAT";
}

void Rsx::CELL_GCM_NV3089_IMAGE_IN_OFFSET_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_IMAGE_IN_OFFSET";
}

void Rsx::CELL_GCM_NV3089_IMAGE_IN_impl(int index) {
    WARNING(rsx) << "method not implemented: CELL_GCM_NV3089_IMAGE_IN";
}

void Rsx::CELL_GCM_DRIVER_INTERRUPT_impl(int index) {
    DriverInterrupt(readarg(1));
}

void Rsx::CELL_GCM_DRIVER_QUEUE_impl(int index) {
    DriverQueue(readarg(1));
}

void Rsx::CELL_GCM_DRIVER_FLIP_impl(int index) {
    DriverFlip(readarg(1));
}

void Rsx::CELL_GCM_unknown_240_impl(int index) { }

void Rsx::CELL_GCM_unknown_e000_impl(int index) { }
