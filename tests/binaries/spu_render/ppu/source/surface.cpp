#include <cell/gcm.h>
#include "memory.h"
#include "util.h"
#include "remap.h"
#include "surface.h"
#include "debug.h"

namespace Render{

uint32_t TileIndexBit = 0;
uint32_t ZcullIndexBit = 0;
uint32_t compression_tag = 0;
const uint32_t COMPRESSION_TAG_SIZE = 0x10000;

static int8_t getZcullIndex(){
	for(int32_t i=0; i < CELL_GCM_INDEX_RANGE_ZCULL_COUNT; i++){
		if((ZcullIndexBit & (0x1 << i)) == 0){
			ZcullIndexBit |= (0x1 << i);
			return i;
		}
	}
	MY_ASSERT(false);
	return RENDER_FAILURE;
}

static int8_t getTileIndex(){
	for(int32_t i=0; i < CELL_GCM_INDEX_RANGE_TILE_COUNT; i++){
		if((TileIndexBit & (0x1 << i)) == 0){
			TileIndexBit |= (0x1 << i);
			return i;
		}
	}
	MY_ASSERT(false);
	return RENDER_FAILURE;
}

#if 0 // These functions are not used in this project.
static void releaseZcullIndex(int8_t index){
	ZcullIndexBit &= ~(0x0001 << index);
}

static void releaseTileIndex(int8_t index){
	TileIndexBit &= ~(0x0001 << index);
}
#endif 

int16_t getCompressionTag(uint32_t size){
	int16_t ret_val = compression_tag;
	int32_t update_tag = compression_tag + size / COMPRESSION_TAG_SIZE;
	if(update_tag > CELL_GCM_ZCULL_COMPRESSION_TAG_BASE_MAX){
		MY_ASSERT(false);
		return RENDER_FAILURE;
	}
	else{
		compression_tag = update_tag;
		return ret_val;
	}
}

static inline int32_t GetMSAASize(uint32_t antialias,int32_t& msaa_w,int32_t& msaa_h){
	msaa_w = 1;
	msaa_h = 1;

	switch (antialias){
		case CELL_GCM_SURFACE_CENTER_1:
			msaa_w = 1;
			msaa_h = 1;
			break;
		case CELL_GCM_SURFACE_DIAGONAL_CENTERED_2:
			msaa_w = 2;
			msaa_h = 1;
			break;
		case CELL_GCM_SURFACE_SQUARE_CENTERED_4:
		case CELL_GCM_SURFACE_SQUARE_ROTATED_4:
			msaa_w = 2;
			msaa_h = 2;
		default:
			msaa_w = 1;
			msaa_h = 1;
	}
	return CELL_OK;
}

int32_t bindDepthColorSurface(const ColorSurface* col0,
							  const DepthSurface* dep,
							  uint16_t x, uint16_t y,
							  CellGcmSurface& sf)
{
	if(col0 == NULL && dep == NULL)
		return RENDER_FAILURE;
	if(col0 != NULL){
		sf.type = col0->type;
		sf.colorTarget = CELL_GCM_SURFACE_TARGET_0;
		sf.antialias = col0->antialias;
		sf.colorFormat = col0->format;
		sf.colorLocation[0] = sf.colorLocation[1] = sf.colorLocation[2] = sf.colorLocation[3] = col0->location;
		sf.colorOffset[0] = col0->offset;
		sf.colorPitch[0] = col0->pitch;
		sf.width = col0->width;
		sf.height = col0->height;
	}
	else{ // Depth Only
		sf.type = dep->type;
		sf.colorTarget = CELL_GCM_SURFACE_TARGET_NONE;
		sf.antialias = CELL_GCM_SURFACE_CENTER_1;
		sf.colorFormat = CELL_GCM_SURFACE_A8R8G8B8;
		sf.colorLocation[0] = sf.colorLocation[1] = sf.colorLocation[2] = sf.colorLocation[3] = CELL_GCM_LOCATION_LOCAL;
		sf.width = dep->width;
		sf.height = dep->height;
		sf.colorOffset[0] = 0;
		sf.colorPitch[0] = 64;
	}
	sf.colorOffset[1] = sf.colorOffset[2] = sf.colorOffset[3] = 0;
	sf.colorPitch[1] = sf.colorPitch[2] = sf.colorPitch[3] = 64;

	if(dep != NULL){
		sf.depthFormat = dep->format;
		sf.depthLocation = dep->location;
		sf.depthOffset = dep->offset;
		sf.depthPitch = dep->pitch;
	}
	else{
		sf.depthFormat = CELL_GCM_SURFACE_Z24S8;
		sf.depthLocation = CELL_GCM_LOCATION_LOCAL;
		sf.depthOffset = 0;
		sf.depthPitch = 64;
	}
	sf.x = x;
	sf.y = y;
	return CELL_OK;
}


int32_t GetDepthFormatSize(uint32_t format){
	if(format == CELL_GCM_SURFACE_Z16)
		return 2;
	else if(format == CELL_GCM_SURFACE_Z24S8)
		return 4;
	else{
		MY_ASSERT(0);
		return RENDER_FAILURE;
	}
}

int32_t getColorFormatSize(uint32_t format){
	switch(format){
		case CELL_GCM_SURFACE_B8:
			return 1;
		case CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5:
		case CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5:
		case CELL_GCM_SURFACE_R5G6B5:
		case CELL_GCM_SURFACE_G8B8:
			return 2;
		case CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8:
		case CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8:
		case CELL_GCM_SURFACE_A8R8G8B8:
		case CELL_GCM_SURFACE_F_X32:
		case CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8:
		case CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8:
		case CELL_GCM_SURFACE_A8B8G8R8:
			return 4;
		case CELL_GCM_SURFACE_F_W16Z16Y16X16:
			return 8;
		case CELL_GCM_SURFACE_F_W32Z32Y32X32:
			return 16;
		default:
			MY_ASSERT(0);
			return RENDER_FAILURE;
	}
}

int32_t createColorBuffer(ColorSurface& sf,
						  uint32_t type,
						  uint32_t width,
						  uint32_t height,
						  uint32_t format,
						  uint32_t location,
						  uint32_t antialias,
						  bool isTiled,
						  uint32_t comp_mode,
						  uint32_t bank)
{
	MY_ASSERT( (isTiled == false) || (isTiled == true && type == CELL_GCM_SURFACE_PITCH) );
	sf.type = type;
	sf.location = location;
	sf.antialias = antialias;
	sf.format = format;
	sf.width = width;
	sf.height = height;
	int32_t msaa_w,msaa_h;
	GetMSAASize(antialias,msaa_w,msaa_h);
	uint32_t color_size;
	Sys::Memory::VramHeap& heap = (location == CELL_GCM_LOCATION_LOCAL) 
		? Sys::Memory::getLocalMemoryHeap() : Sys::Memory::getMappedMainMemoryHeap();
	// Calculate buffer size
	if(isTiled){
		sf.pitch = cellGcmGetTiledPitchSize(sf.width * getColorFormatSize(sf.format) * msaa_w);
		uint32_t height_round = (location == CELL_GCM_LOCATION_LOCAL) ? CELL_GCM_TILE_LOCAL_ALIGN_HEIGHT : CELL_GCM_TILE_MAIN_ALIGN_HEIGHT;
		color_size = ALIGN(sf.pitch * ALIGN(sf.height,height_round), CELL_GCM_TILE_ALIGN_SIZE);
		// "size" of Tiled Region should be multiples of 64K bytes.
		sf.address = heap.alloc(color_size,CELL_GCM_TILE_ALIGN_SIZE);
	}
	else{
		sf.pitch = ALIGN(64, (sf.width * getColorFormatSize(sf.format) * msaa_w));
		color_size = sf.pitch * sf.height;
		uint32_t align = (type == CELL_GCM_SURFACE_SWIZZLE) ? CELL_GCM_SURFACE_SWIZZLE_ALIGN_OFFSET : CELL_GCM_SURFACE_LINEAR_ALIGN_OFFSET;
		sf.address = heap.alloc(color_size,align);
	}
	sf.offset = heap.AddressToOffset(sf.address);
	// Bind Tile Region
	if(isTiled){
		int32_t tile_index = getTileIndex();
		if(tile_index != RENDER_FAILURE){
			int32_t comp_tag = (comp_mode != CELL_GCM_COMPMODE_DISABLED) ? getCompressionTag(color_size) : 0;
			if(comp_tag != RENDER_FAILURE){
				MY_C(cellGcmSetTileInfo(tile_index, CELL_GCM_LOCATION_LOCAL, sf.offset,color_size,sf.pitch, comp_mode, comp_tag, bank));
				MY_C(cellGcmBindTile(tile_index));
			}
			else
				return RENDER_FAILURE;
		}
		else
			return RENDER_FAILURE;

	}
	return CELL_OK;
}

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
						   bool isZcull)
{
	dep.type = type;
	dep.location = location;
	dep.format = format;
	dep.antialias = antialias;
	dep.width = width;
	dep.height = height;
	int32_t msaa_h,msaa_w;
	GetMSAASize(antialias,msaa_w,msaa_h);
	uint32_t height_round = (location == CELL_GCM_LOCATION_LOCAL) ? CELL_GCM_TILE_LOCAL_ALIGN_HEIGHT : CELL_GCM_TILE_MAIN_ALIGN_HEIGHT;
	uint32_t depth_size;
	Sys::Memory::VramHeap& heap = (location == CELL_GCM_LOCATION_LOCAL) 
		? Sys::Memory::getLocalMemoryHeap() : Sys::Memory::getMappedMainMemoryHeap();

	if(isTiled){
		dep.pitch = cellGcmGetTiledPitchSize(width * GetDepthFormatSize(format) * msaa_w);
		depth_size = ALIGN(dep.pitch * ALIGN(height,height_round),CELL_GCM_TILE_ALIGN_SIZE);
		dep.address = heap.alloc(depth_size,CELL_GCM_TILE_ALIGN_SIZE);
	}
	else{
		dep.pitch = width * GetDepthFormatSize(format) * msaa_w;
		depth_size = dep.pitch * height;
		uint32_t align = (type == CELL_GCM_SURFACE_SWIZZLE) ? CELL_GCM_SURFACE_SWIZZLE_ALIGN_OFFSET : CELL_GCM_SURFACE_LINEAR_ALIGN_OFFSET;
		dep.address = heap.alloc(depth_size,align);
	}
	dep.offset = heap.AddressToOffset(dep.address);
	// Tile configure
	bool isTileReady = false;
	if(isTiled){
		int32_t tile_index = getTileIndex();
		if(tile_index != RENDER_FAILURE){
			int32_t comp_tag = (comp_mode != CELL_GCM_COMPMODE_DISABLED) ? getCompressionTag(depth_size) : 0;
			if(comp_tag != RENDER_FAILURE){
				MY_C(cellGcmSetTileInfo(tile_index, CELL_GCM_LOCATION_LOCAL, dep.offset,depth_size,dep.pitch, comp_mode, comp_tag, bank));
				MY_C(cellGcmBindTile(tile_index));
				isTileReady = true;
			}
			else
				return RENDER_FAILURE;
		}
		else
			return RENDER_FAILURE;
	}
	// Zcull configure
	if(isZcull && isTileReady){
		int32_t zcull_index = getZcullIndex();
		if(zcull_index != RENDER_FAILURE){
			MY_C(cellGcmBindZcull(zcull_index, dep.offset,
				ALIGN(dep.width,CELL_GCM_ZCULL_ALIGN_WIDTH), ALIGN(dep.height,CELL_GCM_ZCULL_ALIGN_HEIGHT),
				0, // We adapt Zcull start addres from 0. Because we use Zcull Reload.
				format,
				antialias,
				CELL_GCM_ZCULL_LESS,
				CELL_GCM_ZCULL_LONES,
				CELL_GCM_SCULL_SFUNC_ALWAYS,
				0x80, 0xff));
		}
	}
	return CELL_OK;
}

int32_t getColorSurfaceAsTexture(const ColorSurface& col,CellGcmTexture& tex,uint32_t normalize)
{
	int32_t msaa_w,msaa_h;
	GetMSAASize(col.antialias,msaa_w,msaa_h);
	uint32_t format;
	uint32_t remap;
	switch(col.format){
		case CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5:
			format = CELL_GCM_TEXTURE_D1R5G5B5;
			remap = TEXTURE_REMAP_XRGB_XRGB(CELL_GCM_TEXTURE_REMAP_ZERO);
			break;
		case CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5:
			format = CELL_GCM_TEXTURE_D1R5G5B5;
			remap = TEXTURE_REMAP_XRGB_XRGB(CELL_GCM_TEXTURE_REMAP_ONE);
		case CELL_GCM_SURFACE_R5G6B5:
			format = CELL_GCM_TEXTURE_R5G6B5;
			remap = TEXTURE_REMAP_XRGB_XRGB(CELL_GCM_TEXTURE_REMAP_ZERO);
			break;
		case CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8:
			format = CELL_GCM_TEXTURE_D8R8G8B8;
			remap = TEXTURE_REMAP_XRGB_XRGB(CELL_GCM_TEXTURE_REMAP_ZERO);
			break;
		case CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8:
			format = CELL_GCM_TEXTURE_D8R8G8B8;
			remap = TEXTURE_REMAP_XRGB_XRGB(CELL_GCM_TEXTURE_REMAP_ONE);
			break;
		case CELL_GCM_SURFACE_A8R8G8B8:
			format = CELL_GCM_TEXTURE_A8R8G8B8;
			remap = TEXTURE_REMAP_ARGB_ARGB;
			break;
		case CELL_GCM_SURFACE_B8:
			format = CELL_GCM_TEXTURE_B8;
			remap = TEXTURE_REMAP_ARGB_ARGB;
			break;
		case CELL_GCM_SURFACE_G8B8:
			format = CELL_GCM_TEXTURE_G8B8;
			remap = TEXTURE_REMAP_ARGB_ARGB;
			break;
		case CELL_GCM_SURFACE_F_W16Z16Y16X16:
			format = CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;
			remap = TEXTURE_REMAP_ARGB_ARGB;
			break;
		case CELL_GCM_SURFACE_F_W32Z32Y32X32:
			format = CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT;
			remap = TEXTURE_REMAP_ARGB_ARGB;
			break;
		case CELL_GCM_SURFACE_F_X32:
			format = CELL_GCM_TEXTURE_X32_FLOAT;
			remap = TEXTURE_REMAP_X32_XXXX;
			break;
		case CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8:
			format = CELL_GCM_TEXTURE_D8R8G8B8;
			remap = TEXTURE_REMAP_XBGR_XRGB(CELL_GCM_TEXTURE_REMAP_ZERO);
			break;
		case CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8:
			format = CELL_GCM_TEXTURE_D8R8G8B8;
			remap = TEXTURE_REMAP_XBGR_XRGB(CELL_GCM_TEXTURE_REMAP_ONE);
			break;
		case CELL_GCM_SURFACE_A8B8G8R8:
			format = CELL_GCM_TEXTURE_A8R8G8B8;
			remap = TEXTURE_REMAP_ABGR_ARGB;
			break;
		default:
			format = 0;
			remap = 0;
			MY_ASSERT(0);
	}

	uint32_t swizzle = (col.type == CELL_GCM_SURFACE_PITCH) ? CELL_GCM_TEXTURE_LN : CELL_GCM_TEXTURE_SZ;
	tex.format = format | swizzle | normalize;
	tex.mipmap = 1;
	tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	tex.cubemap = CELL_GCM_FALSE;
	tex.remap = remap;
	tex.depth = 1;
	tex.width = col.width * msaa_w;
	tex.height = col.height * msaa_h;
	tex.pitch = col.pitch;
	tex.offset = col.offset;
	tex.location = col.location;
	return CELL_OK;
}

int32_t getDepthSurfaceAsTexture(const DepthSurface& dep,CellGcmTexture& tex,uint32_t normalize)
{
	int32_t msaa_w,msaa_h;
	GetMSAASize(dep.antialias,msaa_w,msaa_h);
	uint32_t format = (dep.format == CELL_GCM_SURFACE_Z24S8) 
		? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_G8B8;
	uint32_t swizzle = (dep.type == CELL_GCM_SURFACE_PITCH) ? CELL_GCM_TEXTURE_LN : CELL_GCM_TEXTURE_SZ;
	tex.format = format | swizzle | normalize;
	tex.mipmap = 1;
	tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	tex.cubemap = CELL_GCM_FALSE;
	tex.remap = TEXTURE_REMAP_DEPTH_READ;
	tex.depth = 1;
	tex.width = dep.width * msaa_w;
	tex.height = dep.height * msaa_h;
	tex.pitch = dep.pitch;
	tex.offset = dep.offset;
	tex.location = dep.location;
	return CELL_OK;
}

}; // namespace Render
