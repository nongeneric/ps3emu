#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string>
using namespace std;
extern unsigned char fs_basic[208];
extern unsigned char vs_basic[672];
#include <sysutil/sysutil_sysparam.h>
#include <cell/gcm.h>
using namespace cell::Gcm;
#include <vectormath/cpp/vectormath_aos.h>
#include "objs\fs_basic.fpo.c"
#include "objs\vs_basic.vpo.c"

using namespace Vectormath::Aos;
#define HOST_SIZE (1024 * 1024)
#define COMMAND_SIZE (65536)
#define BUFFERS_COUNT (2)
uint32_t localHeapStart = 0;
void *LocalMemoryAlloc(const uint32_t size) {
  uint32_t currentHeap = localHeapStart;
  localHeapStart += (size + 1023) & (~1023);
  return (void *)currentHeap;
}
void *LocalMemoryAlign(const uint32_t alignment, const uint32_t size) {
  localHeapStart = (localHeapStart + alignment - 1) & (~(alignment - 1));
  return (void *)LocalMemoryAlloc(size);
}
struct stVertex {
  float x, y, z;
  uint32_t rgba;
};
void DumpOffsetTable() {
  CellGcmOffsetTable addrTable;
  cellGcmGetOffsetTable(&addrTable);
  for (uint32_t i = 0; i <= 0xbff; ++i) {
    uint32_t io = addrTable.ioAddress[i];
    if (io == 0xffff)
      continue;
  }
  for (uint32_t i = 0; i <= 255; ++i) {
    uint32_t ea = addrTable.eaAddress[i];
    if (ea == 0xffff)
      continue;
  }
}
int main() {

  void *host_addr = memalign(1024 * 1024, HOST_SIZE * 3);

  int err =
      cellGcmInit(COMMAND_SIZE, HOST_SIZE, (uint8_t *)host_addr + 0x200000);
  CellGcmContextData *context = gCellGcmCurrentContext;
  uint32_t coffset = 0;
  int coffset_res = cellGcmAddressToOffset(context, &coffset);
  uint32_t *stack = &coffset;

  CellGcmControl *regs = cellGcmGetControlRegister();
  void *addr = (void *)0x40300050;
  uint32_t offset = 0;
  cellGcmAddressToOffset(addr, &offset);
  uint32_t iooffset;
  cellGcmAddressToOffset((char *)host_addr + 8, &iooffset);

  CellVideoOutState videoState;
  CellVideoOutResolution resolution;

  err = cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);
  err = cellVideoOutGetResolution(videoState.displayMode.resolutionId,
                                  &resolution);

  uint32_t color_depth = 4;
  uint32_t z_depth = 4;
  uint32_t color_pitch = resolution.width * color_depth;
  uint32_t color_size = color_pitch * resolution.height;
  uint32_t depth_pitch = resolution.width * z_depth;
  uint32_t depthSize = depth_pitch * resolution.height;
  CellVideoOutConfiguration video_cfg;

  memset(&video_cfg, 0, sizeof(CellVideoOutConfiguration));
  video_cfg.resolutionId = videoState.displayMode.resolutionId;
  video_cfg.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
  video_cfg.pitch = color_pitch;

  err = cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &video_cfg, NULL, 0);

  err = cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);

  float screenRatio;
  switch (videoState.displayMode.aspect) {
  case CELL_VIDEO_OUT_ASPECT_4_3:
    screenRatio = 4.0f / 3.0f;
    break;
  case CELL_VIDEO_OUT_ASPECT_16_9:
    screenRatio = 16.0f / 9.0f;
    break;
  default:
    screenRatio = 16.0f / 9.0f;
  }
  cellGcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);

  CellGcmConfig config;
  cellGcmGetConfiguration(&config);

  localHeapStart = (uint32_t)config.localAddress;

  void *depthBuffer = LocalMemoryAlign(64, depthSize);
  uint32_t depthOffset;

  cellGcmAddressToOffset(depthBuffer, &depthOffset);

  CellGcmSurface surfaces[BUFFERS_COUNT];
  for (int i = 0; i < BUFFERS_COUNT; ++i) {

    void *buffer = LocalMemoryAlign(64, color_size);

    cellGcmAddressToOffset(buffer, &surfaces[i].colorOffset[0]);
    cellGcmSetDisplayBuffer(i, surfaces[i].colorOffset[0], color_pitch,
                            resolution.width, resolution.height);

    surfaces[i].colorLocation[0] = CELL_GCM_LOCATION_LOCAL;

    surfaces[i].colorPitch[0] = color_pitch;

    surfaces[i].colorTarget = CELL_GCM_SURFACE_TARGET_0;

    for (int j = 1; j < 4; ++j) {
      surfaces[i].colorLocation[j] = CELL_GCM_LOCATION_LOCAL;
      surfaces[i].colorOffset[j] = 0;
      surfaces[i].colorPitch[j] = 64;
    }

    surfaces[i].type = CELL_GCM_SURFACE_PITCH;

    surfaces[i].antialias = CELL_GCM_SURFACE_CENTER_1;

    surfaces[i].colorFormat = CELL_GCM_SURFACE_A8R8G8B8;

    surfaces[i].depthFormat = CELL_GCM_SURFACE_Z24S8;

    surfaces[i].depthLocation = CELL_GCM_LOCATION_LOCAL;

    surfaces[i].depthOffset = depthOffset;

    surfaces[i].depthPitch = depth_pitch;

    surfaces[i].width = resolution.width;
    surfaces[i].height = resolution.height;

    surfaces[i].x = 0;
    surfaces[i].y = 0;
  }

  cellGcmSetSurface(&surfaces[0]);

  uint8_t swapValue = 0;

  CGprogram programFS;

  void *ucodeFS;
  uint32_t offsetFS;
  {
    programFS = (CGprogram)(void *)objs__fs_basic_fpo;

    cellGcmCgInitProgram(programFS);
    unsigned int ucodeSize;
    void *ucodePtr;

    cellGcmCgGetUCode(programFS, &ucodePtr, &ucodeSize);

    ucodeFS = LocalMemoryAlign(64, ucodeSize);
    *(uint32_t *)ucodeFS = 0xff00ff00;

    memcpy(ucodeFS, ucodePtr, ucodeSize);

    cellGcmAddressToOffset(ucodeFS, &offsetFS);
  }

  CGprogram programVS;

  void *ucodeVS;
  {
    programVS = (CGprogram)(void *)objs__vs_basic_vpo;

    cellGcmCgInitProgram(programVS);
    unsigned int ucodeSize;

    cellGcmCgGetUCode(programVS, &ucodeVS, &ucodeSize);
  }

  cellGcmSetFragmentProgram(programFS, offsetFS);
  cellGcmSetVertexProgram(programVS, ucodeVS);

  CGparameter position = cellGcmCgGetNamedParameter(programVS, "position");
  CGparameter color = cellGcmCgGetNamedParameter(programVS, "color");
  CGparameter mvp = cellGcmCgGetNamedParameter(programVS, "modelViewProj");

  int PositionIndex =
      cellGcmCgGetParameterResource(programVS, position) - CG_ATTR0;
  int ColorIndex = cellGcmCgGetParameterResource(programVS, color) - CG_ATTR0;

  cellGcmSetUpdateFragmentProgramParameter(offsetFS);
  int i = sizeof(CellGcmConfig);

  float tempMatrix[16] = {
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
  };

  cellGcmSetVertexProgramParameter(mvp, (float *)&tempMatrix);

  const int numVerts = 3;

  stVertex *vertexBuffer =
      (stVertex *)LocalMemoryAlign(128, sizeof(stVertex) * numVerts);

  uint32_t VertexBufferOffset;

  err = cellGcmAddressToOffset((void *)vertexBuffer, &VertexBufferOffset);

  while (true) {

    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t w = resolution.width;
    uint16_t h = resolution.height;
    float fmin = 0.0f;
    float fmax = 1.0f;

    float scale[4];
    scale[0] = w * 0.5f;
    scale[1] = h * -0.5f;
    scale[2] = (fmax - fmin) * 0.5f;
    scale[3] = 0.0f;

    float offset[4];
    offset[0] = x + scale[0];
    offset[1] = y + h * 0.5f;
    offset[2] = (fmax + fmin) * 0.5f;
    offset[3] = 0.0f;

    cellGcmSetViewport(x, y, w, h, fmin, fmax, scale, offset);

    cellGcmSetColorMask(CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G |
                        CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A);

    cellGcmSetDepthTestEnable(CELL_GCM_TRUE);

    cellGcmSetDepthFunc(CELL_GCM_LESS);

    cellGcmSetCullFaceEnable(CELL_GCM_FALSE);

    cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
    cellGcmSetShadeMode(CELL_GCM_SMOOTH);

    static float count = 0;

    unsigned char r = ((int)count) % 255;
    unsigned char g = 32;
    unsigned char b = (255 - (int)count) % 255;
    cellGcmSetClearColor((b << 0) | (g << 8) | (r << 16) | (255 << 24));
    cellGcmSetClearSurface(CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_S |
                           CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |
                           CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

    vertexBuffer[0].x = -1;
    vertexBuffer[0].y = -1;
    vertexBuffer[0].z = 0.5;
    vertexBuffer[0].rgba = 0xff0000ff;

    vertexBuffer[1].x = 0;
    vertexBuffer[1].y = 1;
    vertexBuffer[1].z = 0.5;
    vertexBuffer[1].rgba = 0x00ff00ff;

    vertexBuffer[2].x = 1;
    vertexBuffer[2].y = -1;
    vertexBuffer[2].z = 0.5;
    vertexBuffer[2].rgba = 0x0000ffff;

    cellGcmSetVertexDataArray(PositionIndex, 0, sizeof(stVertex), 3,
                              CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL,
                              VertexBufferOffset);
    cellGcmSetVertexDataArray(ColorIndex, 0, sizeof(stVertex), 4,
                              CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_LOCAL,
                              VertexBufferOffset + sizeof(float) * 3);

    cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, numVerts);

    while (cellGcmGetFlipStatus() != 0) {
      sys_timer_usleep(100);
    }
    cellGcmResetFlipStatus();

    cellGcmSetFlip((uint8_t)swapValue);
    cellGcmFlush();

    cellGcmSetWaitFlip();
    swapValue = !swapValue;
    cellGcmSetSurface(&surfaces[swapValue]);
	return 0;
  }
  return 0;
}
