#include "libpngdec.h"

#include "../utils.h"
#include "../IDMap.h"
#include "../Process.h"
#include "../ContentManager.h"

#include <png.h>
#include <stdio.h>

namespace {
    class PngDecoder {
    };
    
    void png_error_handler(png_structp png, png_const_charp message) {
        throw std::runtime_error(message);
    }
    
    class PngSource {
    public:
        ~PngSource() {}
        virtual void configurePng(png_struct* png) = 0;
    };
    
    class PngFileSource : public PngSource {
        FILE* _f;
    public:
        PngFileSource(FILE* f) : _f(f) {}
        
        ~PngFileSource() {
            fclose(_f);
        }
        
        void configurePng(png_struct* png) override {
            png_init_io(png, _f);
        }
    };
    
    uint32_t pngColorSpaceToCell(int space) {
        switch (space) {
            case PNG_COLOR_TYPE_GRAY: return CELL_PNGDEC_GRAYSCALE;
            case PNG_COLOR_TYPE_RGB: return CELL_PNGDEC_RGB;
            case PNG_COLOR_TYPE_PALETTE: return CELL_PNGDEC_PALETTE;
            case PNG_COLOR_TYPE_GRAY_ALPHA: return CELL_PNGDEC_GRAYSCALE_ALPHA;
            case PNG_COLOR_TYPE_RGB_ALPHA: return CELL_PNGDEC_RGBA;
            default: throw std::runtime_error("");
        }
    }
    
    class PngStream {
        png_struct* _png;
        png_info* _info;
        std::unique_ptr<PngSource> _src;
    public:
        PngStream(std::unique_ptr<PngSource> source) : _src(std::move(source)) {
            _png = png_create_read_struct(
                PNG_LIBPNG_VER_STRING, NULL, png_error_handler, NULL);
            assert(_png);
            _info = png_create_info_struct(_png);
            assert(_info);
            _src->configurePng(_png);
        }
        
        ~PngStream() {
            png_destroy_read_struct(&_png, &_info, NULL);
        }
        
        uint32_t readChunkInfo() {
            uint32_t info = 0;
            for (auto i = 0u; i < PNG_INFO_IDAT; ++i) {
                info |= png_get_valid(_png, _info, i);
            }
            return info;
        }
        
        int32_t readInfo(CellPngDecInfo* header, bool readinfo = true) {
            if (readinfo) {
                try {
                    png_read_info(_png, _info);
                } catch (...) {
                    return CELL_PNGDEC_ERROR_HEADER;
                }
            }
            header->imageWidth = png_get_image_width(_png, _info);
            header->imageHeight = png_get_image_height(_png, _info);
            header->numComponents = png_get_channels(_png, _info);
            header->colorSpace = pngColorSpaceToCell(png_get_color_type(_png, _info));
            header->bitDepth = png_get_bit_depth(_png, _info);
            header->interlaceMethod =
                png_get_interlace_type(_png, _info) == PNG_INTERLACE_ADAM7
                    ? CELL_PNGDEC_ADAM7_INTERLACE
                    : CELL_PNGDEC_NO_INTERLACE;
            header->chunkInformation = readChunkInfo();
            return CELL_OK;
        }
        
        void setParameter(const CellPngDecInParam* inParam,
                          CellPngDecOutParam* outParam) {
            assert(inParam->commandPtr == 0);
            assert(inParam->outputMode == CELL_PNGDEC_TOP_TO_BOTTOM);
            assert(inParam->outputColorSpace == CELL_PNGDEC_RGBA ||
                   inParam->outputColorSpace == CELL_PNGDEC_ARGB);
            assert(inParam->outputBitDepth == 8);
            assert(inParam->outputPackFlag == CELL_PNGDEC_1BYTE_PER_1PIXEL);
            auto colorType = png_get_color_type(_png, _info);
            auto bitDepth = png_get_bit_depth(_png, _info);
            if (inParam->outputColorSpace == CELL_PNGDEC_RGBA) {
                png_set_add_alpha(_png, 0xff, PNG_FILLER_AFTER);
            } else if (inParam->outputColorSpace == CELL_PNGDEC_ARGB) {
                png_set_add_alpha(_png, 0xff, PNG_FILLER_BEFORE);
            }
            if (colorType == PNG_COLOR_TYPE_PALETTE &&
                inParam->outputColorSpace == CELL_PNGDEC_PALETTE)
                png_set_palette_to_rgb(_png);
            if (png_get_valid(_png, _info, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(_png);
            if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8 &&
                inParam->outputBitDepth == 8)
                png_set_expand_gray_1_2_4_to_8(_png);
            png_read_update_info(_png, _info);
            CellPngDecInfo header;
            readInfo(&header, false);
            outParam->outputBitDepth = header.bitDepth;
            outParam->outputColorSpace = header.colorSpace;
            outParam->outputComponents = header.numComponents;
            outParam->outputHeight = header.imageHeight;
            outParam->outputMode = CELL_PNGDEC_TOP_TO_BOTTOM;
            outParam->outputWidth = header.imageWidth;
            outParam->outputWidthByte = png_get_rowbytes(_png, _info);
            outParam->useMemorySpace = 512;
        }
        
        void readAllRows(MainMemory* mm,
                         ps3_uintptr_t data,
                         uint64_t pitch,
                         CellPngDecDataOutInfo* dataOutInfo) {
            auto height = png_get_image_height(_png, _info);
            auto rowbytes = png_get_rowbytes(_png, _info);
            std::vector<png_byte> buf(rowbytes * height);
            std::vector<png_bytep> rows(height);
            auto ptr = &buf[0];
            for (auto& row : rows) {
                row = ptr;
                ptr += rowbytes;
            }
            png_read_image(_png, &rows[0]);
            for (auto row : rows) {
                mm->writeMemory(data, row, std::min(pitch, rowbytes));
                data += pitch;
            }
            png_read_end(_png, _info);
            dataOutInfo->chunkInformation =
                readChunkInfo() | PNG_INFO_IDAT; // idat is never set otherwise
            dataOutInfo->numText = 0;
            dataOutInfo->numUnknownChunk = 0;
            dataOutInfo->status = 0;
        }
    };
    
    ThreadSafeIDMap<CellPngDecMainHandle, std::shared_ptr<PngDecoder>> decoders;
    ThreadSafeIDMap<CellPngDecSubHandle, std::shared_ptr<PngStream>> streams;
}

int32_t cellPngDecCreate(CellPngDecMainHandle* mainHandle,
                         const CellPngDecThreadInParam* threadInParam,
                         CellPngDecThreadOutParam* threadOutParam) {
    auto decoder = std::make_shared<PngDecoder>();
    *mainHandle = decoders.create(decoder);
    return CELL_OK;
}

int32_t cellPngDecDecodeData(CellPngDecMainHandle mainHandle,
                             CellPngDecSubHandle subHandle,
                             ps3_uintptr_t data,
                             const CellPngDecDataCtrlParam* dataCtrlParam,
                             CellPngDecDataOutInfo* dataOutInfo,
                             MainMemory* mm) {
    auto stream = streams.get(subHandle);
    stream->readAllRows(mm, data, dataCtrlParam->outputBytesPerLine, dataOutInfo);
    return CELL_OK;
}

int32_t cellPngDecClose(CellPngDecMainHandle mainHandle,
                        CellPngDecSubHandle subHandle) {
    streams.destroy(subHandle);
    return CELL_OK;
}

int32_t cellPngDecDestroy(CellPngDecMainHandle mainHandle) {
    decoders.destroy(mainHandle);
    return CELL_OK;
}

int32_t cellPngDecReadHeader(CellPngDecMainHandle mainHandle,
                             CellPngDecSubHandle subHandle,
                             CellPngDecInfo* info) {
    auto stream = streams.get(subHandle);
    return stream->readInfo(info);
}

int32_t cellPngDecOpen(CellPngDecMainHandle mainHandle,
                       CellPngDecSubHandle* subHandle,
                       const CellPngDecSrc* src,
                       CellPngDecOpnInfo* openInfo,
                       Process* proc) {
    auto decoder = decoders.get(mainHandle);
    assert(src->srcSelect == CELL_PNGDEC_FILE);
    std::string fileName;
    readString(proc->mm(), src->fileName, fileName);
    auto hostPath = proc->contentManager()->toHost(fileName.c_str());
    auto f = fopen(hostPath.c_str(), "rb");
    if (!f)
        return CELL_PNGDEC_ERROR_OPEN_FILE;
    auto source = std::make_unique<PngFileSource>(f);
    auto stream = std::make_shared<PngStream>(std::move(source));
    *subHandle = streams.create(stream);
    return CELL_OK;
}

int32_t cellPngDecSetParameter(CellPngDecMainHandle mainHandle,
                               CellPngDecSubHandle subHandle,
                               const CellPngDecInParam* inParam,
                               CellPngDecOutParam* outParam) {
    auto stream = streams.get(subHandle);
    stream->setParameter(inParam, outParam);
    return 0;
}

// png_set_read_status_fn
