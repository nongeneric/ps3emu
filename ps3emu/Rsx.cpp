#include "Rsx.h"

#include "PPU.h"
#include "utils.h"
#include <atomic>
#include <boost/log/trivial.hpp>

union MethodHeader {
    uint32_t val;
    BitField<0, 3> prefix;
    BitField<3, 14> count;
    BitField<14, 16> suffix;
    BitField<16, 32> offset;
    BitField<3, 32> jumpoffset;
    BitField<0, 30> calloffset;
    BitField<30, 32> callsuffix;
};

struct RsxMethodInfo {
    uint32_t offset;
    const char* name;
};

bool isScale(uint32_t value, uint32_t base, uint32_t step, uint32_t maxIndex, uint32_t& index) {
    if (value < base)
        return false;
    auto diff = value - base;
    index = diff / step;
    return diff % step == 0 && index < maxIndex;
}

int64_t Rsx::interpret(uint32_t get) {
    MethodHeader header { _ppu->load<4>(GcmLocalMemoryBase + get) };
    auto count = header.count.u();
#define readarg(x) ([=](unsigned n) {\
        assert(n <= count);\
        assert(n != 0);\
        return _ppu->load<4>(GcmLocalMemoryBase + get + 4 * n);\
    })(x)
    if (header.val == 0) {
        BOOST_LOG_TRIVIAL(trace) << "rsx nop";
        return 4;
    }
    if (header.prefix.u() == 1) {
        auto offset = header.jumpoffset.u();
        BOOST_LOG_TRIVIAL(trace) << ssnprintf("rsx jump to %x", offset);
        return offset - get;
    }
    if (header.callsuffix.u() == 2) {
        auto offset = header.calloffset.u() << 2;
        _ret = get + 4;
        BOOST_LOG_TRIVIAL(trace) << ssnprintf("rsx call to %x", offset);
        return offset - get;
    }
    if (header.val == 0x20000) {
        BOOST_LOG_TRIVIAL(trace) << ssnprintf("rsx ret to %x", _ret);
        return _ret - get;
    }
    auto offset = header.offset.u();
    auto len = 4;
    const char* name = nullptr;
    switch (offset) {
        case 0x00000050:
            name = "CELL_GCM_NV406E_SET_REFERENCE";
            break;
        case 0x00000060:
            //name = "CELL_GCM_NV406E_SET_CONTEXT_DMA_SEMAPHORE";
            ChannelSetContextDmaSemaphore(readarg(1));
            break;
        case 0x00000064:
            //name = "CELL_GCM_NV406E_SEMAPHORE_OFFSET";
            ChannelSemaphoreOffset(readarg(1));
            break;
        case 0x00000068:
            //name = "CELL_GCM_NV406E_SEMAPHORE_ACQUIRE";
            ChannelSemaphoreAcquire(readarg(1));
            break;
        case 0x0000006c:
            //name = "CELL_GCM_NV406E_SEMAPHORE_RELEASE";
            SemaphoreRelease(readarg(1));
            break;
        case 0x00000000:
            name = "CELL_GCM_NV4097_SET_OBJECT";
            break;
        case 0x00000100:
            name = "CELL_GCM_NV4097_NO_OPERATION";
            break;
        case 0x00000104:
            name = "CELL_GCM_NV4097_NOTIFY";
            break;
        case 0x00000110:
            name = "CELL_GCM_NV4097_WAIT_FOR_IDLE";
            break;
        case 0x00000140:
            name = "CELL_GCM_NV4097_PM_TRIGGER";
            break;
        case 0x00000180:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_NOTIFIES";
            break;
        case 0x00000184:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_A";
            break;
        case 0x00000188:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_B";
            break;
        case 0x0000018c:
            //name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_B";
            ContextDmaColorB(readarg(1));
            break;
        case 0x00000190:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_STATE";
            break;
        case 0x00000194:
            //name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_A";
            ContextDmaColorA(readarg(1));
            break;
        case 0x00000198:
            //name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_ZETA";
            ContextDmaZeta(readarg(1));
            break;
        case 0x0000019c:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_A";
            break;
        case 0x000001a0:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_B";
            break;
        case 0x000001a4:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_SEMAPHORE";
            break;
        case 0x000001a8:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_REPORT";
            break;
        case 0x000001ac:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_CLIP_ID";
            break;
        case 0x000001b0:
            name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_CULL_DATA";
            break;
        case 0x000001b4:
            //name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_C";
            if (count == 1) {
                ContextDmaColorC(readarg(1), readarg(1));
            } else {
                ContextDmaColorC(readarg(1), readarg(2));
            }
            break;
        case 0x000001b8:
            //name = "CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_D";
            ContextDmaColorD(readarg(1));
            break;
        case 0x00000200: {
            //name = "CELL_GCM_NV4097_SET_SURFACE_CLIP_HORIZONTAL";
            auto arg1 = readarg(1);
            auto arg2 = readarg(2);
            SurfaceClipHorizontal(arg1 & 0xff, arg1 >> 16, arg2 & 0xff, arg2 >> 16);
            break;
        }
        case 0x00000204:
            name = "CELL_GCM_NV4097_SET_SURFACE_CLIP_VERTICAL";
            break;
        case 0x00000208: {
            //name = "CELL_GCM_NV4097_SET_SURFACE_FORMAT";
            union {
                uint32_t val;
                BitField<0, 8> height;
                BitField<8, 16> width;
                BitField<16, 20> antialias;
                BitField<20, 24> type;
                BitField<24, 27> depthFormat;
                BitField<27, 32> colorFormat;
            } arg = { readarg(1) };
            SurfaceFormat(
                arg.colorFormat.u(),
                arg.depthFormat.u(),
                arg.antialias.u(),
                arg.type.u(),
                arg.width.u(),
                arg.height.u(),
                readarg(2),
                readarg(3),
                readarg(4),
                readarg(5),
                readarg(6)
            );
            break;
        }
        case 0x0000020c:
            name = "CELL_GCM_NV4097_SET_SURFACE_PITCH_A";
            break;
        case 0x00000210:
            name = "CELL_GCM_NV4097_SET_SURFACE_COLOR_AOFFSET";
            break;
        case 0x00000214:
            name = "CELL_GCM_NV4097_SET_SURFACE_ZETA_OFFSET";
            break;
        case 0x00000218:
            name = "CELL_GCM_NV4097_SET_SURFACE_COLOR_BOFFSET";
            break;
        case 0x0000021c:
            name = "CELL_GCM_NV4097_SET_SURFACE_PITCH_B";
            break;
        case 0x00000220:
            //name = "CELL_GCM_NV4097_SET_SURFACE_COLOR_TARGET";
            SurfaceColorTarget(readarg(1));
            break;
        case 0x0000022c:
            //name = "CELL_GCM_NV4097_SET_SURFACE_PITCH_Z";
            SurfacePitchZ(readarg(1));
            break;
        case 0x00000234:
            name = "CELL_GCM_NV4097_INVALIDATE_ZCULL";
            break;
        case 0x00000238:
            name = "CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP";
            break;
        case 0x0000023c:
            name = "CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP1";
            break;
        case 0x00000280:
            //name = "CELL_GCM_NV4097_SET_SURFACE_PITCH_C";
            SurfacePitchC(readarg(1), readarg(2), readarg(3), readarg(4));
            break;
        case 0x00000284:
            name = "CELL_GCM_NV4097_SET_SURFACE_PITCH_D";
            break;
        case 0x00000288:
            name = "CELL_GCM_NV4097_SET_SURFACE_COLOR_COFFSET";
            break;
        case 0x0000028c:
            name = "CELL_GCM_NV4097_SET_SURFACE_COLOR_DOFFSET";
            break;
        case 0x000002b8: {
            //name = "CELL_GCM_NV4097_SET_WINDOW_OFFSET";
            auto arg1 = readarg(1);
            WindowOffset(arg1 & 0xff, arg1 >> 16);
            break;
        }
        case 0x000002bc:
            name = "CELL_GCM_NV4097_SET_WINDOW_CLIP_TYPE";
            break;
        case 0x000002c0:
            name = "CELL_GCM_NV4097_SET_WINDOW_CLIP_HORIZONTAL";
            break;
        case 0x000002c4:
            name = "CELL_GCM_NV4097_SET_WINDOW_CLIP_VERTICAL";
            break;
        case 0x00000300:
            name = "CELL_GCM_NV4097_SET_DITHER_ENABLE";
            break;
        case 0x00000304:
            //name = "CELL_GCM_NV4097_SET_ALPHA_TEST_ENABLE";
            AlphaTestEnable(readarg(1));
            break;
        case 0x00000308:
            //name = "CELL_GCM_NV4097_SET_ALPHA_FUNC";
            AlphaFunc(readarg(1), readarg(2));
            break;
        case 0x0000030c:
            name = "CELL_GCM_NV4097_SET_ALPHA_REF";
            break;
        case 0x00000310:
            name = "CELL_GCM_NV4097_SET_BLEND_ENABLE";
            break;
        case 0x00000314:
            name = "CELL_GCM_NV4097_SET_BLEND_FUNC_SFACTOR";
            break;
        case 0x00000318:
            name = "CELL_GCM_NV4097_SET_BLEND_FUNC_DFACTOR";
            break;
        case 0x0000031c:
            name = "CELL_GCM_NV4097_SET_BLEND_COLOR";
            break;
        case 0x00000320:
            name = "CELL_GCM_NV4097_SET_BLEND_EQUATION";
            break;
        case 0x00000324:
            //name = "CELL_GCM_NV4097_SET_COLOR_MASK";
            ColorMask(readarg(1));
            break;
        case 0x00000328:
            name = "CELL_GCM_NV4097_SET_STENCIL_TEST_ENABLE";
            break;
        case 0x0000032c:
            name = "CELL_GCM_NV4097_SET_STENCIL_MASK";
            break;
        case 0x00000330:
            name = "CELL_GCM_NV4097_SET_STENCIL_FUNC";
            break;
        case 0x00000334:
            name = "CELL_GCM_NV4097_SET_STENCIL_FUNC_REF";
            break;
        case 0x00000338:
            name = "CELL_GCM_NV4097_SET_STENCIL_FUNC_MASK";
            break;
        case 0x0000033c:
            name = "CELL_GCM_NV4097_SET_STENCIL_OP_FAIL";
            break;
        case 0x00000340:
            name = "CELL_GCM_NV4097_SET_STENCIL_OP_ZFAIL";
            break;
        case 0x00000344:
            name = "CELL_GCM_NV4097_SET_STENCIL_OP_ZPASS";
            break;
        case 0x00000348:
            name = "CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE";
            break;
        case 0x0000034c:
            name = "CELL_GCM_NV4097_SET_BACK_STENCIL_MASK";
            break;
        case 0x00000350:
            name = "CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC";
            break;
        case 0x00000354:
            name = "CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_REF";
            break;
        case 0x00000358:
            name = "CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_MASK";
            break;
        case 0x0000035c:
            name = "CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL";
            break;
        case 0x00000360:
            name = "CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZFAIL";
            break;
        case 0x00000364:
            name = "CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZPASS";
            break;
        case 0x00000368:
            //name = "CELL_GCM_NV4097_SET_SHADE_MODE";
            ShadeMode(readarg(1));
            break;
        case 0x0000036c:
            name = "CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT";
            break;
        case 0x00000370:
            name = "CELL_GCM_NV4097_SET_COLOR_MASK_MRT";
            break;
        case 0x00000374:
            name = "CELL_GCM_NV4097_SET_LOGIC_OP_ENABLE";
            break;
        case 0x00000378:
            name = "CELL_GCM_NV4097_SET_LOGIC_OP";
            break;
        case 0x0000037c:
            name = "CELL_GCM_NV4097_SET_BLEND_COLOR2";
            break;
        case 0x00000380:
            name = "CELL_GCM_NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE";
            break;
        case 0x00000384:
            name = "CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MIN";
            break;
        case 0x00000388:
            name = "CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MAX";
            break;
        case 0x00000394:
            //name = "CELL_GCM_NV4097_SET_CLIP_MIN";
            ClipMin(
                union_cast<uint32_t, float>(readarg(1)),
                union_cast<uint32_t, float>(readarg(2))
            );
            break;
        case 0x00000398:
            name = "CELL_GCM_NV4097_SET_CLIP_MAX";
            break;
        case 0x000003b0:
            //name = "CELL_GCM_NV4097_SET_CONTROL0";
            Control0(readarg(1));
            break;
        case 0x000003b8:
            name = "CELL_GCM_NV4097_SET_LINE_WIDTH";
            break;
        case 0x000003bc:
            name = "CELL_GCM_NV4097_SET_LINE_SMOOTH_ENABLE";
            break;
        case 0x000008c0:
            name = "CELL_GCM_NV4097_SET_SCISSOR_HORIZONTAL";
            break;
        case 0x000008c4:
            name = "CELL_GCM_NV4097_SET_SCISSOR_VERTICAL";
            break;
        case 0x000008cc:
            name = "CELL_GCM_NV4097_SET_FOG_MODE";
            FogMode(readarg(1));
            break;
        case 0x000008d0:
            name = "CELL_GCM_NV4097_SET_FOG_PARAMS";
            break;
        case 0x000008e4:
            //name = "CELL_GCM_NV4097_SET_SHADER_PROGRAM";
            ShaderProgram(readarg(1));
            break;
        case 0x00000900:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET";
            break;
        case 0x00000904:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FORMAT";
            break;
        case 0x00000908:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS";
            break;
        case 0x0000090c:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0";
            break;
        case 0x00000910:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3";
            break;
        case 0x00000914:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER";
            break;
        case 0x00000918:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT";
            break;
        case 0x0000091c:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR";
            break;
        case 0x00000a00: {
            //name = "CELL_GCM_NV4097_SET_VIEWPORT_HORIZONTAL";
            auto arg1 = readarg(1);
            auto arg2 = readarg(2);
            SurfaceClipHorizontal(arg1 & 0xff, arg1 >> 16, arg2 & 0xff, arg2 >> 16);
            break;
        }
        case 0x00000a04:
            name = "CELL_GCM_NV4097_SET_VIEWPORT_VERTICAL";
            break;
        case 0x00000a0c:
            name = "CELL_GCM_NV4097_SET_POINT_CENTER_MODE";
            break;
        case 0x00000a1c:
            name = "CELL_GCM_NV4097_ZCULL_SYNC";
            break;
        case 0x00000a20: {
            //name = "CELL_GCM_NV4097_SET_VIEWPORT_OFFSET";
            ViewportOffset(
                union_cast<uint32_t, float>(readarg(1)),
                union_cast<uint32_t, float>(readarg(2)),
                union_cast<uint32_t, float>(readarg(3)),
                union_cast<uint32_t, float>(readarg(4)),
                union_cast<uint32_t, float>(readarg(5)),
                union_cast<uint32_t, float>(readarg(6)),
                union_cast<uint32_t, float>(readarg(7)),
                union_cast<uint32_t, float>(readarg(8))
            );
            break;
        }
        case 0x00000a30:
            name = "CELL_GCM_NV4097_SET_VIEWPORT_SCALE";
            break;
        case 0x00000a60:
            name = "CELL_GCM_NV4097_SET_POLY_OFFSET_POINT_ENABLE";
            break;
        case 0x00000a64:
            name = "CELL_GCM_NV4097_SET_POLY_OFFSET_LINE_ENABLE";
            break;
        case 0x00000a68:
            name = "CELL_GCM_NV4097_SET_POLY_OFFSET_FILL_ENABLE";
            break;
        case 0x00000a6c:
            //name = "CELL_GCM_NV4097_SET_DEPTH_FUNC";
            DepthFunc(readarg(1));
            break;
        case 0x00000a70:
            name = "CELL_GCM_NV4097_SET_DEPTH_MASK";
            break;
        case 0x00000a74:
            //name = "CELL_GCM_NV4097_SET_DEPTH_TEST_ENABLE";
            DepthTestEnable(readarg(1));
            break;
        case 0x00000a78:
            name = "CELL_GCM_NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR";
            break;
        case 0x00000a7c:
            name = "CELL_GCM_NV4097_SET_POLYGON_OFFSET_BIAS";
            break;
        case 0x00000a80:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA_SCALED4S_M";
            break;
        case 0x00001428:
            name = "CELL_GCM_NV4097_SET_SPECULAR_ENABLE";
            break;
        case 0x0000142c:
            name = "CELL_GCM_NV4097_SET_TWO_SIDE_LIGHT_EN";
            break;
        case 0x00001438:
            name = "CELL_GCM_NV4097_CLEAR_ZCULL_SURFACE";
            break;
        case 0x00001450:
            name = "CELL_GCM_NV4097_SET_PERFORMANCE_PARAMS";
            break;
        case 0x00001454:
            //name = "CELL_GCM_NV4097_SET_FLAT_SHADE_OP";
            FlatShadeOp(readarg(1));
            break;
        case 0x0000145c:
            name = "CELL_GCM_NV4097_SET_EDGE_FLAG";
            break;
        case 0x00001478:
            name = "CELL_GCM_NV4097_SET_USER_CLIP_PLANE_CONTROL";
            break;
        case 0x0000147c:
            name = "CELL_GCM_NV4097_SET_POLYGON_STIPPLE";
            break;
        case 0x00001480:
            name = "CELL_GCM_NV4097_SET_POLYGON_STIPPLE_PATTERN";
            break;
        case 0x00001500:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA3F_M";
            break;
        case 0x00001710:
            name = "CELL_GCM_NV4097_INVALIDATE_VERTEX_CACHE_FILE";
            break;
        case 0x00001714:
            name = "CELL_GCM_NV4097_INVALIDATE_VERTEX_FILE";
            break;
        case 0x00001718:
            name = "CELL_GCM_NV4097_PIPE_NOP";
            break;
        case 0x00001738:
            //name = "CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_OFFSET";
            VertexDataBaseOffset(readarg(1), readarg(2));
            break;
        case 0x0000173c:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_INDEX";
            break;
        case 0x000017c8:
            name = "CELL_GCM_NV4097_CLEAR_REPORT_VALUE";
            break;
        case 0x000017cc:
            name = "CELL_GCM_NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE";
            break;
        case 0x00001800:
            name = "CELL_GCM_NV4097_GET_REPORT";
            break;
        case 0x00001804:
            name = "CELL_GCM_NV4097_SET_ZCULL_STATS_ENABLE";
            break;
        case 0x00001808:
            //name = "CELL_GCM_NV4097_SET_BEGIN_END";
            BeginEnd(readarg(1));
            break;
        case 0x0000180c:
            name = "CELL_GCM_NV4097_ARRAY_ELEMENT16";
            break;
        case 0x00001810:
            name = "CELL_GCM_NV4097_ARRAY_ELEMENT32";
            break;
        case 0x00001814: {
            //name = "CELL_GCM_NV4097_DRAW_ARRAYS";
            auto arg = readarg(1);
            DrawArrays(arg & 0xfffff, arg >> 24);
            break;
        }
        case 0x00001818:
            name = "CELL_GCM_NV4097_INLINE_ARRAY";
            break;
        case 0x0000181c:
            name = "CELL_GCM_NV4097_SET_INDEX_ARRAY_ADDRESS";
            break;
        case 0x00001820:
            name = "CELL_GCM_NV4097_SET_INDEX_ARRAY_DMA";
            break;
        case 0x00001824:
            name = "CELL_GCM_NV4097_DRAW_INDEX_ARRAY";
            break;
        case 0x00001828:
            name = "CELL_GCM_NV4097_SET_FRONT_POLYGON_MODE";
            break;
        case 0x0000182c:
            name = "CELL_GCM_NV4097_SET_BACK_POLYGON_MODE";
            break;
        case 0x00001830:
            name = "CELL_GCM_NV4097_SET_CULL_FACE";
            break;
        case 0x00001834:
            name = "CELL_GCM_NV4097_SET_FRONT_FACE";
            break;
        case 0x00001838:
            name = "CELL_GCM_NV4097_SET_POLY_SMOOTH_ENABLE";
            break;
        case 0x0000183c:
            //name = "CELL_GCM_NV4097_SET_CULL_FACE_ENABLE";
            CullFaceEnable(readarg(1));
            break;
        case 0x00001840:
            name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL3";
            break;
        case 0x00001880:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA2F_M";
            break;
        case 0x00001900:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA2S_M";
            break;
        case 0x00001940:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA4UB_M";
            break;
        case 0x00001980:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA4S_M";
            break;
        case 0x00001a00:
            name = "CELL_GCM_NV4097_SET_TEXTURE_OFFSET";
            break;
        case 0x00001a04:
            name = "CELL_GCM_NV4097_SET_TEXTURE_FORMAT";
            break;
        case 0x00001a10:
            name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL1";
            break;
        case 0x00001c00:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA4F_M";
            break;
        case 0x00001d00:
            name = "CELL_GCM_NV4097_SET_COLOR_KEY_COLOR";
            break;
        case 0x00001d60: {
            //name = "CELL_GCM_NV4097_SET_SHADER_CONTROL";
            auto arg = readarg(1);
            ShaderControl(arg & 0xfff, arg >> 24);
            break;
        }
        case 0x00001d64:
            name = "CELL_GCM_NV4097_SET_INDEXED_CONSTANT_READ_LIMITS";
            break;
        case 0x00001d6c:
            name = "CELL_GCM_NV4097_SET_SEMAPHORE_OFFSET";
            break;
        case 0x00001d70:
            name = "CELL_GCM_NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE";
            break;
        case 0x00001d74:
            name = "CELL_GCM_NV4097_TEXTURE_READ_SEMAPHORE_RELEASE";
            break;
        case 0x00001d78:
            name = "CELL_GCM_NV4097_SET_ZMIN_MAX_CONTROL";
            break;
        case 0x00001d7c:
            name = "CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL";
            break;
        case 0x00001d80:
            name = "CELL_GCM_NV4097_SET_SURFACE_COMPRESSION";
            SurfaceCompression(readarg(1));
            break;
        case 0x00001d84:
            name = "CELL_GCM_NV4097_SET_ZCULL_EN";
            break;
        case 0x00001d88: {
            //name = "CELL_GCM_NV4097_SET_SHADER_WINDOW";
            auto arg = readarg(1);
            ShaderWindow(arg & 0xfff, (arg >> 12) & 0xf, (arg >> 16) && 0xffff);
            break;
        }
        case 0x00001d8c:
            name = "CELL_GCM_NV4097_SET_ZSTENCIL_CLEAR_VALUE";
            break;
        case 0x00001d90:
            //name = "CELL_GCM_NV4097_SET_COLOR_CLEAR_VALUE";
            ColorClearValue(readarg(1));
            break;
        case 0x00001d94:
            //name = "CELL_GCM_NV4097_CLEAR_SURFACE";
            ClearSurface(readarg(1));
            break;
        case 0x00001d98: {
            //name = "CELL_GCM_NV4097_SET_CLEAR_RECT_HORIZONTAL";
            auto arg1 = readarg(1);
            auto arg2 = readarg(2);
            ClearRectHorizontal(arg1 & 0xff, arg1 >> 16, arg2 & 0xff, arg2 >> 16);
            break;
        }
        case 0x00001d9c:
            name = "CELL_GCM_NV4097_SET_CLEAR_RECT_VERTICAL";
            break;
        case 0x00001da4:
            name = "CELL_GCM_NV4097_SET_CLIP_ID_TEST_ENABLE";
            ClipIdTestEnable(readarg(1));
            break;
        case 0x00001dac:
            name = "CELL_GCM_NV4097_SET_RESTART_INDEX_ENABLE";
            break;
        case 0x00001db0:
            name = "CELL_GCM_NV4097_SET_RESTART_INDEX";
            break;
        case 0x00001db4:
            name = "CELL_GCM_NV4097_SET_LINE_STIPPLE";
            break;
        case 0x00001db8:
            name = "CELL_GCM_NV4097_SET_LINE_STIPPLE_PATTERN";
            break;
        case 0x00001e40:
            name = "CELL_GCM_NV4097_SET_VERTEX_DATA1F_M";
            break;
        case 0x00001e94:
            name = "CELL_GCM_NV4097_SET_TRANSFORM_EXECUTION_MODE";
            break;
        case 0x00001e98:
            name = "CELL_GCM_NV4097_SET_RENDER_ENABLE";
            break;
        case 0x00001e9c:
            //name = "CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_LOAD";
            if (count == 1) {
                TransformProgramLoad(readarg(1), readarg(1));
            } else {
                assert(count == 2);
                TransformProgramLoad(readarg(1), readarg(2));
            }
            break;
        case 0x00001ea0:
            name = "CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_START";
            break;
        case 0x00001ea4:
            name = "CELL_GCM_NV4097_SET_ZCULL_CONTROL0";
            break;
        case 0x00001ea8:
            name = "CELL_GCM_NV4097_SET_ZCULL_CONTROL1";
            break;
        case 0x00001eac:
            name = "CELL_GCM_NV4097_SET_SCULL_CONTROL";
            break;
        case 0x00001ee0:
            name = "CELL_GCM_NV4097_SET_POINT_SIZE";
            break;
        case 0x00001ee4:
            name = "CELL_GCM_NV4097_SET_POINT_PARAMS_ENABLE";
            break;
        case 0x00001ee8:
            name = "CELL_GCM_NV4097_SET_POINT_SPRITE_CONTROL";
            break;
        case 0x00001ef8: {
            //name = "CELL_GCM_NV4097_SET_TRANSFORM_TIMEOUT";
            auto arg = readarg(1);
            TransformTimeout(arg & 0xffff, arg >> 16);
            break;
        }
        case 0x00001efc:
            name = "CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD";
            break;
        case 0x00001f00:
            name = "CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT";
            break;
        case 0x00001fc0:
            name = "CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION";
            FrequencyDividerOperation(readarg(1));
            break;
        case 0x00001fc4:
            name = "CELL_GCM_NV4097_SET_ATTRIB_COLOR";
            break;
        case 0x00001fc8:
            name = "CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD";
            break;
        case 0x00001fcc:
            name = "CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_EX";
            break;
        case 0x00001fd0:
            name = "CELL_GCM_NV4097_SET_ATTRIB_UCLIP0";
            break;
        case 0x00001fd4:
            name = "CELL_GCM_NV4097_SET_ATTRIB_UCLIP1";
            break;
        case 0x00001fd8:
            name = "CELL_GCM_NV4097_INVALIDATE_L2";
            break;
        case 0x00001fe0:
            //name = "CELL_GCM_NV4097_SET_REDUCE_DST_COLOR";
            ReduceDstColor(readarg(1));
            break;
        case 0x00001fe8:
            name = "CELL_GCM_NV4097_SET_NO_PARANOID_TEXTURE_FETCHES";
            break;
        case 0x00001fec:
            name = "CELL_GCM_NV4097_SET_SHADER_PACKER";
            break;
        case 0x00001ff0:
            //name = "CELL_GCM_NV4097_SET_VERTEX_ATTRIB_INPUT_MASK";
            VertexAttribInputMask(readarg(1));
            break;
        case 0x00001ff4:
            //name = "CELL_GCM_NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK";
            VertexAttribOutputMask(readarg(1));
            break;
        case 0x00001ff8:
            name = "CELL_GCM_NV4097_SET_TRANSFORM_BRANCH_BITS";
            break;
        case 0x00002000:
            name = "CELL_GCM_NV0039_SET_OBJECT";
            break;
        case 0x00002180:
            name = "CELL_GCM_NV0039_SET_CONTEXT_DMA_NOTIFIES";
            break;
        case 0x00002184:
            name = "CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_IN";
            break;
        case 0x00002188:
            name = "CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_OUT";
            break;
        case 0x0000230C:
            name = "CELL_GCM_NV0039_OFFSET_IN";
            break;
        case 0x00002310:
            name = "CELL_GCM_NV0039_OFFSET_OUT";
            break;
        case 0x00002314:
            name = "CELL_GCM_NV0039_PITCH_IN";
            break;
        case 0x00002318:
            name = "CELL_GCM_NV0039_PITCH_OUT";
            break;
        case 0x0000231C:
            name = "CELL_GCM_NV0039_LINE_LENGTH_IN";
            break;
        case 0x00002320:
            name = "CELL_GCM_NV0039_LINE_COUNT";
            break;
        case 0x00002324:
            name = "CELL_GCM_NV0039_FORMAT";
            break;
        case 0x00002328:
            name = "CELL_GCM_NV0039_BUFFER_NOTIFY";
            break;
        case 0x00006000:
            name = "CELL_GCM_NV3062_SET_OBJECT";
            break;
        case 0x00006180:
            name = "CELL_GCM_NV3062_SET_CONTEXT_DMA_NOTIFIES";
            break;
        case 0x00006184:
            name = "CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE";
            break;
        case 0x00006188:
            name = "CELL_GCM_NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN";
            break;
        case 0x00006300:
            name = "CELL_GCM_NV3062_SET_COLOR_FORMAT";
            break;
        case 0x00006304:
            name = "CELL_GCM_NV3062_SET_PITCH";
            break;
        case 0x00006308:
            name = "CELL_GCM_NV3062_SET_OFFSET_SOURCE";
            break;
        case 0x0000630C:
            name = "CELL_GCM_NV3062_SET_OFFSET_DESTIN";
            break;
        case 0x00008000:
            name = "CELL_GCM_NV309E_SET_OBJECT";
            break;
        case 0x00008180:
            name = "CELL_GCM_NV309E_SET_CONTEXT_DMA_NOTIFIES";
            break;
        case 0x00008184:
            name = "CELL_GCM_NV309E_SET_CONTEXT_DMA_IMAGE";
            break;
        case 0x00008300:
            name = "CELL_GCM_NV309E_SET_FORMAT";
            break;
        case 0x00008304:
            name = "CELL_GCM_NV309E_SET_OFFSET";
            break;
        case 0x0000A000:
            name = "CELL_GCM_NV308A_SET_OBJECT";
            break;
        case 0x0000A180:
            name = "CELL_GCM_NV308A_SET_CONTEXT_DMA_NOTIFIES";
            break;
        case 0x0000A184:
            name = "CELL_GCM_NV308A_SET_CONTEXT_COLOR_KEY";
            break;
        case 0x0000A188:
            name = "CELL_GCM_NV308A_SET_CONTEXT_CLIP_RECTANGLE";
            break;
        case 0x0000A18C:
            name = "CELL_GCM_NV308A_SET_CONTEXT_PATTERN";
            break;
        case 0x0000A190:
            name = "CELL_GCM_NV308A_SET_CONTEXT_ROP";
            break;
        case 0x0000A194:
            name = "CELL_GCM_NV308A_SET_CONTEXT_BETA1";
            break;
        case 0x0000A198:
            name = "CELL_GCM_NV308A_SET_CONTEXT_BETA4";
            break;
        case 0x0000A19C:
            name = "CELL_GCM_NV308A_SET_CONTEXT_SURFACE";
            break;
        case 0x0000A2F8:
            name = "CELL_GCM_NV308A_SET_COLOR_CONVERSION";
            break;
        case 0x0000A2FC:
            name = "CELL_GCM_NV308A_SET_OPERATION";
            break;
        case 0x0000A300:
            name = "CELL_GCM_NV308A_SET_COLOR_FORMAT";
            break;
        case 0x0000A304:
            name = "CELL_GCM_NV308A_POINT";
            break;
        case 0x0000A308:
            name = "CELL_GCM_NV308A_SIZE_OUT";
            break;
        case 0x0000A30C:
            name = "CELL_GCM_NV308A_SIZE_IN";
            break;
        case 0x0000A400:
            name = "CELL_GCM_NV308A_COLOR";
            break;
        case 0x0000C000:
            name = "CELL_GCM_NV3089_SET_OBJECT";
            break;
        case 0x0000C180:
            name = "CELL_GCM_NV3089_SET_CONTEXT_DMA_NOTIFIES";
            break;
        case 0x0000C184:
            name = "CELL_GCM_NV3089_SET_CONTEXT_DMA_IMAGE";
            break;
        case 0x0000C188:
            name = "CELL_GCM_NV3089_SET_CONTEXT_PATTERN";
            break;
        case 0x0000C18C:
            name = "CELL_GCM_NV3089_SET_CONTEXT_ROP";
            break;
        case 0x0000C190:
            name = "CELL_GCM_NV3089_SET_CONTEXT_BETA1";
            break;
        case 0x0000C194:
            name = "CELL_GCM_NV3089_SET_CONTEXT_BETA4";
            break;
        case 0x0000C198:
            name = "CELL_GCM_NV3089_SET_CONTEXT_SURFACE";
            break;
        case 0x0000C2FC:
            name = "CELL_GCM_NV3089_SET_COLOR_CONVERSION";
            break;
        case 0x0000C300:
            name = "CELL_GCM_NV3089_SET_COLOR_FORMAT";
            break;
        case 0x0000C304:
            name = "CELL_GCM_NV3089_SET_OPERATION";
            break;
        case 0x0000C308:
            name = "CELL_GCM_NV3089_CLIP_POINT";
            break;
        case 0x0000C30C:
            name = "CELL_GCM_NV3089_CLIP_SIZE";
            break;
        case 0x0000C310:
            name = "CELL_GCM_NV3089_IMAGE_OUT_POINT";
            break;
        case 0x0000C314:
            name = "CELL_GCM_NV3089_IMAGE_OUT_SIZE";
            break;
        case 0x0000C318:
            name = "CELL_GCM_NV3089_DS_DX";
            break;
        case 0x0000C31C:
            name = "CELL_GCM_NV3089_DT_DY";
            break;
        case 0x0000C400:
            name = "CELL_GCM_NV3089_IMAGE_IN_SIZE";
            break;
        case 0x0000C404:
            name = "CELL_GCM_NV3089_IMAGE_IN_FORMAT";
            break;
        case 0x0000C408:
            name = "CELL_GCM_NV3089_IMAGE_IN_OFFSET";
            break;
        case 0x0000C40C:
            name = "CELL_GCM_NV3089_IMAGE_IN";
            break;
        case 0x0000EB00:
            name = "CELL_GCM_DRIVER_INTERRUPT";
            break;
        case 0x0000E944:
            name = "CELL_GCM_DRIVER_QUEUE";
            break;
        case 0x0000E924:
            name = "CELL_GCM_DRIVER_FLIP";
            break;
        case 0x240:
            name = "unknown_240";
            break;
        case 0xe000:
            name = "unknown_e000";
            break;
        default: {
            uint32_t index;
            if (isScale(offset,
                        0x00001a18,
                        32,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                name = "CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT";
                break;
            }
            if (isScale(offset,
                        0x00001a1c,
                        0x20,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR";
                TextureBorderColor(index, readarg(1));
                break;
            }
            if (isScale(offset,
                        0x00001a0c,
                        0x20,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL0";
                union {
                    uint32_t val;
                    BitField<0, 1> enable;
                    BitField<1, 13> minlod;
                    BitField<13, 25> maxlod;
                    BitField<25, 28> maxaniso;
                    BitField<28, 30> alphakill;
                } arg = { readarg(1) };
                TextureControl0(
                    index,
                    arg.enable.u(),
                    arg.minlod.u(),
                    arg.maxlod.u(),
                    arg.maxaniso.u(),
                    arg.alphakill.u()
                );
                break;
            }
            if (isScale(offset,
                        0x00001a10,
                        32,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL1";
                break;
            }
            if (isScale(offset,
                        0x00001a14,
                        0x20,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TEXTURE_FILTER";
                union {
                    uint32_t val;
                    BitField<0, 1> bs;
                    BitField<1, 2> gs;
                    BitField<2, 3> rs;
                    BitField<3, 4> as;
                    BitField<4, 8> mag;
                    BitField<8, 16> min;
                    BitField<16, 19> conv;
                    BitField<19, 32> bias;
                } arg = { readarg(1) };
                TextureFilter(
                    index,
                    arg.bias.u(),
                    arg.min.u(),
                    arg.mag.u(),
                    arg.conv.u(),
                    arg.as.u(),
                    arg.rs.u(),
                    arg.gs.u(),
                    arg.bs.u()
                );
                break;
            }
            if (isScale(offset,
                        0x00001a08,
                        0x20,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TEXTURE_ADDRESS";
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
                } arg = { readarg(1) };
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
                break;
            }
            if (isScale(offset,
                        0x00001680,
                        4,
                        16,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_OFFSET";
                union {
                    uint32_t val;
                    BitField<0, 1> location;
                    BitField<1, 32> offset;
                } arg = { readarg(1) };
                VertexDataArrayOffset(
                    index,
                    arg.location.u(),
                    arg.offset.u()
                );
                break;
            }
            if (isScale(offset,
                        0x00001740,
                        4,
                        16,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT";
                union {
                    uint32_t val;
                    BitField<0, 16> frequency;
                    BitField<16, 24> stride;
                    BitField<24, 28> size;
                    BitField<28, 32> type;
                } arg = { readarg(1) };
                VertexDataArrayFormat(
                    index,
                    arg.frequency.u(),
                    arg.stride.u(),
                    arg.size.u(),
                    arg.type.u()
                );
                break;
            }
            if (isScale(offset,
                        0x00000908,
                        32,
                        4,
                        index)) {
                name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS";
                break;
            }
            if (isScale(offset,
                        0x0000091c,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR";
                break;
            }
            if (isScale(offset,
                        0x0000090c,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0";
                break;
            }
            if (isScale(offset,
                        0x00000914,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER";
                break;
            }
            if (isScale(offset,
                        0x00001a00,
                        32,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                name = "CELL_GCM_NV4097_SET_TEXTURE_OFFSET";
                break;
            }
            if (isScale(offset,
                        0x00001840,
                        4,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL3";
                break;
            }
            if (isScale(offset,
                        0x00000900,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET";
                break;
            }
            if (isScale(offset,
                        0x00000910,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3";
                break;
            }
            if (isScale(offset,
                        0x00000918,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT";
                break;
            }
            if (isScale(offset,
                        0x00000b40,
                        4,
                        CELL_GCM_MAX_VERTEX_TEXTURE, // conf->texCoordsInputMask
                        index)) {
                name = "CELL_GCM_NV4097_SET_TEX_COORD_CONTROL";
                TexCoordControl(index, readarg(1));
                break;
            }
            if (isScale(offset,
                        0x00000b00,
                        4,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL2";
                TextureControl2(index, readarg(1));
                break;
            }
            if (isScale(offset,
                        0x000003c0,
                        4,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_ANISO_SPREAD";
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
                break;
            }
            if (isScale(offset,
                        0x00000b80,
                        16,
                        1, // ?
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM";
                TransformProgram(readarg(1), readarg(2), readarg(3), readarg(4));
                break;
            }
            BOOST_LOG_TRIVIAL(fatal) << ssnprintf("illegal method offset %x", offset);
        }
    }
    if (name) {
        BOOST_LOG_TRIVIAL(trace) << ssnprintf("[0x%08x: %02d%s] %s", 
            get, header.count.u(), header.prefix.u() == 2 ? "(ni)" : "", name);
    }
    len = (header.count.u() + 1) * 4;
    return len;
}

enum class SurfaceColorLocation {
    Main, Local
};

enum class DepthFunc {
    Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always
};

enum class ShadeMode {
    Flat, Smooth
};

enum class DepthFormat {
    Fixed, Float
};

enum class SurfaceDepthFormat {
    z16, z24s8
};

class RsxContext {
public:
    SurfaceColorLocation surfaceColorLocation;
    SurfaceColorLocation surfaceDepthLocation;
    unsigned surfaceWidth;
    unsigned surfaceHeight;
    unsigned surfaceColorPitch[4];
    unsigned surfaceColorOffset[4];
    unsigned surfaceDepthPitch;
    unsigned surfaceDepthOffset;
    unsigned surfaceWindowOriginX;
    unsigned surfaceWindowOriginY;
    DepthFormat depthFormat;
    SurfaceDepthFormat surfaceDepthFormat;
    unsigned viewPortX;
    unsigned viewPortY;
    unsigned viewPortWidth;
    unsigned viewPortHeight;
    float clipMin;
    float clipMax;
    float viewPortScale[4];
    float viewPortOffset[4];
    uint32_t colorMask;
    bool isDepthTestEnabled;
    DepthFunc depthFunc;
    bool isCullFaceEnabled;
    ShadeMode shadeMode;
    uint32_t colorClearValue;
    uint32_t clearSurfaceMask;
    bool isFlipInProgress = false;
};

Rsx::Rsx(PPU* ppu) : _ppu(ppu) {
    _context.reset(new RsxContext());
    _thread.reset(new boost::thread([=]{ loop(); }));
}

void Rsx::setPut(uint32_t put) {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _put = put;
    _cv.notify_all();
}

void Rsx::setGet(uint32_t get) {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _get = get;
    _cv.notify_all();
}

void Rsx::loop() {
    BOOST_LOG_TRIVIAL(trace) << "rsx loop started, waiting for updates";
    boost::unique_lock<boost::mutex> lock(_mutex);
    for (;;) {
        _cv.wait(lock);
        BOOST_LOG_TRIVIAL(trace) << "rsx loop update recieved";
        if (_shutdown) {
            BOOST_LOG_TRIVIAL(trace) << "rsx loop shutdown";
            return;
        }
        while (_get < _put) {
            auto get = _get;
            lock.unlock();
            auto len = interpret(get);
            lock.lock();
            _get += len;
        }
    }
}

void Rsx::shutdown() {
    assert(!_shutdown);
    BOOST_LOG_TRIVIAL(trace) << "waiting for rsx to shutdown";
    {
        boost::unique_lock<boost::mutex> lock(_mutex);
        _shutdown = true;
    }
    _cv.notify_all();
    _thread->join();
}

Rsx::~Rsx() {
    shutdown();
}

void Rsx::setLabel(int index, uint32_t value) {
    auto offset = index * 0x10;
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("setting rsx label at offset %x", offset);
    auto ptr = _ppu->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
    auto atomic = (std::atomic<uint32_t>*)ptr;
    atomic->store(boost::endian::native_to_big(value));
}

void Rsx::ChannelSetContextDmaSemaphore(uint32_t handle) {
    _activeSemaphoreHandle = handle;
}

void Rsx::ChannelSemaphoreOffset(uint32_t offset) {
    _semaphores[_activeSemaphoreHandle] = offset;
}

void Rsx::ChannelSemaphoreAcquire(uint32_t value) {
    auto offset = _semaphores[_activeSemaphoreHandle];
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("acquiring semaphore %x at offset %x with value %x",
        _activeSemaphoreHandle, offset, value
    );
    auto ptr = _ppu->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
    auto atomic = (std::atomic<uint32_t>*)ptr;
    while (boost::endian::big_to_native(atomic->load()) != value) ;
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("acquired");
}

void Rsx::SemaphoreRelease(uint32_t value) {
    auto offset = _semaphores[_activeSemaphoreHandle];
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("releasing semaphore %x at offset %x with value %x",
        _activeSemaphoreHandle, offset, value
    );
    auto ptr = _ppu->getMemoryPointer(GcmLabelBaseOffset + offset, sizeof(uint32_t));
    auto atomic = (std::atomic<uint32_t>*)ptr;
    atomic->store(boost::endian::native_to_big(value));
}

void Rsx::SurfaceClipHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfaceClipHorizontal(%d, %d, %d, %d)", x, w, y, h);
    _context->viewPortX = x;
    _context->viewPortY = y;
    _context->viewPortWidth = w;
    _context->viewPortHeight = h;
}

void Rsx::SurfacePitchC(uint32_t pitchC, uint32_t pitchD, uint32_t offsetC, uint32_t offsetD) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfacePitchC(%d, %d, %d, %d)",
        pitchC, pitchD, offsetC, offsetD);
    _context->surfaceColorPitch[2] = pitchC;
    _context->surfaceColorPitch[3] = pitchD;
    _context->surfaceColorOffset[2] = offsetC;
    _context->surfaceColorOffset[3] = offsetD;
}

void Rsx::SurfaceCompression(uint32_t x) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfaceCompression(%d)", x);
}

void Rsx::WindowOffset(uint16_t x, uint16_t y) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("WindowOffset(%d, %d)", x, y);
    _context->surfaceWindowOriginX = x;
    _context->surfaceWindowOriginY = y;
}

void Rsx::ClearRectHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ClearRectHorizontal(%d, %d, %d, %d)", x, w, y, h);
}

void Rsx::ClipIdTestEnable(uint32_t x) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ClipIdTestEnable(%d)", x);
}

void Rsx::Control0(uint32_t x) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("Control0(%x)", x);
    if (x & 0x00100000) {
        auto depthFormat = (x & ~0x00100000) >> 12;
        if (depthFormat == CELL_GCM_DEPTH_FORMAT_FIXED) {
            BOOST_LOG_TRIVIAL(trace) << "CELL_GCM_DEPTH_FORMAT_FIXED";
        } else if (depthFormat == CELL_GCM_DEPTH_FORMAT_FLOAT) {
            BOOST_LOG_TRIVIAL(trace) << "CELL_GCM_DEPTH_FORMAT_FLOAT";
        } else {
            assert(false);
        }
    } else {
        BOOST_LOG_TRIVIAL(error) << "unknown control0";
    }
}

void Rsx::FlatShadeOp(uint32_t x) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("FlatShadeOp(%d)", x);
}

void Rsx::VertexAttribOutputMask(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexAttribOutputMask(%x)", mask);
}

void Rsx::FrequencyDividerOperation(uint16_t op) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("FrequencyDividerOperation(%x)", op);
}

void Rsx::TexCoordControl(unsigned int index, uint32_t control) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TexCoordControl(%d, %x)", index, control);
}

void Rsx::ShaderWindow(uint16_t height, uint8_t origin, uint16_t pixelCenters) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ShaderWindow(%d, %x, %x)", height, origin, pixelCenters);
    assert(origin == CELL_GCM_WINDOW_ORIGIN_BOTTOM);
    assert(pixelCenters == CELL_GCM_WINDOW_PIXEL_CENTER_HALF);
}

void Rsx::ReduceDstColor(bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ReduceDstColor(%d)", enable);
}

void Rsx::TextureControl2(unsigned int index, uint32_t control) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureControl2(%d, %x)", index, control);
}

void Rsx::FogMode(uint32_t mode) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureControl2(%x)", mode);
}

void Rsx::AnisoSpread(unsigned int index,
                      bool reduceSamplesEnable,
                      bool hReduceSamplesEnable,
                      bool vReduceSamplesEnable,
                      uint8_t spacingSelect,
                      uint8_t hSpacingSelect,
                      uint8_t vSpacingSelect) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("AnisoSpread(%d, ...)", index);
}

void Rsx::VertexDataBaseOffset(uint32_t baseOffset, uint32_t baseIndex) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexDataBaseOffset(%x, %x)", baseOffset, baseIndex);
}

void Rsx::AlphaFunc(uint32_t af, uint32_t ref) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("AlphaFunc(%x, %x)", af, ref);
}

void Rsx::AlphaTestEnable(bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("AlphaTestEnable(%d)", enable);
}

void Rsx::TextureAddress(unsigned index,
                         uint8_t wraps,
                         uint8_t wrapt,
                         uint8_t wrapr,
                         uint8_t unsignedRemap,
                         uint8_t zfunc,
                         uint8_t gamma,
                         uint8_t anisoBias,
                         uint8_t signedRemap) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureAddress(%d, ...)", index);
}

void Rsx::TextureBorderColor(unsigned index, uint32_t color) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureBorderColor(%d, %x)", index, color);
}

void Rsx::TextureControl0(unsigned index,
                          uint8_t alphaKill,
                          uint8_t maxaniso,
                          uint16_t maxlod,
                          uint16_t minlod,
                          bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureControl0(%d, ...)", index);
}

void Rsx::TextureFilter(unsigned index,
                        uint16_t bias,
                        uint8_t min,
                        uint8_t mag,
                        uint8_t conv,
                        uint8_t as,
                        uint8_t rs,
                        uint8_t gs,
                        uint8_t bs) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TextureFilter(%d, ...)", index);
}

void Rsx::ShaderControl(uint32_t control, uint8_t registerCount) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ShaderControl(%x, %d)", control, registerCount);
}

void Rsx::TransformProgramLoad(uint32_t load, uint32_t start) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TransformProgramLoad(%x, %x)", load, start);
}

void Rsx::TransformProgram(uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TransformProgram(%x, %x, %x, %x)", d0, d1, d2, d3);
}

void Rsx::VertexAttribInputMask(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexAttribInputMask(%x)", mask);
}

void Rsx::TransformTimeout(uint16_t count, uint16_t registerCount) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("TransformTimeout(%x, %d)", count, registerCount);
}

void Rsx::ShaderProgram(uint32_t locationOffset) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ShaderProgram(%x)", locationOffset);
}

void Rsx::ViewportHorizontal(uint16_t x, uint16_t w, uint16_t y, uint16_t h) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ViewportHorizontal(%d, %d, %d, %d)", x, w, y, h);
}

void Rsx::ClipMin(float min, float max) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ClipMin(%e, %e)", min, max);
    _context->clipMin = min;
    _context->clipMax = max;
}

void Rsx::ViewportOffset(float offset0,
                         float offset1,
                         float offset2,
                         float offset3,
                         float scale0,
                         float scale1,
                         float scale2,
                         float scale3) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ViewportOffset(%e, ...)", offset0);
    _context->viewPortOffset[0] = offset0;
    _context->viewPortOffset[1] = offset1;
    _context->viewPortOffset[2] = offset2;
    _context->viewPortOffset[3] = offset3;
    _context->viewPortScale[0] = scale0;
    _context->viewPortScale[1] = scale1;
    _context->viewPortScale[2] = scale2;
    _context->viewPortScale[3] = scale3;
}

void Rsx::ContextDmaColorA(uint32_t context) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorA(%x)", context);
    setSurfaceColorLocation(context);
}

void Rsx::ContextDmaColorB(uint32_t context) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorB(%x)", context);
    setSurfaceColorLocation(context);
}

void Rsx::ContextDmaColorC(uint32_t contextC, uint32_t contextD) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorC(%x, %x)", contextC, contextD);
    setSurfaceColorLocation(contextC);
    setSurfaceColorLocation(contextD);
}

void Rsx::ContextDmaColorD(uint32_t context) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaColorD(%x)", context);
    setSurfaceColorLocation(context);
}

void Rsx::ContextDmaZeta(uint32_t context) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ContextDmaZeta(%x)", context);
    assert(context - CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER == CELL_GCM_LOCATION_LOCAL); // only local is supported
    _context->surfaceDepthLocation = SurfaceColorLocation::Local;
}

void Rsx::SurfaceFormat(uint8_t colorFormat,
                        uint8_t depthFormat,
                        uint8_t antialias,
                        uint8_t type,
                        uint8_t width,
                        uint8_t height,
                        uint32_t pitchA,
                        uint32_t offsetA,
                        uint32_t offsetZ,
                        uint32_t offsetB,
                        uint32_t pitchB) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfaceFormat(%x, ...)", colorFormat);
    assert(colorFormat == CELL_GCM_SURFACE_A8R8G8B8);
    if (depthFormat == CELL_GCM_SURFACE_Z16) {
        _context->surfaceDepthFormat = SurfaceDepthFormat::z16;
    } else if (depthFormat == CELL_GCM_SURFACE_Z24S8) {
        _context->surfaceDepthFormat = SurfaceDepthFormat::z24s8;
    } else {
        assert(false);
    }
    assert(antialias == CELL_GCM_SURFACE_CENTER_1);
    assert(type == CELL_GCM_SURFACE_PITCH);
    _context->surfaceWidth = 1 << (width + 1);
    _context->surfaceHeight = 1 << (height + 1);
    assert(_context->surfaceWidth == 2048);
    assert(_context->surfaceHeight == 1024);
    _context->surfaceColorPitch[0] = pitchA;
    _context->surfaceColorPitch[1] = pitchB;
    _context->surfaceColorOffset[0] = offsetA;
    _context->surfaceColorOffset[1] = offsetB;
    _context->surfaceDepthOffset = offsetZ;
}

void Rsx::SurfacePitchZ(uint32_t pitch) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfacePitchZ(%d)", pitch);
    _context->surfaceDepthPitch = pitch;
}

void Rsx::SurfaceColorTarget(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("SurfaceColorTarget(%x)", mask);
    assert(mask == CELL_GCM_SURFACE_TARGET_0);
}

void Rsx::ColorMask(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ColorMask(%x)", mask);
    _context->colorMask = mask;
}

void Rsx::DepthTestEnable(bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("DepthTestEnable(%d)", enable);
    _context->isDepthTestEnabled = enable;
}

void Rsx::DepthFunc(uint32_t zf) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("DepthFunc(%d)", zf);
    assert(zf == CELL_GCM_LESS);
    _context->depthFunc = ::DepthFunc::Less;
}

void Rsx::CullFaceEnable(bool enable) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("CullFaceEnable(%d)", enable);
    assert(!enable);
    _context->isCullFaceEnabled = enable;
}

void Rsx::ShadeMode(uint32_t sm) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ShadeMode(%x)", sm);
    if (sm == CELL_GCM_SMOOTH) {
        _context->shadeMode = ::ShadeMode::Smooth;
    } else if (sm == CELL_GCM_FLAT) {
        _context->shadeMode = ::ShadeMode::Flat;
    } else {
        assert(false);
    }
}

void Rsx::ColorClearValue(uint32_t color) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ColorClearValue(%x)", color);
    _context->colorClearValue = color;
}

void Rsx::ClearSurface(uint32_t mask) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("ClearSurface(%x)", mask);
    _context->clearSurfaceMask = mask;
}

void Rsx::setSurfaceColorLocation(uint32_t context) {
    // all color locations must be the same, so A=B=C=D
    context -= CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
    assert(context == CELL_GCM_LOCATION_LOCAL);
    if (context == CELL_GCM_LOCATION_LOCAL) {
        _context->surfaceColorLocation = SurfaceColorLocation::Local;   
    } else if (context == CELL_GCM_LOCATION_MAIN) {
        _context->surfaceColorLocation = SurfaceColorLocation::Main;
    } else {
        assert(false);
    }
}

void Rsx::VertexDataArrayFormat(uint8_t index,
                                uint16_t frequency,
                                uint8_t stride,
                                uint8_t size,
                                uint8_t type) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexDataArrayFormat(%x, %x, %x, %x, %x)",
        index, frequency, stride, size, type);
}

void Rsx::VertexDataArrayOffset(unsigned index, uint8_t location, uint32_t offset) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("VertexDataArrayOffset(%x, %x, %x)",
        index, location, offset);
}

void Rsx::BeginEnd(uint32_t mode) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("BeginEnd(%x)", mode);
}

void Rsx::DrawArrays(unsigned first, unsigned count) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("DrawArrays(%d, %d)", first, count);
}

bool Rsx::isFlipInProgress() const {
    boost::unique_lock<boost::mutex> lock(_mutex);
    return _context->isFlipInProgress;
}

void Rsx::resetFlipStatus() {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _context->isFlipInProgress = true;
}