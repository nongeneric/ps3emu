/* SCE CONFIDENTIAL
 * PlayStation(R)Edge 1.3.0
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <cell/cell_fs.h>
#include <cell/gcm.h>
#include <cell/spurs.h>
#include <cell/sysmodule.h>
#include <sys/spu_initialize.h>
#include <ppu_intrinsics.h>
#include <spu_printf.h>

#include "gcm/FWCellGCMWindow.h"
#include "FWDebugFont.h"
#include "FWTime.h"
#include "gcmutil.h"
#include "../common/profile_bars.h"
#include "../common/sampledatadirs.h"

#include "edge/geom/edgegeom_structs.h"
#include "spu/job_geom_interface.h"

#include "main.h"

#include "../common/data/elephant.edge.h"

// When the RSX reads the fragment program, it can read up to 400 bytes past
// the end of the program.  We allocate and map an extra 512 bytes at the end
// of the ring and shared buffers, but we do not include the extra space in
// the size of the output buffer info structures.  This prevents us from
// reading unmapped memory due to overfetch.
const uint32_t FRAGMENT_OVERFETCH_BUFFER_SIZE   = 512;

#define SPURS_SPU_NUM					2
#define SPU_THREAD_GROUP_PRIORITY		250
#define EVENT_FLAG_BIT					(0U)					// bit used by job_send_event (between 0 and 15)
#define EVENT_FLAG_BITS					(1U<<(EVENT_FLAG_BIT))	// bit mask for job_send_event

const uint32_t EDGE_SHARED_OUTPUT_BUFFER_SIZE = 4*1024*1024;
const uint32_t EDGE_RING_OUTPUT_BUFFER_SIZE = 1*1024*1024; // per SPU
const uint32_t COMMAND_BUFFER_SIZE = 4*1024*1024;

#define MAX_NUM_BLENDSHAPES 2 // arbitrary limit
#define MAX_SEGMENTS 0x100 * 4

// This must be the index of the first of a contiguous block of RSX labels,
// one for each SPU.  Labels 0-63 are reserved by the OS, and your application
// and its libraries may have reserved others.  128 seems safe for this sample app.
const uint32_t kFirstRsxLabelIndex = 128;

using namespace cell;
using namespace cell::Gcm;

static const unsigned int textureDimension = 1024;

// Skinning matrix data
static const uint32_t kNumFrames = 120;
static const uint32_t kNumBones = 32; // the elephant model has this many bones

// Shader (embedded)
extern unsigned long _binary_vpshader_vpo_start;
extern unsigned long _binary_vpshader_vpo_end;
extern unsigned long _binary_fpshader_fpo_start;
extern unsigned long _binary_fpshader_fpo_end;
// Embedded SPU ELF symbols
extern char _binary_job_job_geom_spu_bin_start[];
extern char _binary_job_job_geom_spu_bin_size[];
extern char _binary_job_job_send_event_spu_bin_start[];
extern char _binary_job_job_send_event_spu_bin_size[];

// instantiate the class
SampleApp app;

static const uint32_t numElephants = 128;
static uint32_t elephantAnimFrameOffsets[numElephants];
static EdgeGeomLocalToWorldMatrix localToWorldMatrix[numElephants] __attribute__((__aligned__(128)));

static CellSpursJob256 jobs[MAX_SEGMENTS];

static EdgeGeomBlendShapeInfo shapeInfos[MAX_SEGMENTS*MAX_NUM_BLENDSHAPES] __attribute__((__aligned__(128)));
static uint32_t numShapeInfos = 0;
static float shapeAlphas[MAX_NUM_BLENDSHAPES] = {0};

static void *toPatchAddress;
static uint32_t toPatchSize;
static const char *toPatchParameters[] = {"lightRgb"};
static float toPatchValues[] = {1.f,  0.f,  0.f,  0.f,	// Red
								0.f,  1.f,  0.f,  0.f,	// Green
								0.f,  0.f,  1.f,  0.f,	// Blue
								0.5f, 0.5f, 0.5f, 0.f}; // Grey
#define MAX_PATCHES 16
struct PatchInfo
{
	uint32_t numToPatch;
	uint32_t pad[3];
	uint32_t numInstances[MAX_PATCHES];
	uint32_t offsets[MAX_PATCHES];
};

static PatchInfo patchInfo __attribute__((__aligned__(16)));
static float patchData[numElephants * 4 * MAX_PATCHES] __attribute__((__aligned__(16)));

static void buildMipmaps(const void *tex, void *result, uint32_t topDim, uint32_t dim)
{
	if (dim <= 1)
		return;

	const uint8_t *in0 = (const uint8_t*)tex;
	const uint8_t *in1 = (const uint8_t*)tex + 4;
	const uint8_t *in2 = (const uint8_t*)tex + 4 * topDim;
	const uint8_t *in3 = (const uint8_t*)tex + 4 * topDim + 4;
	uint8_t *out = (uint8_t *)result;
	for (uint32_t i = 0; i < dim * topDim / 2; i++)
	{
		for (int k = 0; k < 4; k++)
		{
			*out = (*in0 + *in1 + *in2 + *in3)/4;
			in0++;
			in1++;
			in2++;
			in3++;
			out++;
		}
		in0 += 4;
		in1 += 4;
		in2 += 4;
		in3 += 4;
	}

	buildMipmaps(result, out, topDim, dim >> 1);
}

static void* loadFile(const char* fileName, uint32_t& fileSize)
{
	FILE* fp = fopen(fileName, "rb");
	if (!fp) {
		printf("cannot open file %s\n", fileName);
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	void* data = memalign(128, fileSize);
	if (!data) {
		printf("can't allocate memory\n");
		return NULL;
	}
	fread(data, 1, fileSize, fp);
	fclose(fp);

	return data;
}

// Loads a texture and creates mipmaps.
// Assumes standard texture dimension (defined by the constant "textureDimension")
// totalSize parameter is set to size of texture including mipmaps.
static void* loadTexture(const char* fileName, uint32_t& totalSize)
{
	totalSize = 0;

	// Open file
	FILE* fp = fopen(fileName, "rb");
	if (!fp) {
		printf("cannot open file %s\n", fileName);
		return NULL;
	}

	// Get the file size
	fseek(fp, 0, SEEK_END);
	uint32_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	assert(fileSize == textureDimension*textureDimension*4);

	// Allocate memory for texture and mipmaps
	// (double texture buffer size to leave room for mipmaps)
	void* data = memalign(1024*1024, 2*fileSize);
	if (!data) {
		printf("can't allocate memory\n");
		return NULL;
	}
	totalSize = 2*fileSize;

	// Load texture
	fread(data, 1, fileSize, fp);
	fclose(fp);

	// Create mipmaps for the loaded texture
	buildMipmaps(data, (uint8_t*)data + fileSize, textureDimension, textureDimension);

	return data;
}

void SampleApp::CreateGeomJob(JobGeom256* job, CellGcmContextData *ctx, EdgeGeomPpuConfigInfo *info, void *skinningMatrices, uint32_t elephantIndex) const
{
	// Steam descriptor
	job->inputDmaList[kJobGeomDmaOutputStreamDesc].eal  = (uintptr_t)info->spuOutputStreamDesc;
	job->inputDmaList[kJobGeomDmaOutputStreamDesc].size = info->spuOutputStreamDescSize;

	// Indexes
	job->inputDmaList[kJobGeomDmaIndexes_0].eal  = (uintptr_t)info->indexes;
	job->inputDmaList[kJobGeomDmaIndexes_0].size = info->indexesSizes[0];
	job->inputDmaList[kJobGeomDmaIndexes_1].eal  = job->inputDmaList[kJobGeomDmaIndexes_0].eal + job->inputDmaList[kJobGeomDmaIndexes_0].size;
	job->inputDmaList[kJobGeomDmaIndexes_1].size = info->indexesSizes[1];

	// Skinning Matrices
	job->inputDmaList[kJobGeomDmaSkinMatrices_0].eal  = info->skinMatricesByteOffsets[0] + (uintptr_t)skinningMatrices;
	job->inputDmaList[kJobGeomDmaSkinMatrices_0].size = info->skinMatricesSizes[0];
	job->inputDmaList[kJobGeomDmaSkinMatrices_1].eal  = info->skinMatricesByteOffsets[1] + (uintptr_t)skinningMatrices;
	job->inputDmaList[kJobGeomDmaSkinMatrices_1].size = info->skinMatricesSizes[1];

	// Skinning Indexes & Weights
	job->inputDmaList[kJobGeomDmaSkinIndexesAndWeights_0].eal  = (uintptr_t)info->skinIndexesAndWeights;
	job->inputDmaList[kJobGeomDmaSkinIndexesAndWeights_0].size = info->skinIndexesAndWeightsSizes[0];
	job->inputDmaList[kJobGeomDmaSkinIndexesAndWeights_1].eal  = job->inputDmaList[kJobGeomDmaSkinIndexesAndWeights_0].eal + job->inputDmaList[kJobGeomDmaSkinIndexesAndWeights_0].size;
	job->inputDmaList[kJobGeomDmaSkinIndexesAndWeights_1].size = info->skinIndexesAndWeightsSizes[1];

	// Vertexes (stream 1)
	job->inputDmaList[kJobGeomDmaVertexes1_0].eal  = (uintptr_t)info->spuVertexes[0];
	job->inputDmaList[kJobGeomDmaVertexes1_0].size = info->spuVertexesSizes[0];
	job->inputDmaList[kJobGeomDmaVertexes1_1].eal  = job->inputDmaList[kJobGeomDmaVertexes1_0].eal + job->inputDmaList[kJobGeomDmaVertexes1_0].size;
	job->inputDmaList[kJobGeomDmaVertexes1_1].size = info->spuVertexesSizes[1];
	job->inputDmaList[kJobGeomDmaVertexes1_2].eal  = job->inputDmaList[kJobGeomDmaVertexes1_1].eal + job->inputDmaList[kJobGeomDmaVertexes1_1].size;
	job->inputDmaList[kJobGeomDmaVertexes1_2].size = info->spuVertexesSizes[2];

	// Vertexes (stream 2)
	job->inputDmaList[kJobGeomDmaVertexes2_0].eal  = (uintptr_t)info->spuVertexes[1];
	job->inputDmaList[kJobGeomDmaVertexes2_0].size = info->spuVertexesSizes[3];
	job->inputDmaList[kJobGeomDmaVertexes2_1].eal  = job->inputDmaList[kJobGeomDmaVertexes2_0].eal + job->inputDmaList[kJobGeomDmaVertexes2_0].size;
	job->inputDmaList[kJobGeomDmaVertexes2_1].size = info->spuVertexesSizes[4];
	job->inputDmaList[kJobGeomDmaVertexes2_2].eal  = job->inputDmaList[kJobGeomDmaVertexes2_1].eal + job->inputDmaList[kJobGeomDmaVertexes2_1].size;
	job->inputDmaList[kJobGeomDmaVertexes2_2].size = info->spuVertexesSizes[5];

	// Triangle Culling Data
	job->inputDmaList[kJobGeomDmaViewportInfo].eal  = (uintptr_t)&mViewportInfo;
	job->inputDmaList[kJobGeomDmaViewportInfo].size = sizeof(EdgeGeomViewportInfo);
	job->inputDmaList[kJobGeomDmaLocalToWorldMatrix].eal  = (uintptr_t)(&localToWorldMatrix[elephantIndex]);
	job->inputDmaList[kJobGeomDmaLocalToWorldMatrix].size = sizeof(EdgeGeomLocalToWorldMatrix);

	// SpuConfigInfo
	job->inputDmaList[kJobGeomDmaSpuConfigInfo].eal	 = (uintptr_t)info;
	job->inputDmaList[kJobGeomDmaSpuConfigInfo].size = sizeof(EdgeGeomSpuConfigInfo);

	// Software Fixed Point Format Offsets
	job->inputDmaList[kJobGeomDmaFixedPointOffsets1].eal  = (uintptr_t)info->fixedOffsets[0];
	job->inputDmaList[kJobGeomDmaFixedPointOffsets1].size = info->fixedOffsetsSize[0];
	job->inputDmaList[kJobGeomDmaFixedPointOffsets2].eal  =	(uintptr_t)info->fixedOffsets[1];
	job->inputDmaList[kJobGeomDmaFixedPointOffsets2].size = info->fixedOffsetsSize[1];

	// Input Streams Descriptors
	job->inputDmaList[kJobGeomDmaInputStreamDesc1].eal  = (uintptr_t)info->spuInputStreamDescs[0];
	job->inputDmaList[kJobGeomDmaInputStreamDesc1].size = info->spuInputStreamDescSizes[0];
	job->inputDmaList[kJobGeomDmaInputStreamDesc2].eal  = (uintptr_t)info->spuInputStreamDescs[1];
	job->inputDmaList[kJobGeomDmaInputStreamDesc2].size = info->spuInputStreamDescSizes[1];

	// --- User Data ---

	job->userData.eaOutputBufferInfo = (uintptr_t)&mOutputBufferInfo;

	// Command Buffer Hole
	uint32_t holeSize = info->spuConfigInfo.commandBufferHoleSize << 4;
	if(ctx->current + holeSize/4 + 3 > ctx->end) {
		if((*ctx->callback)(ctx, holeSize/4 + 3) != CELL_OK) {
			return;
		}
	}
	while(((uint32_t)ctx->current & 0xF) != 0) {
		*ctx->current++ = 0;
	}
	uint32_t holeEa = (uint32_t)ctx->current;
	uint32_t holeEnd = holeEa + holeSize;
	uint32_t jumpOffset;
	cellGcmAddressToOffset(ctx->current, &jumpOffset);
	cellGcmSetJumpCommandUnsafeInline(ctx, jumpOffset);
	uint32_t nextJ2S = ((uint32_t)ctx->current + 0x80) & ~0x7F;
	while(nextJ2S < holeEnd) {
		__dcbz( (void*)nextJ2S ); // prefetching here improves this loop's performance significantly
		ctx->current = (uint32_t*)nextJ2S;
		cellGcmAddressToOffset(ctx->current, &jumpOffset);
		cellGcmSetJumpCommandUnsafeInline(ctx, jumpOffset);
		nextJ2S = ((uint32_t)ctx->current + 0x80) & ~0x7F;
	}
	ctx->current = (uint32_t*)holeEnd;
	job->userData.eaCommandBufferHole = (uintptr_t)holeEa;

	// Blend shapes / shape count
	if(info->blendShapes) {
		job->userData.eaBlendShapeInfo = (uintptr_t)&shapeInfos[numShapeInfos];
		uint64_t numShapes = 0;
		for(uint32_t i = 0; i < info->numBlendShapes; ++i) {
			if(!info->blendShapes[i] || !shapeAlphas[i])
				continue;
			uint64_t shapeSize = (uint64_t)(info->blendShapeSizes[i]);
			shapeInfos[numShapeInfos].dmaTag = info->blendShapes[i] | (shapeSize << 32);
			shapeInfos[numShapeInfos].alpha = shapeAlphas[i];
			numShapeInfos++;
			numShapes++;
		}
		job->userData.blendShapeInfoCount = numShapes;
	}

	// Fragment program and patch data
	job->userData.fragmentProgramSize = toPatchSize;
	job->userData.eaFragmentProgram = (uint32_t)toPatchAddress;
	job->userData.eaPatchInfo = (uint32_t)&patchInfo;
	job->userData.eaConstantData = (uint32_t)(patchData + elephantIndex * 4 * MAX_PATCHES);

	job->header.eaBinary = (uintptr_t)_binary_job_job_geom_spu_bin_start;
	job->header.sizeBinary = CELL_SPURS_GET_SIZE_BINARY(_binary_job_job_geom_spu_bin_size);
	job->header.sizeDmaList = kJobGeomInputDmaEnd*8;
	job->header.eaHighInput = 0;
	job->header.useInOutBuffer = 1;
	job->header.sizeInOrInOut = info->ioBufferSize;
	//job->header.sizeOut = 0;
	job->header.sizeStack = 0;
	job->header.sizeScratch = info->scratchSizeInQwords;
	//job->header.eaHighCache = 0;
	job->header.sizeCacheDmaList = 0;

    // enable memory checking in jobs.  SPURS will place a memory check marker in 16 byte buffers added after IO buffer, read only data, and stack, and check the integrity
    // of the marker at the end of the job.  Enabling this flag will add 3-4% to SPURS overhead of running jobs.  Linking with -mspurs-job instead of -mspurs-job-initialize
    // will cause this flag to be ignored.
    job->header.jobType = CELL_SPURS_JOB_TYPE_MEMORY_CHECK;
}

SampleApp::SampleApp() :
mLastUpdate(FWTime::sZero),
mTurnRate(500.f),
mMoveRate(5.f),
mSkinningMatrices(NULL)
{
	mNear = 0.1f;
	mFar = 1000.f;
}

SampleApp::~SampleApp()
{
}

void SampleApp::initShader()
{
	mCGVertexProgram   = (CGprogram)(void*)&_binary_vpshader_vpo_start;
	mCGFragmentProgram = (CGprogram)(void*)&_binary_fpshader_fpo_start;

	// init
	cellGcmCgInitProgram(mCGVertexProgram);
	cellGcmCgInitProgram(mCGFragmentProgram);

	// allocate video memory for fragment program
	void *ucode;
	cellGcmCgGetUCode(mCGFragmentProgram, &ucode, &toPatchSize);
	toPatchAddress = memalign(16, toPatchSize);
	memcpy(toPatchAddress, ucode, toPatchSize);

	patchInfo.numToPatch = sizeof(toPatchParameters) / sizeof(toPatchParameters[0]);
	uint32_t *offsets = patchInfo.offsets;
	for (uint32_t i = 0; i < patchInfo.numToPatch; i++)
	{
		CGparameter toPatch = cellGcmCgGetNamedParameter(mCGFragmentProgram, toPatchParameters[i]);
		uint32_t count = cellGcmCgGetEmbeddedConstantCount(mCGFragmentProgram, toPatch);
		patchInfo.numInstances[i] = count;
		for (uint32_t j = 0; j < count; j++)
			*offsets++ = cellGcmCgGetEmbeddedConstantOffset(mCGFragmentProgram, toPatch, j);
	}

	for (uint32_t i = 0; i < numElephants; ++i)
		for (uint32_t j = 0; j < patchInfo.numToPatch; ++j)
			memcpy(&patchData[i * 4 * MAX_PATCHES + 4 * j], &toPatchValues[4 * (i % 4)], 16);

	uint32_t unused;
	cellGcmCgGetUCode(mCGVertexProgram, &mVertexProgramUCode, &unused);
}

bool SampleApp::onInit(int argc, char **ppArgv)
{
	FWGCMCamApplication::onInit(argc, ppArgv);	// note: calls sys_spu_initialize() internally

    /*
     * loading prx for libfs
     */
    int ret;
    ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
    if (ret != CELL_OK) {
        printf("cellSysmoduleLoadModule() error 0x%x !\n", ret);
        sys_process_exit(1);
    }

	FWInputDevice	*pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);
	if(pPad != NULL) {
		mpTriangle = pPad->bindFilter();
		mpTriangle->setChannel(FWInput::Channel_Button_Triangle);
		mpCircle = pPad->bindFilter();
		mpCircle->setChannel(FWInput::Channel_Button_Circle);
		mpCross = pPad->bindFilter();
		mpCross->setChannel(FWInput::Channel_Button_Cross);
		mpSquare = pPad->bindFilter();
		mpSquare->setChannel(FWInput::Channel_Button_Square);
		mpStart = pPad->bindFilter();
		mpStart->setChannel(FWInput::Channel_Button_Start);
		mpUp = pPad->bindFilter();
		mpUp->setChannel(FWInput::Channel_Button_Up);
		mpDown = pPad->bindFilter();
		mpDown->setChannel(FWInput::Channel_Button_Down);
		mpInputX0 = pPad->bindFilter();
		mpInputX0->setChannel(FWInput::Channel_XAxis_0);
		mpInputX0->setGain(0.5f);
		mpInputX0->setDeadzone(0.08f);
		mpInputY0 = pPad->bindFilter();
		mpInputY0->setChannel(FWInput::Channel_YAxis_0);
		mpInputY0->setGain(-0.5f);
		mpInputY0->setDeadzone(0.08f);
		mpInputX1 = pPad->bindFilter();
		mpInputX1->setChannel(FWInput::Channel_XAxis_1);
		mpInputX1->setGain(0.002f);
		mpInputX1->setDeadzone(0.0008f);
		mpInputY1 = pPad->bindFilter();
		mpInputY1->setChannel(FWInput::Channel_YAxis_1);
		mpInputY1->setGain(0.002f);
		mpInputY1->setDeadzone(0.0008f);
	}

	// Get PPU thread priority
	sys_ppu_thread_t my_ppu_thread_id;
	ret = sys_ppu_thread_get_id(&my_ppu_thread_id);
	if (ret != CELL_OK){
		printf("sys_ppu_thread_get_id failed : 0x%x\n", ret);
	}
	int ppu_thr_prio;
	ret = sys_ppu_thread_get_priority(my_ppu_thread_id, &ppu_thr_prio);
	if (ret != CELL_OK){
		printf("sys_ppu_thread_get_priority failed : 0x%x\n", ret);
		return false;
	}

	// Initialize spu_printf handling
	ret = spu_printf_initialize(ppu_thr_prio - 1, NULL);
    if (ret != CELL_OK){
        printf("spu_printf_initialize failed : 0x%x\n", ret);
		return false;
    }

	// Initialize a SPURS attribute structure.  This contains all the data that
	// was originally passed directly to cellSpursInitialize(), plus some additional
	// fields.
	CellSpursAttribute spursAttributes;
	ret = cellSpursAttributeInitialize(&spursAttributes, SPURS_SPU_NUM,
	SPU_THREAD_GROUP_PRIORITY, ppu_thr_prio - 1, false);
	if (ret != CELL_OK){
		printf("cellSpursAttributeInitialize failed : 0x%x\n", ret);
		return false;
	}
	// Enable support for spu_printf in the SPURS attributes.
	ret = cellSpursAttributeEnableSpuPrintfIfAvailable(&spursAttributes);
	if (ret != CELL_OK){
		printf("cellSpursAttributeEnableSpuPrintfItAvailable failed : 0x%x\n", ret);
		return false;
	}
	// Initialize SPURS using our attributes structure.
	ret = cellSpursInitializeWithAttribute(&mSpurs, &spursAttributes);
	if (ret != CELL_OK){
		printf("cellSpursInitializeWithAttribute failed : 0x%x\n", ret);
		return false;
	}

	// Initialize Spurs event flag (used for job chain completion)
	ret = cellSpursEventFlagInitializeIWL(&mSpurs, &mSpursEventFlag,
		CELL_SPURS_EVENT_FLAG_CLEAR_AUTO,
		CELL_SPURS_EVENT_FLAG_SPU2PPU);
	if (ret != CELL_OK){
		printf("cellSpursEventFlagInitializeIWL failed : 0x%x\n", ret);
		return false;
	}
	ret = cellSpursEventFlagAttachLv2EventQueue(&mSpursEventFlag);
	if (ret != CELL_OK){
		printf("cellSpursEventFlagAttachLv2EventQueue failed : 0x%x\n", ret);
		return false;
	}

	// Edge output buffers
	// mapped memory regions must be a multiple of 1 MB in size, and aligned to 1 MB
	mSharedOutputBuffer = (uint8_t*)memalign(1024*1024, EDGE_SHARED_OUTPUT_BUFFER_SIZE + FRAGMENT_OVERFETCH_BUFFER_SIZE);
	mRingOutputBuffer = (uint8_t*)memalign(1024*1024, SPURS_SPU_NUM * EDGE_RING_OUTPUT_BUFFER_SIZE + FRAGMENT_OVERFETCH_BUFFER_SIZE);
	uint32_t sharedBufferMapSize = (EDGE_SHARED_OUTPUT_BUFFER_SIZE + FRAGMENT_OVERFETCH_BUFFER_SIZE + 0xFFFFF) & ~0xFFFFF;
	uint32_t ringBufferMapSize = (SPURS_SPU_NUM * EDGE_RING_OUTPUT_BUFFER_SIZE + FRAGMENT_OVERFETCH_BUFFER_SIZE + 0xFFFFF) & ~0xFFFFF;
	uint32_t commandBufferMapSize = (COMMAND_BUFFER_SIZE + 0xFFFFF) & ~0xFFFFF;
	uint32_t dummy;
	if (mSharedOutputBuffer && sharedBufferMapSize) {
		cellGcmMapMainMemory(mSharedOutputBuffer, sharedBufferMapSize, &dummy);
	}
	if (mRingOutputBuffer && ringBufferMapSize) {
		cellGcmMapMainMemory(mRingOutputBuffer, ringBufferMapSize, &dummy);
	}

	// Command buffer
	mCommandBuffer = memalign(1024*1024, COMMAND_BUFFER_SIZE);
	cellGcmMapMainMemory(mCommandBuffer, commandBufferMapSize, &mCommandBufferOffset);

	// Textures
	uint32_t colorTexSize = 0;
	uint32_t normalTexSize = 0;
	// loadTexture allocates enough space after the texture for mipmaps and creates the mipmaps
	// The second parameter is set to the total texture size including mipmaps
	mTextures[0] = loadTexture(DIR_SOURCE_ASSETS"/elephant-color.bin", colorTexSize);
	mTextures[1] = loadTexture(DIR_SOURCE_ASSETS"/elephant-normal.bin", normalTexSize);
	cellGcmMapMainMemory(mTextures[0], colorTexSize, &dummy);
	cellGcmMapMainMemory(mTextures[1], normalTexSize, &dummy);

	// Skinning matrix
	uint32_t matrixFileSize = 0;
	mSkinningMatrices = (float*)loadFile(DIR_SOURCE_ASSETS"/elephant-matrices.bin", matrixFileSize);
	assert(matrixFileSize / (kNumBones * 12 * sizeof(float)) == kNumFrames);

	initShader();

	// Copy the Edge segment's RSX-only buffers into local memory, so that the RSX can see them.
	// This is something of a hack, as ideally you'd want to load this data directly into VRAM,
	// but it suffices for the purposes of this demo.
	uint8_t *geomBuffer = (uint8_t*)cellGcmUtilAllocateLocalMemory(1024*1024, 16);
	uint8_t *nextSegmentGeom = geomBuffer;
	for(uint32_t iSegment = 0; iSegment < sizeof(allSegments) / sizeof(allSegments[0]); ++iSegment)
	{
		// Skip this segment if it doesn't have a RSX-only stream
		if (allSegments[iSegment]->rsxOnlyVertexesSize == 0)
			continue;
		memcpy(nextSegmentGeom, allSegments[iSegment]->rsxOnlyVertexes,
			allSegments[iSegment]->rsxOnlyVertexesSize);
		// Update the segment's rsxOnlyVertexes pointer to reference the VRAM copy of the data.
		// NOTE: This leaks a pointer to the original rsxOnlyVertexes in main memory, but due
		// to the implementation of this sample it would be impossible to free it.
		allSegments[iSegment]->rsxOnlyVertexes = nextSegmentGeom;
		nextSegmentGeom += allSegments[iSegment]->rsxOnlyVertexesSize;
	}

	// set default camera position
	mCamera.setPosition(Point3(-3.5f, 1.94f, 13.08f));

	// Initialize output buffer info.
	// Basic rules:
	//  - If the ring buffer size (end-start) is zero, use the shared buffer exlusively.
	//  - If the shared buffer size (end-start) is zero, use the ring buffer exclusively.
	//  - If both buffer sizes are nonzero, then use the ring buffer primarily,
	//    falling back on the shared buffer for overflow if the ring buffer is
	//    temporarily full.

	// shared buffer info
	memset(&mOutputBufferInfo, 0, sizeof(mOutputBufferInfo));
	mOutputBufferInfo.sharedInfo.startEa = (uint32_t)mSharedOutputBuffer;
	mOutputBufferInfo.sharedInfo.endEa = mOutputBufferInfo.sharedInfo.startEa
		+ EDGE_SHARED_OUTPUT_BUFFER_SIZE;
	mOutputBufferInfo.sharedInfo.currentEa = mOutputBufferInfo.sharedInfo.startEa;
	mOutputBufferInfo.sharedInfo.locationId = CELL_GCM_LOCATION_MAIN;
	mOutputBufferInfo.sharedInfo.failedAllocSize = 0;
	cellGcmAddressToOffset((void*)mOutputBufferInfo.sharedInfo.startEa,
		&mOutputBufferInfo.sharedInfo.startOffset);
	// ring buffer info
	for(uint32_t iSpu=0; iSpu<SPURS_SPU_NUM; ++iSpu) {
		// Each SPU must have its own separate ring buffer, with no overlap.
		mOutputBufferInfo.ringInfo[iSpu].startEa = (uint32_t)mRingOutputBuffer
			+ iSpu * EDGE_RING_OUTPUT_BUFFER_SIZE;
		mOutputBufferInfo.ringInfo[iSpu].endEa = mOutputBufferInfo.ringInfo[iSpu].startEa
			+ EDGE_RING_OUTPUT_BUFFER_SIZE;
		mOutputBufferInfo.ringInfo[iSpu].currentEa = mOutputBufferInfo.ringInfo[iSpu].startEa;
		mOutputBufferInfo.ringInfo[iSpu].locationId = CELL_GCM_LOCATION_MAIN;
		mOutputBufferInfo.ringInfo[iSpu].rsxLabelEa = (uint32_t)cellGcmGetLabelAddress(kFirstRsxLabelIndex + iSpu);
        mOutputBufferInfo.ringInfo[iSpu].cachedFree = 0;
		cellGcmAddressToOffset((void*)mOutputBufferInfo.ringInfo[iSpu].startEa,
			&mOutputBufferInfo.ringInfo[iSpu].startOffset);
		// RSX label value must be initialized to the end of the buffer
		*(uint32_t *)mOutputBufferInfo.ringInfo[iSpu].rsxLabelEa = mOutputBufferInfo.ringInfo[iSpu].endEa;
	}

	// Increase command buffer hole size for additional fragment program commands
    for (uint32_t i = 0; i < sizeof(allSegments) / sizeof(allSegments[0]); i++)
    {
		//2 Words(8 bytes) for the cellGcmSetUpdateFragmentProgramParameter command
		//2 Words(8 bytes) for the cellGcmSetInvalidateTextureCache command
		allSegments[i]->spuConfigInfo.commandBufferHoleSize += 1;

		//1 Quadword(16 bytes) to set the rsx label for the ring buffer output for the patched programs
		allSegments[i]->spuConfigInfo.commandBufferHoleSize += 1;
	}

	// Give each elephant a random animation frame offset
	for(uint32_t i=0; i<numElephants; ++i)
	{
		elephantAnimFrameOffsets[i] = rand() % kNumFrames;
	}

	// init profile bars
	ProfileBars::Init(&mSpurs, SPURS_SPU_NUM);

	return true;
}

bool SampleApp::onUpdate()
{
	FWTimeVal time = FWTime::getCurrentTime();

	float deltaTime;
	if(mLastUpdate == FWTime::sZero) {
		deltaTime = 0.f;
		mLastUpdate = time;
	}
	else {
		deltaTime = (float)(time - mLastUpdate);
		mLastUpdate = time;
	}

	mCamera.setPan(mCamera.getPan() + (mpInputX1 ? mpInputX1->getFloatValue() : 0.f) * deltaTime * mTurnRate);
	mCamera.setTilt(mCamera.getTilt() + (mpInputY1 ? mpInputY1->getFloatValue() : 0.f) * deltaTime * mTurnRate);

	Matrix3 xRot = Matrix3::rotationX(-mCamera.getTilt());
	Matrix3 yRot = Matrix3::rotationY(-mCamera.getPan());
	Vector3 xVec = (yRot * xRot) * Vector3::xAxis();
	Vector3 yVec = (yRot * xRot) * Vector3::yAxis();
	Vector3 zVec = (yRot * xRot) * -Vector3::zAxis();

	mCamera.setPosition(mCamera.getPosition() + (
		  xVec * (mpInputX0 ? mpInputX0->getFloatValue() : 0.f)
		+ yVec * ((mpUp && mpUp->getBoolValue()) ? 1.0f : 0.f)
		+ yVec * ((mpDown && mpDown->getBoolValue()) ? -1.0f : 0.f)
		+ zVec * (mpInputY0 ? mpInputY0->getFloatValue() : 0.f)
		) * deltaTime * mMoveRate);

	return FWGCMCamApplication::onUpdate();
}

void SampleApp::onRender()
{
	static int ffframe = 0;
	ffframe++;
	if(ffframe ==2)
	{
		cellGcmFinish(0x13113);
		exit(0);
	}
	// time stamp 0 is the base rsx time used by the profile bars for this frame
	// wait for vsync and write the timestamp
	ProfileBars::StartFrame();
	cellGcmSetTimeStamp(0);
	cellGcmFlush();

	// base implementation clears screen and sets up camera
	FWGCMCamApplication::onRender();

	cellGcmSetInvalidateVertexCache();

	CellGcmTexture tex;
	uint32_t format, remap;
	cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_RGBA8, &format, &remap, 0, 1);

	tex.format = format;
	tex.mipmap = 10;
	tex.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	tex.cubemap = CELL_GCM_FALSE;
	tex.remap = remap;
	tex.width = textureDimension;
	tex.height = textureDimension;
	tex.depth = 1;
	tex.pitch = textureDimension*4;
	tex.location = CELL_GCM_LOCATION_MAIN;
	tex.offset = 0;

	cellGcmSetTextureControl(0, CELL_GCM_TRUE, 0<<8, 12<<8, CELL_GCM_TEXTURE_MAX_ANISO_1); // MIN:0,MAX:12
	cellGcmSetTextureAddress(0,	CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
	cellGcmSetTextureFilter(0, 0, CELL_GCM_TEXTURE_NEAREST_LINEAR, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);
	cellGcmSetTextureControl(1, CELL_GCM_TRUE, 0<<8, 12<<8, CELL_GCM_TEXTURE_MAX_ANISO_1); // MIN:0,MAX:12
	cellGcmSetTextureAddress(1,	CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
	cellGcmSetTextureFilter(1, 0, CELL_GCM_TEXTURE_NEAREST_LINEAR, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);
	cellGcmSetVertexProgram(mCGVertexProgram, mVertexProgramUCode);
	cellGcmSetFragmentProgram(mCGFragmentProgram, mFragmentProgramOffset);

	cellGcmSetBlendEnable(CELL_GCM_FALSE);
	cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
	cellGcmSetDepthFunc(CELL_GCM_LESS);
	cellGcmSetShadeMode(CELL_GCM_SMOOTH);

	const FWDisplayInfo& rDispInfo = getDisplayInfo();

	mViewportInfo.scissorArea[0] = 0;
	mViewportInfo.scissorArea[1] = 0;
	mViewportInfo.scissorArea[2] = rDispInfo.mWidth;
	mViewportInfo.scissorArea[3] = rDispInfo.mHeight;

    mViewportInfo.depthRange[0] = 0.0f;
    mViewportInfo.depthRange[1] = 1.0f;

    mViewportInfo.viewportScales[0] = mViewportInfo.scissorArea[2] * .5f;
    mViewportInfo.viewportScales[1] = mViewportInfo.scissorArea[3] * -.5f;
    mViewportInfo.viewportScales[2] = (mViewportInfo.depthRange[1] - mViewportInfo.depthRange[0]) * .5f;
    mViewportInfo.viewportScales[3] = 0.0f;

    mViewportInfo.viewportOffsets[0] = mViewportInfo.scissorArea[0] + mViewportInfo.viewportScales[0];
    mViewportInfo.viewportOffsets[1] = mViewportInfo.scissorArea[1] + mViewportInfo.scissorArea[3] * .5f;
    mViewportInfo.viewportOffsets[2] = (mViewportInfo.depthRange[1] + mViewportInfo.depthRange[0]) * .5f;
    mViewportInfo.viewportOffsets[3] = 0.0f;

    mViewportInfo.sampleFlavor = CELL_GCM_SURFACE_CENTER_1;

	Matrix4 vp = transpose(getProjectionMatrix() * getViewMatrix());
	memcpy(mViewportInfo.viewProjectionMatrix, &vp, 64);
	uint32_t texOff[2];
	cellGcmAddressToOffset(mTextures[0], &texOff[0]);
	cellGcmAddressToOffset(mTextures[1], &texOff[1]);

	tex.offset = texOff[0];
	cellGcmSetTexture(0, &tex);
	tex.offset = texOff[1];
	cellGcmSetTexture(1, &tex);

	// Reset Parameters
	int numSegments = 0;
	numShapeInfos = 0;
	mOutputBufferInfo.sharedInfo.currentEa = mOutputBufferInfo.sharedInfo.startEa;
	if(mOutputBufferInfo.sharedInfo.failedAllocSize != 0) {
		// When using hybrid buffers, the allocations that "failed" did not actually
		// fail fatally.  What it means is that the shared buffer was full, so the SPUs
		// had to wait for the RSX to consume more data before the allocation could complete.
		//
		// This is an important warning (it means your output buffers should probably be a
		// little larger), but certainly not worth spewing output every frame about failed allocations.
		//
        //printf("Edge: %d bytes of allocations could not be fulfilled\n",
        //    mOutputBufferInfo.sharedInfo.failedAllocSize);
	}
	mOutputBufferInfo.sharedInfo.failedAllocSize = 0;

	CellGcmContextData pbContext;
	pbContext.current = (uint32_t*)mCommandBuffer;
	pbContext.end = (uint32_t*)((uint32_t)mCommandBuffer + COMMAND_BUFFER_SIZE - 4);
	pbContext.begin = (uint32_t*)mCommandBuffer;
	pbContext.callback = 0;

	static CellSpursJob256 jobEnd;
	__builtin_memset(&jobEnd,0,sizeof(CellSpursJob256));
	jobEnd.header.eaBinary     = (uintptr_t)_binary_job_job_send_event_spu_bin_start;
	jobEnd.header.sizeBinary   = CELL_SPURS_GET_SIZE_BINARY(_binary_job_job_send_event_spu_bin_size);
	jobEnd.workArea.userData[0] = (uintptr_t)&mSpursEventFlag;
	jobEnd.workArea.userData[1] = (uint64_t)EVENT_FLAG_BITS;

	static int frame = 0;
	++frame;
	for (uint32_t iElephant = 0; iElephant < numElephants; iElephant++)
	{
		Matrix4 mat = Matrix4::rotationZYX(Vector3(0, 0, 0));
		mat.setTranslation(Vector3(0.0f + 5.0f * (float)iElephant, 0.0f, -5.0f));
		Matrix4 mvp = transpose(getProjectionMatrix() * getViewMatrix() * mat);
		float lp[4] = {0,25,100,1};
		Vector4 objSpaceEyePos = inverse(mat) * mCamera.getPosition();
		float ep[4] = {objSpaceEyePos[0], objSpaceEyePos[1], objSpaceEyePos[2], 1};
		cellGcmSetVertexProgramConstants(&pbContext, 0, 16, (float*)&mvp);
		cellGcmSetVertexProgramConstants(&pbContext, 4, 4, lp);
		cellGcmSetVertexProgramConstants(&pbContext, 5, 4, ep);

		Matrix4 m = transpose(mat);
		memcpy(&localToWorldMatrix[iElephant].matrixData, &m, 48);

		for(uint32_t iSegment = 0; iSegment < sizeof(allSegments) / sizeof(allSegments[0]); ++iSegment)
		{
			// Bind the RSX-only vertex attributes for this job before it's created
			if (allSegments[iSegment]->rsxOnlyVertexesSize > 0)
			{
				EdgeGeomVertexStreamDescription *rsxOnlyStreamDesc = (EdgeGeomVertexStreamDescription*)(allSegments[iSegment]->rsxOnlyStreamDesc);
				uint32_t rsxVertexesOffset = 0;
				cellGcmAddressToOffset(allSegments[iSegment]->rsxOnlyVertexes, &rsxVertexesOffset);
				for(uint32_t iRsxAttr=0; iRsxAttr < rsxOnlyStreamDesc->numAttributes; ++iRsxAttr)
				{
					EdgeGeomAttributeBlock &attr = rsxOnlyStreamDesc->blocks[iRsxAttr].attributeBlock;
					cellGcmSetVertexDataArray(&pbContext, attr.vertexProgramSlotIndex, 0, rsxOnlyStreamDesc->stride,
						attr.componentCount, attr.format, CELL_GCM_LOCATION_LOCAL,
						rsxVertexesOffset + attr.offset);
				}
			}

			JobGeom256 *job = (JobGeom256*) &jobs[numSegments++];
			CreateGeomJob(job, &pbContext, (EdgeGeomPpuConfigInfo*)allSegments[iSegment], &mSkinningMatrices[kNumBones*12*((frame + elephantAnimFrameOffsets[iElephant])%kNumFrames)], iElephant);

			int ret = cellSpursCheckJob((CellSpursJob256*)job, sizeof(JobGeom256),
				sizeof(CellSpursJob256));
			if ( ret != CELL_OK){
				printf("cellSpursCheckJob failed : 0x%x\n", ret);
			}
		}
	}

	static CellSpursJobList jobList;
	jobList.eaJobList = (uint64_t)jobs;
	jobList.numJobs = numSegments;
	assert(sizeof(JobGeom256)==sizeof(jobs[0]));
	jobList.sizeOfJob = sizeof(JobGeom256);

	static uint64_t command_list[5];
	command_list[0] = CELL_SPURS_JOB_COMMAND_JOBLIST(&jobList);
	command_list[1] = CELL_SPURS_JOB_COMMAND_SYNC;
	command_list[2] = CELL_SPURS_JOB_COMMAND_FLUSH;
	command_list[3] = CELL_SPURS_JOB_COMMAND_JOB(&jobEnd);
	command_list[4] = CELL_SPURS_JOB_COMMAND_END;

	static CellSpursJobChain jobChain;
    static CellSpursJobChainAttribute      jobChainAttributes;
	static uint8_t prios[8] = {1, 1, 1, 1, 1, 1, 1, 1};

	int ret = cellSpursJobChainAttributeInitialize(
		&jobChainAttributes, command_list, sizeof(CellSpursJob256),
		(SPURS_SPU_NUM == 1) ? 1 : 16, prios, SPURS_SPU_NUM,
		true, 0, 1, false, sizeof(CellSpursJob256), 6);
	if (ret != CELL_OK){
		printf("cellSpursJobChainAttributeInitialize failed : 0x%x\n", ret);
	}
	ret = cellSpursJobChainAttributeSetName(&jobChainAttributes, "geom-jobchain");
	if (ret != CELL_OK){
		printf("cellSpursJobChainAttributeSetName failed : 0x%x\n", ret);
	}
	//When the job manager detects an error, halt on SPU to help with debugging
	ret = cellSpursJobChainAttributeSetHaltOnError(&jobChainAttributes);
	if (ret != CELL_OK){
		printf("cellSpursJobChainAttributeSetHaltOnError failed : 0x%x\n", ret);
	}
	ret = cellSpursCreateJobChainWithAttribute(&mSpurs, &jobChain, &jobChainAttributes);
	if (ret != CELL_OK){
		printf("cellSpursCreateJobChainWithAttribute failed : 0x%x\n", ret);
	}
	ret = cellSpursRunJobChain(&jobChain);
	if (ret != CELL_OK){
		printf("cellSpursRunJobChain failed : 0x%x\n", ret);
	}

	cellGcmSetReturnCommand(&pbContext);

	ProfileBars::AddRsxMarker(0x20202000);		// grey rsx bar until that point (clear etc)
	cellGcmSetCallCommand(mCommandBufferOffset);
	ProfileBars::AddRsxMarker(0xff000000);		// red rsx bar indicates draw time

	// kick rsx
    cellGcmFlush();

    uint16_t ev_mask = EVENT_FLAG_BITS;
	ret = cellSpursEventFlagWait(&mSpursEventFlag,&ev_mask,CELL_SPURS_EVENT_FLAG_AND);
    if (ret != CELL_OK){
        printf("cellSpursEventFlagWait failed : 0x%x\n", ret);
    }
	ret = cellSpursShutdownJobChain(&jobChain);
    if (ret != CELL_OK){
        printf("cellSpursShutdownJobChain failed : 0x%x\n", ret);
    }
	ret = cellSpursJoinJobChain(&jobChain);
    if (ret != CELL_OK){
        printf("cellSpursJoinJobChain failed : 0x%x\n", ret);
    }

	// cellGcmFinish is only there because of profile bars displayed for the current frame.
	ProfileBars::AddPpuMarker(0x0000ff00);
	static uint32_t finishReference = 0;
	cellGcmFinish(finishReference++);
	//ProfileBars::Render();

	{
		// calc fps
		FWTimeVal	time = FWTime::getCurrentTime();
		//float fFPS = 1.f / (float)(time - mLastTime);
		mLastTime = time;

		// print some messages
		FWDebugFont::setPosition(0, 0);
		FWDebugFont::setColor(1.f, 1.f, 1.f, 1.0f);

		//FWDebugFont::print("Edge Fragment Patch Sample Application\n\n");
		//FWDebugFont::printf("FPS: %.2f\n", fFPS);
	}
}

void SampleApp::onSize(const FWDisplayInfo& rDispInfo)
{
	FWGCMCamApplication::onSize(rDispInfo);
}

void SampleApp::onShutdown()
{
	int ret;
	ret = cellSpursEventFlagDetachLv2EventQueue(&mSpursEventFlag);
	if (ret != CELL_OK) {
		printf("cellSpursEventFlagDetachLv2EventQueue failed : 0x%x\n", ret);
	}
	ret = cellSpursFinalize(&mSpurs);
	if (ret != CELL_OK){
		printf("cellSpursFinalize failed : 0x%x\n", ret);
	}
	ret = spu_printf_finalize();
	if (ret != CELL_OK){
		printf("spu_printf_finalize failed : 0x%x\n", ret);
	}

	free( toPatchAddress );
	free( mSharedOutputBuffer );
	free( mRingOutputBuffer );
	free( mCommandBuffer );
	free( mSkinningMatrices );
	free( mTextures[0] );
	free( mTextures[1] );

	FWGCMCamApplication::onShutdown();

	// unbind input filters
	FWInputDevice	*pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);

	if(pPad != NULL)  {
		pPad->unbindFilter(mpTriangle);
		pPad->unbindFilter(mpCircle);
		pPad->unbindFilter(mpCross);
		pPad->unbindFilter(mpSquare);
		pPad->unbindFilter(mpUp);
		pPad->unbindFilter(mpDown);
		pPad->unbindFilter(mpStart);
		pPad->unbindFilter(mpInputX0);
		pPad->unbindFilter(mpInputY0);
		pPad->unbindFilter(mpInputX1);
		pPad->unbindFilter(mpInputY1);
	}
}
