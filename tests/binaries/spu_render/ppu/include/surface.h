#ifndef PPU_SURFACE_H
#define PPU_SURFACE_H

#include <cell/gcm.h>

namespace Render{

enum {
	RENDER_OK = CELL_OK,
	RENDER_FAILURE = -1,
};

struct ColorSurface{
	uint8_t type;
	uint8_t location; // CELL_GCM_LOCATION_LOCAL or CELL_GCM_LOCALTION_MAIN
	uint8_t format;
	uint8_t antialias; //CELL_GCM_SURFACE_CENTER_1 CELL_GCM_SURFACE_DIAGONAL_CENTERED_2 CELL_GCM_SURFACE_SQUARE_CENTERED_4 CELL_GCM_SURFACE_SQUARE_ROTATED_4
	uint16_t width;
	uint16_t height;
	uint32_t offset;
	uint32_t pitch;
	void* address;
};

struct DepthSurface{
	uint8_t type;
	uint8_t location; // CELL_GCM_LOCATION_LOCAL or CELL_GCM_LOCALTION_MAIN
	uint8_t format; // CELL_GCM_SURFACE_Z16 or CELL_GCM_SURFACE_Z24S8
	uint8_t antialias; //CELL_GCM_SURFACE_CENTER_1 CELL_GCM_SURFACE_DIAGONAL_CENTERED_2 CELL_GCM_SURFACE_SQUARE_CENTERED_4 CELL_GCM_SURFACE_SQUARE_ROTATED_4
	uint16_t width;
	uint16_t height;
	uint32_t offset;
	uint32_t pitch;
	void* address;
};

int32_t getColorFormatSize(uint32_t format);
int32_t getDepthFormatSize(uint32_t format);
int32_t createColorBuffer(ColorSurface& sf,
						  uint32_t type,
						  uint32_t width,
						  uint32_t height,
						  uint32_t format,
						  uint32_t location,
						  uint32_t antialias,
						  bool isTiled,
						  uint32_t comp_mode,
						  uint32_t bank);
int32_t createDepthSurface(DepthSurface& dep,
						   uint32_t type,
						   uint32_t width,
						   uint32_t height,
						   uint32_t format,
						   uint32_t location,
						   uint32_t antialias,
						   bool isTiled,
						   uint32_t comp_mode,
						   uint32_t bank,
						   bool isZcull);
int32_t bindDepthColorSurface(const ColorSurface* col0, const DepthSurface* dep,
							uint16_t x, uint16_t y, CellGcmSurface& sf);
int32_t getColorSurfaceAsTexture(const ColorSurface& col,CellGcmTexture& tex,uint32_t normalize);
int32_t getDepthSurfaceAsTexture(const DepthSurface& dep,CellGcmTexture& tex,uint32_t normalize);
}; // namespace Render

#endif // PPU_SURFACE_H
