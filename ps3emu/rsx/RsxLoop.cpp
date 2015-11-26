#include "Rsx.h"

#include "../PPU.h"
#include "../utils.h"
#include <vector>
#include <boost/log/trivial.hpp>
#include "../../libs/graphics/graphics.h"

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
        auto offset = _ret - get;
        _ret = 0;
        return offset;
    }
    auto offset = header.offset.u();
    auto len = 4;
    const char* name = nullptr;
    switch (offset) {
        case EmuFlipCommandMethod:
            EmuFlip(readarg(1), readarg(2), readarg(3));
            break;
        case 0x00000050:
            //name = "CELL_GCM_NV406E_SET_REFERENCE";
            SetReference(readarg(1));
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
                ContextDmaColorC(readarg(1));
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
        case 0x00000904:
            name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FORMAT";
            break;
        case 0x00000a00: {
            //name = "CELL_GCM_NV4097_SET_VIEWPORT_HORIZONTAL";
            auto arg1 = readarg(1);
            auto arg2 = readarg(2);
            ViewportHorizontal(arg1 & 0xff, arg1 >> 16, arg2 & 0xff, arg2 >> 16);
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
        case 0x0000181c: {
            //name = "CELL_GCM_NV4097_SET_INDEX_ARRAY_ADDRESS";
            auto arg2 = readarg(2);
            IndexArrayAddress(arg2 & 0xf, readarg(1), arg2 >> 4);
            break;
        }
        case 0x00001820:
            name = "CELL_GCM_NV4097_SET_INDEX_ARRAY_DMA";
            break;
        case 0x00001824: {
            //name = "CELL_GCM_NV4097_DRAW_INDEX_ARRAY";
            auto arg = readarg(1);
            int indexCount = (arg >> 24) + 1;
            int veryFirst = arg & 0xff;
            for (auto i = 2u; i <= count; ++i) {
                arg = readarg(i);
                int first = arg & 0xff;
                int c = (arg >> 24) + 1;
                assert(first == 0);
                indexCount += c;
            }
            DrawIndexArray(veryFirst, indexCount);
            break;
        }
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
        case 0x00001a04:
            name = "CELL_GCM_NV4097_SET_TEXTURE_FORMAT";
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
            //name = "CELL_GCM_NV4097_SET_RESTART_INDEX_ENABLE";
            RestartIndexEnable(readarg(1));
            break;
        case 0x00001db0:
            //name = "CELL_GCM_NV4097_SET_RESTART_INDEX";
            RestartIndex(readarg(1));
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
        case 0x00001efc: {
            //name = "CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD";
            std::vector<uint32_t> vec(count - 1);
            for (auto i = 0u; i < vec.size(); ++i) {
                vec[i] = readarg(i + 2);
            }
            TransformConstantLoad(readarg(1), vec);
            break;
        }
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
                //name = "CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT";
                auto arg = readarg(1);
                TextureImageRect(index, arg >> 16, arg & 0xffff);
                break;
            }
            if (isScale(offset,
                        0x00001a1c,
                        0x20,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR";
                TextureBorderColor(index, parseColor(readarg(1)));
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
                    BitField<1, 5> minlod_i;
                    BitField<5, 13> minlod_d;
                    BitField<13, 17> maxlod_i;
                    BitField<17, 25> maxlod_d;
                    BitField<25, 28> maxaniso;
                    BitField<28, 30> alphakill;
                } arg = { readarg(1) };
                TextureControl0(
                    index,
                    arg.alphakill.u(),
                    arg.maxaniso.u(),
                    arg.maxlod_i.u() + (float)arg.maxlod_d.u() / 255.f,
                    arg.minlod_i.u() + (float)arg.minlod_d.u() / 255.f,
                    arg.enable.u()
                );
                break;
            }
            if (isScale(offset,
                        0x00001a10,
                        32,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL1";
                TextureControl1(index, readarg(1));
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
                    BitField<19, 24> biasInteger;
                    BitField<24, 32> biasDecimal;
                } arg = { readarg(1) };
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
                //name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS";
                auto arg = readarg(1);
                VertexTextureAddress(index, arg & 0xff, (arg >> 8) & 0xff);
                break;
            }
            if (isScale(offset,
                        0x0000091c,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR";
                VertexTextureBorderColor(index, parseColor(readarg(1)));
                break;
            }
            if (isScale(offset,
                        0x0000090c,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0";
                union {
                    uint32_t val;
                    BitField<0, 1> enable;
                    BitField<1, 5> minlod_i;
                    BitField<5, 13> minlod_d;
                    BitField<13, 17> maxlod_i;
                    BitField<17, 25> maxlod_d;
                } arg = { readarg(1) };
                VertexTextureControl0(
                    index, 
                    arg.enable.u(), 
                    arg.minlod_i.u() + (float)arg.minlod_d.u() / 255.f,
                    arg.maxlod_i.u() + (float)arg.maxlod_d.u() / 255.f);
                break;
            }
            if (isScale(offset,
                        0x00000914,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER";
                union {
                    uint32_t val;
                    BitField<19, 24> integer;
                    BitField<24, 32> decimal;
                } arg = { readarg(1) };
                VertexTextureFilter(index, arg.integer.s() + (float)arg.decimal.u() / 255.f);
                break;
            }
            if (isScale(offset,
                        0x00001a00,
                        32,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TEXTURE_OFFSET";
                union {
                    uint32_t val;
                    BitField<8, 16> mipmap;
                    BitField<16, 24> format;
                    BitField<24, 28> dimension;
                    BitField<28, 29> border;
                    BitField<29, 30> cubemap;
                    BitField<30, 32> location;
                } f = { readarg(2) };
                TextureOffset(index, 
                              readarg(1),
                              f.mipmap.u(),
                              f.format.u(),
                              f.dimension.u(),
                              f.border.u(),
                              f.cubemap.u(),
                              f.location.u());
                break;
            }
            if (isScale(offset,
                        0x00001840,
                        4,
                        CELL_GCM_MAX_TEXIMAGE_COUNT,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL3";
                union {
                    uint32_t val;
                    BitField<0, 12> depth;
                    BitField<12, 32> pitch;
                } f = { readarg(1) };
                TextureControl3(index, f.depth.u(), f.pitch.u());
                break;
            }
            if (isScale(offset,
                        0x00000900,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET";
                union {
                    uint32_t val;
                    BitField<8, 16> mipmap;
                    BitField<16, 24> format;
                    BitField<24, 28> dimension;
                    BitField<28, 32> location;
                } f = { readarg(2) };
                VertexTextureOffset(index, 
                                    readarg(1), 
                                    f.mipmap.u(),
                                    f.format.u(),
                                    f.dimension.u(),
                                    f.location.u());
                break;
            }
            if (isScale(offset,
                        0x00000910,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3";
                VertexTextureControl3(index, readarg(1));
                break;
            }
            if (isScale(offset,
                        0x00000918,
                        32,
                        CELL_GCM_MAX_VERTEX_TEXTURE,
                        index)) {
                //name = "CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT";
                auto arg = readarg(1);
                VertexTextureImageRect(index, arg >> 16, arg & 0xffff);
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
                //name = "CELL_GCM_NV4097_SET_TEXTURE_CONTROL2";
                union {
                    uint32_t val;
                    BitField<24, 25> aniso;
                    BitField<25, 26> iso;
                    BitField<26, 32> slope;
                } arg = { readarg(1) };
                TextureControl2(index, arg.slope.u(), arg.iso.u(), arg.slope.u());
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
                assert(count <= 32);
                TransformProgram(get + 4, count);
                break;
            }
            BOOST_LOG_TRIVIAL(fatal) << ssnprintf("illegal method offset %x", offset);
        }
    }
    if (!name)
        name = "";
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("[0x%08x: %02d%s] %s", 
        get, header.count.u(), header.prefix.u() == 2 ? "(ni)" : "", name);
    len = (header.count.u() + 1) * 4;
    assert(len != 0);
    return len;
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

uint32_t Rsx::getRef() {
    boost::unique_lock<boost::mutex> lock(_mutex);
    return _ref;
}

void Rsx::setRef(uint32_t ref) {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _ref = ref;
}

void Rsx::loop() {
    initGcm();
    BOOST_LOG_TRIVIAL(trace) << "rsx loop started, waiting for updates";
    boost::unique_lock<boost::mutex> lock(_mutex);
    for (;;) {
        _cv.wait(lock);
        BOOST_LOG_TRIVIAL(trace) << "rsx loop update received";
        if (_shutdown)
            return;
        while (_get < _put || _ret || _shutdown) {
            _get += interpret(_get);
        }
    }
}

void Rsx::shutdown() {
    if (!_initialized)
        return;
    assert(!_shutdown);
    BOOST_LOG_TRIVIAL(trace) << "waiting for rsx to shutdown";
    {
        boost::unique_lock<boost::mutex> lock(_mutex);
        _shutdown = true;
    }
    _cv.notify_all();
    _thread->join();
}
