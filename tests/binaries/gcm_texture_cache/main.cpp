/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

// some simple procedural texture 

#define __CELL_ASSERT__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <sys/timer.h>
#include <sys/return_code.h>

#include <cell/gcm.h>
#include <cell/sysmodule.h>
#include <cell/dbgfont.h>
#include <sysutil/sysutil_sysparam.h>
#include "cellutil.h"
#include "gcmutil.h"

#include "snaviutil.h"

using namespace cell::Gcm;

extern "C" int32_t userMain(void);

static int32_t initDisplay(void);
static int32_t initBuffer(void);
static void initShader(void);

// clean up function called at exit
static void clean_up(void);

#define HOST_SIZE (16*1024*1024)
void* host_addr = NULL;

#define COLOR_BUFFER_NUM 2

#define TEX_SIZE    256
#define VERTEX_SIZE 512
typedef struct
{
  float Px, Py, Pz;
  uint32_t RGBA; 
  float u, v;
} Vertex_t;

uint32_t        g_Width;
uint32_t        g_Height; 

uint32_t        g_ColorDepth=4; // ARGB8
uint32_t        g_ZDepth=4;     // COMPONENT24


// address of Frame/Z
uint32_t        g_FrameBaseOffset;
uint32_t        g_FrameOffset[2];
uint32_t        g_FramePitch;
uint32_t        g_DepthBaseOffset;
uint32_t        g_DepthOffset;
uint32_t        g_DepthPitch;

// shader
extern uint32_t     _binary_vp_shader_vpo_start;
extern uint32_t     _binary_vp_shader_vpo_end;
extern uint32_t     _binary_fp_shader_fpo_start;
extern uint32_t     _binary_fp_shader_fpo_end;

// vertex buffer
						
static Vertex_t *   g_VertexBuffer;             // this is vidmem
uint32_t            g_VertexBufferOffset;       // this is vidmem

static CGprogram    g_CGVertexProgram;          // CG binary program
static CGprogram    g_CGFragmentProgram;        // CG binary program

static void *       g_VertexProgramUCode;       // this is sysmem
static void *       g_FragmentProgramUCode;     // this is vidmem

static uint32_t     g_TextureWidth = TEX_SIZE;
static uint32_t     g_TextureHeight = TEX_SIZE;
static uint32_t     g_TextureSys[TEX_SIZE*TEX_SIZE*4]; // sys mem texture
static void *       g_TextureVid;               // video texture
static uint32_t     g_TextureVidOffset;         // video texture

static uint32_t     g_FrameIndex = 0;

static float        g_MVP[16];
static float        g_CosTable[TEX_SIZE*4];
static uint32_t     g_Palette[TEX_SIZE];

static uint32_t     g_VertexCount;

// time
#define TIME_STAMP_1 844
#define TIME_STAMP_2 845
#define GIGA_FLOAT 1000000000.0
static uint64_t g_DrawTime = 0;

static int32_t initDisplay(void)
{

  // read the current video status
  // INITIAL DISPLAY MODE HAS TO BE SET BY RUNNING SETMONITOR.SELF
  CellVideoOutState videoState;
  CELL_GCMUTIL_CHECK_ASSERT(cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState));

  CellVideoOutResolution videoRes;
  CELL_GCMUTIL_CHECK_ASSERT(cellVideoOutGetResolution(videoState.displayMode.resolutionId, &videoRes));

  g_Width  = videoRes.width;
  g_Height = videoRes.height;
  printf("g_Width = %d, g_Height %d\n", g_Width, g_Height);

  CellVideoOutConfiguration videoCfg;
  memset(&videoCfg, 0, sizeof(CellVideoOutConfiguration));
  videoCfg.resolutionId = videoState.displayMode.resolutionId;
  videoCfg.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
  videoCfg.pitch = cellGcmGetTiledPitchSize(cellGcmAlign(CELL_GCM_ZCULL_ALIGN_WIDTH, g_Width)*g_ColorDepth);
  
  CELL_GCMUTIL_CHECK_ASSERT(cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &videoCfg, NULL, 0));
  
  cellGcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);
  return 0;
}

static int32_t initBuffer(void)
{
  // preparation for buffer allocation, etc.:
  uint32_t bufferWidth = cellGcmAlign(CELL_GCM_ZCULL_ALIGN_WIDTH, g_Width);
  printf("bufferWidth:0x%x\n", bufferWidth);

  g_FramePitch = cellGcmGetTiledPitchSize(bufferWidth*g_ColorDepth);
  if (g_FramePitch==0) return -1;
  printf("FramePitch=0x%x\n",g_FramePitch);
  
  g_DepthPitch = cellGcmGetTiledPitchSize(bufferWidth*g_ZDepth);
  if (g_DepthPitch==0) return -1;
  printf("DepthPitch=0x%x\n",g_DepthPitch);

  // ColorSize: size of 1 color buffer
  uint32_t FrameSize =   g_FramePitch*cellGcmAlign(CELL_GCM_ZCULL_ALIGN_HEIGHT, g_Height);
  // ColorLimit: size of all the color buffers, to set tiled
  uint32_t FrameLimit =  cellGcmAlign(CELL_GCM_TILE_ALIGN_SIZE, COLOR_BUFFER_NUM * FrameSize);
  printf("FrameSize:0x%x FrameLimit:0x%x\n", FrameSize, FrameLimit);

  // DepthSize: size of 1 depth buffer
  uint32_t DepthSize     =  g_DepthPitch*cellGcmAlign(CELL_GCM_ZCULL_ALIGN_HEIGHT, g_Height);
  // DepthLimit: size of all the depth buffers, to set tiled
  uint32_t DepthLimit= cellGcmAlign(CELL_GCM_TILE_ALIGN_SIZE, DepthSize);
  printf("DepthSize:0x%x DepthLimit:0x%x\n", DepthSize, DepthLimit);

  // get config
  CellGcmConfig gcmConfig;
  cellGcmGetConfiguration(&gcmConfig);
  printf("* vidmem base: 0x%p\n", gcmConfig.localAddress);
  printf("* IO base    : 0x%p\n", gcmConfig.ioAddress);
  printf("* vidmem size: 0x%x\n", gcmConfig.localSize);
  printf("* IO size    : 0x%x\n", gcmConfig.ioSize);
  printf("* memclk     : %u\n",   gcmConfig.memoryFrequency / 1000000);
  printf("* nvclk      : %u\n",   gcmConfig.coreFrequency / 1000000);

  // Local Memory
  cellGcmUtilInitializeLocalMemory((size_t)gcmConfig.localAddress, (size_t)gcmConfig.localSize);

  void *FrameBaseAddress= cellGcmUtilAllocateLocalMemory(FrameLimit, CELL_GCM_TILE_ALIGN_OFFSET);
  CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(FrameBaseAddress, &g_FrameBaseOffset));
  printf("g_FrameBaseOffset = 0x%08x\n", g_FrameBaseOffset);

  void *DepthBaseAddress= cellGcmUtilAllocateLocalMemory(DepthLimit, CELL_GCM_TILE_ALIGN_OFFSET);
  CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(DepthBaseAddress, &g_DepthBaseOffset));
  printf("g_DepthBaseOffset = 0x%08x\n", g_DepthBaseOffset);

  //-----------------------------------------------  
  // buffers
  for (uint32_t j=0; j<2; j++) {
    g_FrameOffset[j]    = g_FrameBaseOffset + (j * FrameSize);
    printf("g_FrameOffset[%d]:0x%08x\n", j, g_FrameOffset[j]);
  }

  g_DepthOffset    = g_DepthBaseOffset;
  printf("g_DepthOffset   :0x%08x\n", g_DepthOffset);

  // set tile
  CELL_GCMUTIL_CHECK(cellGcmSetTileInfo(0, CELL_GCM_LOCATION_LOCAL, g_FrameBaseOffset, FrameLimit,
	  g_FramePitch, CELL_GCM_COMPMODE_DISABLED, 0, 0));
  CELL_GCMUTIL_CHECK(cellGcmBindTile(0));
  CELL_GCMUTIL_CHECK(cellGcmSetTileInfo(1, CELL_GCM_LOCATION_LOCAL, g_DepthBaseOffset, DepthLimit,
                  g_DepthPitch, CELL_GCM_COMPMODE_Z32_SEPSTENCIL, 0, 3));
  CELL_GCMUTIL_CHECK(cellGcmBindTile(1));

  /* regist surface */
  for (uint32_t i=0; i<COLOR_BUFFER_NUM; i++) {
	  CELL_GCMUTIL_CHECK_ASSERT(cellGcmSetDisplayBuffer(i, g_FrameOffset[i], g_FramePitch, g_Width, g_Height));
  }
  return 0;
}

static void initShader(void)
{
  g_CGVertexProgram   = (CGprogram)(void *)&_binary_vp_shader_vpo_start;
  g_CGFragmentProgram = (CGprogram)(void *)&_binary_fp_shader_fpo_start;

  // init  
  cellGcmCgInitProgram(g_CGVertexProgram);
  cellGcmCgInitProgram(g_CGFragmentProgram);

  // allocate video memory for fragment program
  uint32_t ucodeSize;
  void *ucode;

  // get and copy 
  cellGcmCgGetUCode(g_CGFragmentProgram, &ucode, &ucodeSize);

  g_FragmentProgramUCode = cellGcmUtilAllocateLocalMemory(ucodeSize, 64);
  assert(g_FragmentProgramUCode != NULL);
  memcpy(g_FragmentProgramUCode, ucode, ucodeSize); 

  // get ucode pointer 
  cellGcmCgGetUCode(g_CGVertexProgram, &ucode, &ucodeSize);
  g_VertexProgramUCode = ucode;
}

static void SetRenderTarget(const uint32_t Index)
{
  CellGcmSurface rt;
  memset(&rt, 0, sizeof(rt));
  rt.colorFormat     = CELL_GCM_SURFACE_A8R8G8B8;
  rt.colorTarget     = CELL_GCM_SURFACE_TARGET_0;
  rt.colorLocation[0]     = CELL_GCM_LOCATION_LOCAL;
  rt.colorOffset[0]     = g_FrameOffset[Index];
  rt.colorPitch[0]     = g_FramePitch;

  rt.depthFormat     = CELL_GCM_SURFACE_Z24S8;
  rt.depthLocation    = CELL_GCM_LOCATION_LOCAL;
  rt.depthOffset     = g_DepthOffset;
  rt.depthPitch     = g_DepthPitch;

  rt.colorLocation[1]     = CELL_GCM_LOCATION_LOCAL;
  rt.colorLocation[2]     = CELL_GCM_LOCATION_LOCAL;
  rt.colorLocation[3]     = CELL_GCM_LOCATION_LOCAL;
  rt.colorOffset[1]     = 0;
  rt.colorOffset[2]     = 0;
  rt.colorOffset[3]     = 0;
  rt.colorPitch[1]     = 64;
  rt.colorPitch[2]     = 64;
  rt.colorPitch[3]     = 64;

  rt.antialias         = CELL_GCM_SURFACE_CENTER_1;
  rt.type         = CELL_GCM_SURFACE_PITCH;

  rt.x         = 0;
  rt.y         = 0;
  rt.width         = g_Width;
  rt.height         = g_Height;
  cellGcmSetSurface(&rt);
}

static int32_t initVtxTexBuffer(void)
{
  // allocate vertex buffer
  uint32_t vsize = sizeof(Vertex_t);
  g_VertexBuffer = (Vertex_t *)cellGcmUtilAllocateLocalMemory(vsize*VERTEX_SIZE, CELL_GCM_TILE_ALIGN_OFFSET);
  assert(g_VertexBuffer != NULL);
  CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(g_VertexBuffer, &g_VertexBufferOffset));

  printf("g_VertexBufferOffset:0x%08x\n", g_VertexBufferOffset);

  // allocate texture buffers
  g_TextureVid = cellGcmUtilAllocateLocalMemory(
                          g_TextureWidth*g_TextureHeight*g_ColorDepth, CELL_GCM_TILE_ALIGN_OFFSET);
  assert(g_TextureVid != NULL);

  CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(g_TextureVid, &g_TextureVidOffset));
  printf("g_TextureVidOffset  :0x%08x\n", g_TextureVidOffset);
  memset(g_TextureVid, 0xFF, TEX_SIZE*TEX_SIZE*4);

  return 0;
}

static void BuildGLProjection(float *M, const float top, const float bottom, const float left, const float right, const float near, const float far)
{
  memset(M, 0, 16*sizeof(float)); 

  M[0*4+0] = (2.0f*near) / (right - left);
  M[1*4+1] = (2.0f*near) / (bottom - top);

  float A = (right + left) / (right - left);
  float B = (top + bottom) / (top - bottom);
  float C = -(far + near) / (far - near);
  float D = -(2.0f*far*near) / (far - near);

  M[0*4 + 2] = A;
  M[1*4 + 2] = B;
  M[2*4 + 2] = C;
  M[3*4 + 2] = -1.0f; 
  M[2*4 + 3] = D;
}

static void MatrixMul(float *Dest, float *A, float *B)
{
  for (int i=0; i < 4; i++)
    {
      for (int j=0; j < 4; j++)
    {
      Dest[i*4+j] = A[i*4+0]*B[0*4+j] + A[i*4+1]*B[1*4+j] + A[i*4+2]*B[2*4+j] + A[i*4+3]*B[3*4+j];
    }
    }
}
static void MatrixTranslate(float *M, const float x, const float y, const float z)
{
  memset(M, 0, sizeof(float)*16);
  M[0*4+3] = x;
  M[1*4+3] = y;
  M[2*4+3] = z;

  M[0*4+0] = 1.0f;
  M[1*4+1] = 1.0f;
  M[2*4+2] = 1.0f;
  M[3*4+3] = 1.0f;
}

static void EulerRotate(float *M, const float Rx, const float Ry, const float Rz)
{
  const float Sx = sinf(Rx);
  const float Cx = cosf(Rx);

  const float Cy = cosf(Ry);
  const float Sy = sinf(Ry);

  const float Cz = cosf(Rz);
  const float Sz = sinf(Rz);

  // x * y * z order
  M[0*4+0] = Cy*Cz;
  M[0*4+1] = Cy*Sz;
  M[0*4+2] = -Sy;
  M[0*4+3] = 0.0f;

  M[1*4+0] = Sx*Sy*Cz - Cx*Sz;
  M[1*4+1] = Sx*Sy*Sz + Cx*Cz;
  M[1*4+2] = Sx*Cy;
  M[1*4+3] = 0.0f;

  M[2*4+0] = Cx*Sy*Cz + Sz*Sx;
  M[2*4+1] = Cx*Sy*Sz - Cz*Sx;
  M[2*4+2] = Cx*Cy;
  M[2*4+3] = 0.0f;
 
  M[3*4+0] = 0.0f;
  M[3*4+1] = 0.0f;
  M[3*4+2] = 0.0f;
  M[3*4+3] = 1.0f;
}

#define PI 3.141592653
static uint32_t GenerateCube(Vertex_t *V)
{
  const Vertex_t *Base = V;
  float X[] = {-1.0f,  1.0f, -1.0f,  1.0f, -1.0f,  1.0f, -1.0f,  1.0f};
  float Y[] = {-1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f};
  float Z[] = {-1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f};

  uint32_t C0 = 0xFFFFFFFF;
  uint32_t C1 = 0x0000FF00;
  uint32_t C2 = 0x00FF0000;
  uint32_t C3 = 0xFF000000;
  uint32_t C4 = 0xFFFF0000;
  uint32_t C5 = 0xFF00FF00;

  // face 0
  V->Px = X[0]; V->Py = Y[0]; V->Pz = Z[0]; V->RGBA = C0; V->u = 0.0f; V->v = 0.0f; V++;
  V->Px = X[1]; V->Py = Y[1]; V->Pz = Z[1]; V->RGBA = C0; V->u = 1.0f; V->v = 0.0f; V++;
  V->Px = X[2]; V->Py = Y[2]; V->Pz = Z[2]; V->RGBA = C0; V->u = 0.0f; V->v = 1.0f; V++;

  V->Px = X[1]; V->Py = Y[1]; V->Pz = Z[1]; V->RGBA = C0; V->u = 1.0f; V->v = 0.0f; V++;
  V->Px = X[2]; V->Py = Y[2]; V->Pz = Z[2]; V->RGBA = C0; V->u = 0.0f; V->v = 1.0f; V++;
  V->Px = X[3]; V->Py = Y[3]; V->Pz = Z[3]; V->RGBA = C0; V->u = 1.0f; V->v = 1.0f; V++;

  // face 1
  V->Px = X[0]; V->Py = Y[0]; V->Pz = Z[0]; V->RGBA = C1; V->u = 0.0f; V->v = 0.0f; V++;
  V->Px = X[1]; V->Py = Y[1]; V->Pz = Z[1]; V->RGBA = C1; V->u = 1.0f; V->v = 0.0f; V++;
  V->Px = X[4]; V->Py = Y[4]; V->Pz = Z[4]; V->RGBA = C1; V->u = 0.0f; V->v = 1.0f; V++;

  V->Px = X[1]; V->Py = Y[1]; V->Pz = Z[1]; V->RGBA = C1; V->u = 1.0f; V->v = 0.0f; V++;
  V->Px = X[4]; V->Py = Y[4]; V->Pz = Z[4]; V->RGBA = C1; V->u = 0.0f; V->v = 1.0f; V++;
  V->Px = X[5]; V->Py = Y[5]; V->Pz = Z[5]; V->RGBA = C1; V->u = 1.0f; V->v = 1.0f; V++;

  // face 2
  V->Px = X[2]; V->Py = Y[2]; V->Pz = Z[2]; V->RGBA = C2; V->u = 0.0f; V->v = 1.0f; V++;
  V->Px = X[3]; V->Py = Y[3]; V->Pz = Z[3]; V->RGBA = C2; V->u = 1.0f; V->v = 1.0f; V++;
  V->Px = X[6]; V->Py = Y[6]; V->Pz = Z[6]; V->RGBA = C2; V->u = 0.0f; V->v = 0.0f; V++;

  V->Px = X[3]; V->Py = Y[3]; V->Pz = Z[3]; V->RGBA = C2; V->u = 1.0f; V->v = 1.0f; V++;
  V->Px = X[6]; V->Py = Y[6]; V->Pz = Z[6]; V->RGBA = C2; V->u = 0.0f; V->v = 0.0f; V++;
  V->Px = X[7]; V->Py = Y[7]; V->Pz = Z[7]; V->RGBA = C2; V->u = 1.0f; V->v = 0.0f; V++;

  // face 3
  V->Px = X[6]; V->Py = Y[6]; V->Pz = Z[6]; V->RGBA = C3; V->u = 0.0f; V->v = 0.0f; V++;
  V->Px = X[7]; V->Py = Y[7]; V->Pz = Z[7]; V->RGBA = C3; V->u = 1.0f; V->v = 0.0f; V++;
  V->Px = X[4]; V->Py = Y[4]; V->Pz = Z[4]; V->RGBA = C3; V->u = 0.0f; V->v = 1.0f; V++;

  V->Px = X[7]; V->Py = Y[7]; V->Pz = Z[7]; V->RGBA = C3; V->u = 1.0f; V->v = 0.0f; V++;
  V->Px = X[4]; V->Py = Y[4]; V->Pz = Z[4]; V->RGBA = C3; V->u = 0.0f; V->v = 1.0f; V++;
  V->Px = X[5]; V->Py = Y[5]; V->Pz = Z[5]; V->RGBA = C3; V->u = 1.0f; V->v = 1.0f; V++;

  // face 4
  V->Px = X[3]; V->Py = Y[3]; V->Pz = Z[3]; V->RGBA = C4; V->u = 1.0f; V->v = 1.0f; V++;
  V->Px = X[1]; V->Py = Y[1]; V->Pz = Z[1]; V->RGBA = C4; V->u = 0.0f; V->v = 1.0f; V++;
  V->Px = X[5]; V->Py = Y[5]; V->Pz = Z[5]; V->RGBA = C4; V->u = 0.0f; V->v = 0.0f; V++;

  V->Px = X[3]; V->Py = Y[3]; V->Pz = Z[3]; V->RGBA = C4; V->u = 1.0f; V->v = 1.0f; V++;
  V->Px = X[5]; V->Py = Y[5]; V->Pz = Z[5]; V->RGBA = C4; V->u = 0.0f; V->v = 0.0f; V++;
  V->Px = X[7]; V->Py = Y[7]; V->Pz = Z[7]; V->RGBA = C4; V->u = 1.0f; V->v = 0.0f; V++;

  // face 5
  V->Px = X[2]; V->Py = Y[2]; V->Pz = Z[2]; V->RGBA = C5; V->u = 0.0f; V->v = 0.0f; V++;
  V->Px = X[0]; V->Py = Y[0]; V->Pz = Z[0]; V->RGBA = C5; V->u = 1.0f; V->v = 0.0f; V++;
  V->Px = X[4]; V->Py = Y[4]; V->Pz = Z[4]; V->RGBA = C5; V->u = 1.0f; V->v = 1.0f; V++;

  V->Px = X[2]; V->Py = Y[2]; V->Pz = Z[2]; V->RGBA = C5; V->u = 0.0f; V->v = 0.0f; V++;
  V->Px = X[4]; V->Py = Y[4]; V->Pz = Z[4]; V->RGBA = C5; V->u = 1.0f; V->v = 1.0f; V++;
  V->Px = X[6]; V->Py = Y[6]; V->Pz = Z[6]; V->RGBA = C5; V->u = 0.0f; V->v = 1.0f; V++;

  return V - Base;
}

// .... its been a while. 
void GenerateTexture(uint32_t *Texture, void *Target, uint32_t forceColor)
{
  static uint32_t t0 = 0;
  static uint32_t t1 = 10;
  static uint32_t t2 = 900;
  static uint32_t t3 = 400;

  for (uint32_t i=0; i < g_TextureHeight; i++) {
    for (uint32_t j=0; j < g_TextureWidth; j++) {
      float c0 = g_CosTable[t0&0x3FF];
      float c1 = g_CosTable[t1&0x3FF];
      float c2 = g_CosTable[t2&0x3FF];
      float c3 = g_CosTable[t3&0x3FF];

      float Index_f = (40.0f*(c0 + c0*c1 + c0*c3 + c2));
      Index_f = Index_f > 0 ? Index_f : -Index_f; // to cast into unsigned
      uint32_t Index = (uint32_t)Index_f;

      Texture[i*g_TextureWidth + j] = forceColor > 0 ? g_Palette[forceColor] : g_Palette[Index&0xFF]; 
      t0 += 1;
      t1 += 3;
    }
    t2 += 3;
    t3 += 2;
  }
  t0 += 3;
  t1 += 2;
  t2 += 1;
  t3 += 3;

  // copy directly. texture is not swizzled 
  memcpy(Target, Texture, g_TextureWidth*g_TextureHeight*g_ColorDepth);
}

static void initModelView(void)
{
  // Transform
  float P[16];
  float V[16];

  // projection 
  BuildGLProjection(P, -1.0f, 1.0f, -1.0f, 1.0f, 1.0, 10000.0f); 

  // 16:9 scale
  MatrixTranslate(V, 0.0f, 0.0f, 0.0);
  V[0*4 + 0] = 9.0f / 16.0f; 
  V[1*4 + 1] = 1.0f; 

  // model view 
  MatrixMul(g_MVP, P, V);
}

static void generateCosTable(void)
{
  // cos generate table
  for (uint32_t i=0; i < TEX_SIZE*4; i++) {
      g_CosTable[i] = (cosf(i*2.0*PI/TEX_SIZE));
  }
}

static void generatePalette(void)
{
  // generate palette 
  float R[] = {  255, 0};
  float G[] = {  0,   0};
  float B[] = {  0,   0};
  for (uint32_t i=0; i < 128; i++)
    {
      float t = (float)i / 128.0;
      uint32_t r0 = (uint32_t)(t*R[0] + (1-t)*R[1]);
      uint32_t g0 = (uint32_t)(t*G[0] + (1-t)*G[1]);
      uint32_t b0 = (uint32_t)(t*B[0] + (1-t)*B[1]);

      uint32_t r1 = (uint32_t)(t*R[1] + (1-t)*R[0]);
      uint32_t g1 = (uint32_t)(t*G[1] + (1-t)*G[0]);
      uint32_t b1 = (uint32_t)(t*B[1] + (1-t)*B[0]);

      g_Palette[i]     = (r0<<16)| (g0<<8) | (b0<<0);
      g_Palette[i+128]  = (r1<<16)| (g1<<8) | (b1<<0);
    }
}

static void setViewport(void)
{
      uint16_t x, y, w,h;
      float min = 0.0f, max = 1.0f;

      x = y = 0;
      w = g_Width;
      h = g_Height;
#if 1
      float scale[4] = { w*0.5f, h*-0.5f, (max-min)*0.5f, 0.0f };
      float offset[4] = { x + scale[0], h - y + scale[1], (max+min)*0.5f, 0.0f };
#else
      float scale[4] = { w*0.5f, h*0.5f, (max-min)*0.5f, 0.0f };
      float offset[4] = { x + scale[0], y + scale[1], (max+min)*0.5f, 0.0f };
#endif

      cellGcmSetViewport(x, y, w, h, min, max, scale, offset); 
      cellGcmSetScissor( x, y, w, h ) ;
}

static void drawCube(void)
{
      cellGcmSetClearColor((32<<0)|(32<<8)|(32<<16)|(32<<24));

      // inital state
      cellGcmSetBlendEnable(CELL_GCM_FALSE);
      cellGcmSetDepthTestEnable(CELL_GCM_TRUE);

      cellGcmSetDepthFunc(CELL_GCM_LESS);

      uint32_t depthStencilClearValue =   0xffffff << 8 | 0;
      cellGcmSetClearDepthStencil(depthStencilClearValue);     

      // model rotate
      static float AngleX = 0.0f; 
      static float AngleY = 0.0f; 
      static float AngleZ = 0.0f;
      AngleX += 0.02f;
      AngleY += 0.03f;
      AngleZ += 0.05f;

      float M[16];
      float MVP[16];
      EulerRotate(M, AngleX, AngleY, AngleZ); 

      M[0*4+3] = 0.0f;
      M[1*4+3] = 0.0f;
      M[2*4+3] = -3.0f; 

      // final
      MatrixMul(MVP, g_MVP, M);  

      // clear frame buffer
      cellGcmSetClearSurface(CELL_GCM_CLEAR_Z |
                 CELL_GCM_CLEAR_R |
                 CELL_GCM_CLEAR_G |
                 CELL_GCM_CLEAR_B |
                 CELL_GCM_CLEAR_A);  


      // set uniform variables 
      // NOTE: this modifies the ucode but currently the ucode is inlined into the push buffer
      CGparameter modelViewProj
                      = cellGcmCgGetNamedParameter(g_CGVertexProgram, "modelViewProj");
      CGparameter objCoord    = cellGcmCgGetNamedParameter(g_CGVertexProgram, "a2v.objCoord");
      CGparameter color        = cellGcmCgGetNamedParameter(g_CGVertexProgram, "a2v.color");
      CGparameter tex        = cellGcmCgGetNamedParameter(g_CGVertexProgram, "a2v.tex");

      CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( modelViewProj );
      CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( objCoord );
      CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( color );
      CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( tex );

      cellGcmSetVertexProgramParameter(modelViewProj, MVP);

      // get Vertex Attribute index
      uint32_t ObjCoordIndex
                    = cellGcmCgGetParameterResource(g_CGVertexProgram, objCoord) - CG_ATTR0; 
      uint32_t ColorIndex
                    = cellGcmCgGetParameterResource(g_CGVertexProgram, color) - CG_ATTR0; 
      uint32_t TexIndex
                    = cellGcmCgGetParameterResource(g_CGVertexProgram, tex) - CG_ATTR0; 

      CGparameter texture = cellGcmCgGetNamedParameter(g_CGFragmentProgram, "texture0");
      CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( texture );
      CGresource texunit = (CGresource)(cellGcmCgGetParameterResource(g_CGFragmentProgram, texture) - CG_TEXUNIT0);
      
      CellGcmTexture cubetex;
      cubetex.format = (CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR);
      cubetex.remap =   (CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
             CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
             CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
             CELL_GCM_TEXTURE_REMAP_REMAP <<  8 |
             CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
             CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
             CELL_GCM_TEXTURE_REMAP_FROM_R << 2 |
             CELL_GCM_TEXTURE_REMAP_FROM_A);

      cubetex.mipmap = 1;
      cubetex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
      cubetex.cubemap = CELL_GCM_FALSE;
      cubetex.width = g_TextureWidth;
      cubetex.height = g_TextureHeight;
      cubetex.depth = 1;
      cubetex.pitch = g_TextureWidth * g_ColorDepth;
      cubetex.location = CELL_GCM_LOCATION_LOCAL;
      cubetex.offset = g_TextureVidOffset;
      cellGcmSetTexture(texunit, &cubetex);
      
      // bind texture and set filter
      cellGcmSetTextureControl(texunit, CELL_GCM_TRUE, 0, 0, 0); // MIN:0,MAX:15
      
      cellGcmSetTextureAddress(texunit,
                   CELL_GCM_TEXTURE_CLAMP,
                   CELL_GCM_TEXTURE_CLAMP,
                   CELL_GCM_TEXTURE_CLAMP,
                   CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL,
                   CELL_GCM_TEXTURE_ZFUNC_LESS,
                   0);
      cellGcmSetTextureFilter(texunit, 0,
                  CELL_GCM_TEXTURE_LINEAR,
                  CELL_GCM_TEXTURE_LINEAR,
                  CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);
      

      // bind the shaders
      // NOTE: vertex program constants are copied here
      cellGcmSetVertexProgram(g_CGVertexProgram, g_VertexProgramUCode);

      uint32_t fragment_offset;
      CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(g_FragmentProgramUCode, &fragment_offset));
      cellGcmSetFragmentProgram(g_CGFragmentProgram, fragment_offset);

	  uint32_t vertex_offset[3];
	  CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(&g_VertexBuffer->Px, &vertex_offset[0]));
	  CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(&g_VertexBuffer->RGBA, &vertex_offset[1]));
	  CELL_GCMUTIL_CHECK_ASSERT(cellGcmAddressToOffset(&g_VertexBuffer->u, &vertex_offset[2]));

      // set vertex pointer
      cellGcmSetVertexDataArray(ObjCoordIndex, 0, sizeof(Vertex_t), 3,
                 CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, vertex_offset[0]);

      cellGcmSetVertexDataArray(ColorIndex, 0, sizeof(Vertex_t), 4,
                 CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_LOCAL, vertex_offset[1]);

      cellGcmSetVertexDataArray(TexIndex, 0, sizeof(Vertex_t), 2,
                 CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, vertex_offset[2]);

      // kaboom
      cellGcmSetDrawArrays(CELL_GCM_PRIMITIVE_TRIANGLES, 0, g_VertexCount);
}

//
// dbgfont
//
static void initDbgFont(void)
{
  int frag_size = CELL_DBGFONT_FRAGMENT_SIZE;
  int font_tex  = CELL_DBGFONT_TEXTURE_SIZE;
  int vertex_size = 1024 * CELL_DBGFONT_VERTEX_SIZE;
  int local_size = frag_size + font_tex + vertex_size;
  void *localmem = (void *)cellGcmUtilAllocateLocalMemory(local_size, 128);

  printf("local_size = %d\n", local_size);

  CellDbgFontConfigGcm cfg;
  memset(&cfg, 0, sizeof(CellDbgFontConfigGcm));
  cfg.localBufAddr = (sys_addr_t)localmem;
  cfg.localBufSize = local_size;
  cfg.mainBufAddr = NULL;
  cfg.mainBufSize = 0;
  cfg.option = CELL_DBGFONT_VERTEX_LOCAL |
               CELL_DBGFONT_TEXTURE_LOCAL |
    CELL_DBGFONT_SYNC_ON;
  cellDbgFontInitGcm(&cfg);

}
static void printDbgFont(void)
{
  float x = 0.05f, y = 0.05f, dy = 0.05f;
  float draw_time;

  draw_time = (float)g_DrawTime/1000000.0f;
  y += dy;
  cellDbgFontPrintf(x, y, 1.0f, 0xff00ffff,
                    "Drawing Time = %.4f msec", draw_time);

  float dps = GIGA_FLOAT / (float)g_DrawTime;
  y += dy;
  cellDbgFontPrintf(x, y, 1.0f, 0xff00ffff,
                    "draw/sec     = %.4f", dps);

  cellDbgFontDrawGcm();
}

static void exitDbgFont(void)
{
  cellDbgFontExitGcm();
}

//
// flip
//
static bool sFlipped=true;
static void FlipCallback(uint32_t head)
{
    (void)head;
    sFlipped=true;
}

/* wait until flip */
static void waitFlip(void)
{
    while (!sFlipped){
        sys_timer_usleep(300);
    }
    sFlipped=false;
}

static void flip(void)
{
    if(cellGcmSetFlip(g_FrameIndex) != CELL_OK) {
      printf("SetFlip returned NOT_OK\n");
      return;
    }
    cellGcmFlush();

    cellGcmSetWaitFlip();
}


//
// main
//
#define CB_SIZE    (0x10000)

int32_t userMain()
{
  // init
  host_addr = memalign(1024*1024, HOST_SIZE);
  CELL_GCMUTIL_ASSERTS(host_addr != NULL,"memalign()");
  CELL_GCMUTIL_CHECK_ASSERT(cellGcmInit(CB_SIZE, HOST_SIZE, host_addr));

  cellGcmSetDebugOutputLevel(CELL_GCM_DEBUG_LEVEL2);

  initDisplay();

  initBuffer();

  initVtxTexBuffer();

  initShader();

  initModelView();

  initDbgFont();

  cellGcmUtilInitCallback(clean_up);

  // build cube verts
  g_VertexCount = GenerateCube(g_VertexBuffer);

  generateCosTable();
  generatePalette();

  sFlipped=true;
  cellGcmSetFlipHandler(FlipCallback);

  int frames = 0;

  // rendering loop
  while (cellGcmUtilCheckCallback() && frames < 3)
  {
      GenerateTexture(g_TextureSys, g_TextureVid, frames % 2 ? 10 : 0);

	  frames++;

      //=========================================================================

      // set inital target
      g_FrameIndex ^= 0x1;
      SetRenderTarget(g_FrameIndex);

      cellGcmSetColorMask(CELL_GCM_COLOR_MASK_B|
              CELL_GCM_COLOR_MASK_G|
              CELL_GCM_COLOR_MASK_R|
              CELL_GCM_COLOR_MASK_A);
      
      cellGcmSetColorMaskMrt(0);

      setViewport();

      cellGcmSetTimeStamp(TIME_STAMP_1);
      drawCube();
      cellGcmSetTimeStamp(TIME_STAMP_2);

      cellGcmFinish(1);

      uint64_t time0 = cellGcmGetTimeStamp(TIME_STAMP_1);
      uint64_t time1 = cellGcmGetTimeStamp(TIME_STAMP_2);
      g_DrawTime = time1 - time0;
      //printDbgFont();

      //========================================================================
      // wait until the previous flip executed

      waitFlip();

      flip();
      
      // for sample navigator
      if(cellSnaviUtilIsExitRequested(0)){
          break;
      }
    }
    
    return 0;
}

void clean_up(void)
{
	// exit debug font
	exitDbgFont();
}
