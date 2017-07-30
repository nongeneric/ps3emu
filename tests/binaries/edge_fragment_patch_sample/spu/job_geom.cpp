/* SCE CONFIDENTIAL
 * PlayStation(R)Edge 1.3.0
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <spu_intrinsics.h>
#include <cell/spurs/common.h>
#include <cell/spurs/job_chain.h>
#include <cell/spurs/job_context.h>
#include <cell/atomic.h>
#include <cell/dma.h>
#include <cell/gcm_spu.h>

#include "edge/edge_assert.h"
#include "edge/edge_printf.h"
#include "edge/geom/edgegeom.h"
#include "job_geom_interface.h"

#define MAX_PATCHES 16
struct PatchInfo
{
	uint32_t numToPatch;
	uint32_t pad[3];
	uint32_t numInstances[MAX_PATCHES];
	uint32_t offsets[MAX_PATCHES];
};

extern "C" 
{
	void cellSpursJobMain2(CellSpursJobContext2* stInfo,  CellSpursJob256 *job256)
	{
		const JobGeom256 *job = (const JobGeom256 *) job256;

		// Input buffers
		// Note that these are pointers into the input buffer, and
		// their lifetime is limited by the current stage of
		// processing.  For example, you can not assume that the
		// vertexesA pointer will actually point to input vertex data
		// after calling DecompressVertexes()!
		void* inputsPointers[kJobGeomInputDmaEnd] __attribute__((aligned(16)));
		cellSpursJobGetPointerList(inputsPointers, &job->header, stInfo);

		// Extract matrix count from dma size
		uint32_t totalMatrixCount  = (job->inputDmaList[kJobGeomDmaSkinMatrices_0].size
			+ job->inputDmaList[kJobGeomDmaSkinMatrices_1].size) / 48;

		// Extract additional values from the userData area.
		uint32_t outputBufferInfoEa = job->userData.eaOutputBufferInfo;
		uint32_t holeEa            	= job->userData.eaCommandBufferHole;
		uint32_t numBlendShapes    	= job->userData.blendShapeInfoCount;
		uint32_t blendShapeInfosEa 	= job->userData.eaBlendShapeInfo;
		uint32_t fragmentProgramSize =job->userData.fragmentProgramSize;
		uint32_t fragmentProgramEa  = job->userData.eaFragmentProgram;
		uint32_t patchInfoAddress   = job->userData.eaPatchInfo;
		uint32_t constantDataEa     = job->userData.eaConstantData;

        uint8_t fragmentProgram[fragmentProgramSize] __attribute__((__aligned__(16)));
		PatchInfo patchInfo __attribute__((__aligned__(16)));
		float patchData[4 * MAX_PATCHES] __attribute__((__aligned__(16)));
		if(fragmentProgramEa) 
		{
			//let's start the DMA before we do anything else.
			bool interruptsEnabled = spu_readch(SPU_RdMachStat) & 1;
			spu_idisable();

			// dma in the PatchInfo structure
			spu_writech(MFC_LSA, (uint32_t)&patchInfo);
			spu_writech(MFC_EAL, patchInfoAddress);
			spu_writech(MFC_Size, sizeof(PatchInfo));
			spu_writech(MFC_TagID, stInfo->dmaTag);
			spu_writech(MFC_Cmd, MFC_GET_CMD);

			// dma in the fragment ucode to local store
			spu_writech(MFC_LSA, (uint32_t)fragmentProgram);
			spu_writech(MFC_EAL, fragmentProgramEa);
			spu_writech(MFC_Size, fragmentProgramSize);
			spu_writech(MFC_TagID, stInfo->dmaTag);
			spu_writech(MFC_Cmd, MFC_GET_CMD);

			// dma in the constant values
			spu_writech(MFC_LSA, (uint32_t)&patchData);
			spu_writech(MFC_EAL, constantDataEa);
			spu_writech(MFC_Size, sizeof(patchData));
			spu_writech(MFC_TagID, stInfo->dmaTag);
			spu_writech(MFC_Cmd, MFC_GET_CMD);

			if(interruptsEnabled)
				spu_ienable();
		}

		// If only one stream was passed (i.e. the second stream size is 0), we need to explicitly zero
		// out the secondary vertex stream pointers.
		bool hasSecondStream = (job->inputDmaList[kJobGeomDmaVertexes2_0].size
			+ job->inputDmaList[kJobGeomDmaVertexes2_1].size
			+ job->inputDmaList[kJobGeomDmaVertexes2_2].size) > 0;
		if (!hasSecondStream)
		{
			inputsPointers[kJobGeomDmaVertexes2] = 0;
			inputsPointers[kJobGeomDmaInputStreamDesc2] = 0;
			inputsPointers[kJobGeomDmaFixedPointOffsets2] = 0;
		}
		
		// Validate buffer order
		uint32_t errCode = edgeGeomValidateBufferOrder(
			inputsPointers[kJobGeomDmaOutputStreamDesc], 
			inputsPointers[kJobGeomDmaIndexes], 
			inputsPointers[kJobGeomDmaSkinMatrices], 
			inputsPointers[kJobGeomDmaSkinIndexesAndWeights], 
			inputsPointers[kJobGeomDmaVertexes1],
			inputsPointers[kJobGeomDmaVertexes2], 
			inputsPointers[kJobGeomDmaViewportInfo], 
			inputsPointers[kJobGeomDmaLocalToWorldMatrix], 
			inputsPointers[kJobGeomDmaSpuConfigInfo],
			inputsPointers[kJobGeomDmaFixedPointOffsets1], 
			inputsPointers[kJobGeomDmaFixedPointOffsets2], 
			inputsPointers[kJobGeomDmaInputStreamDesc1],
			inputsPointers[kJobGeomDmaInputStreamDesc2]);
		EDGE_ASSERT_MSG( errCode == 0, ("Buffer order incorrect; errCode = 0x%08X\n", errCode) );
		(void)errCode;

		EdgeGeomCustomVertexFormatInfo customFormatInfo = 
		{
			(EdgeGeomVertexStreamDescription*) inputsPointers[kJobGeomDmaInputStreamDesc1],
			(EdgeGeomVertexStreamDescription*) inputsPointers[kJobGeomDmaInputStreamDesc2],
			(EdgeGeomVertexStreamDescription*) inputsPointers[kJobGeomDmaOutputStreamDesc],
			0,
			0,0,0,0,0,0
		};

		EdgeGeomSpuContext ctx;

		// Edge processing begins here
		edgeGeomInitialize(&ctx, 
			(const EdgeGeomSpuConfigInfo*) inputsPointers[kJobGeomDmaSpuConfigInfo], 
			stInfo->sBuffer, job->header.sizeScratch << 4, 
			stInfo->ioBuffer, job->header.sizeInOrInOut,            
            stInfo->dmaTag, 
			(const EdgeGeomViewportInfo*) inputsPointers[kJobGeomDmaViewportInfo], 
			(const EdgeGeomLocalToWorldMatrix*) inputsPointers[kJobGeomDmaLocalToWorldMatrix], 
            &customFormatInfo);

		// Initialize Edge's free pointer to the end of the input
		// data. Not strictly necessary, but it can make certain
		// problems easier to debug.
		uint32_t lastInputListIndex = (job256->header.sizeDmaList / sizeof(uint64_t)) - 1;
		while(inputsPointers[lastInputListIndex] == 0)
			--lastInputListIndex;
		uint32_t inputDataEnd = (intptr_t)inputsPointers[lastInputListIndex]
			+ (job256->workArea.dmaList[lastInputListIndex]>>32); // LS start + size
		edgeGeomSetFreePtr(&ctx, (uint8_t*)inputDataEnd);

        edgeGeomDecompressVertexes(&ctx, inputsPointers[kJobGeomDmaVertexes1], 
			inputsPointers[kJobGeomDmaFixedPointOffsets1],
			inputsPointers[kJobGeomDmaVertexes2], 
			inputsPointers[kJobGeomDmaFixedPointOffsets2]);
		edgeGeomProcessBlendShapes(&ctx, numBlendShapes, blendShapeInfosEa);
		edgeGeomSkinVertexes(&ctx, inputsPointers[kJobGeomDmaSkinMatrices], totalMatrixCount, inputsPointers[kJobGeomDmaSkinIndexesAndWeights]);
		edgeGeomDecompressIndexes(&ctx, inputsPointers[kJobGeomDmaIndexes]);

        uint32_t numVisibleIdxs = edgeGeomCullTriangles(&ctx, EDGE_GEOM_CULL_BACKFACES_AND_FRUSTUM);

        // Get space for the program and the geom in a single allocation

        if (fragmentProgramEa || numVisibleIdxs) 
        {
            if (fragmentProgramEa)
            {
                // Wait for the fragment program information to be done DMAing
                bool interruptsEnabled = spu_readch(SPU_RdMachStat) & 1;
                spu_idisable();
                spu_writech(MFC_WrTagMask, 1 << stInfo->dmaTag);
                spu_writech(MFC_WrTagUpdate, 2);
                spu_readch(MFC_RdTagStat);
                if(interruptsEnabled)
                    spu_ienable();
            }

            //allocate
            uint32_t indexAllocSize = numVisibleIdxs ? edgeGeomCalculateDefaultOutputSize(&ctx, numVisibleIdxs) : 0;
            uint32_t fragmentProgramAllocSize = fragmentProgramEa ? (fragmentProgramSize + 0x7F) & ~0x7F : 0;

            uint32_t allocSize = indexAllocSize + fragmentProgramAllocSize;
            
            EdgeGeomAllocationInfo info;
            if (edgeGeomAllocateOutputSpace(&ctx, outputBufferInfoEa, allocSize, &info, cellSpursGetCurrentSpuId()))
            { 
                uint32_t fragmentProgramOffset = 0;

                if (fragmentProgramEa)
                {
                    // Patch the fragment program constants
                    for (uint32_t i = 0; i < patchInfo.numToPatch; i++) 
                    {
                        uint32_t *offsets = patchInfo.offsets;
                        for (uint32_t j = 0; j < patchInfo.numInstances[i]; j++) 
                        {
                            qword patch = si_roti(si_lqd(si_from_ptr(patchData + 4 * i), 0x0), 16);
                            si_stqx(patch, si_from_ptr(fragmentProgram), si_from_uint(*offsets++));
                        }
                    }

                    uint32_t outputEa;

                    edgeGeomUseOutputSpace(&info, fragmentProgramAllocSize, &outputEa, &fragmentProgramOffset);

                    outputEa = (outputEa + 0x7F) & ~0x7F;

                    // Write the patched program
                    bool interruptsEnabled = spu_readch(SPU_RdMachStat) & 1;
                    spu_idisable();

                    spu_writech(MFC_LSA, (uint32_t)fragmentProgram);
                    spu_writech(MFC_EAL, outputEa);
                    spu_writech(MFC_Size, fragmentProgramSize);
                    spu_writech(MFC_TagID, stInfo->dmaTag);
                    spu_writech(MFC_Cmd, MFC_PUT_CMD);

                    spu_writech(MFC_WrTagMask, 1 << stInfo->dmaTag);
                    spu_writech(MFC_WrTagUpdate, 2);
                    spu_readch(MFC_RdTagStat);

                    if(interruptsEnabled)
                        spu_ienable();
                }

                EdgeGeomLocation idx;
                EdgeGeomLocation vtx;

                if (numVisibleIdxs) 
                {
                    edgeGeomOutputIndexes(&ctx, numVisibleIdxs, &info, &idx);
                    edgeGeomCompressVertexes(&ctx);
                    edgeGeomOutputVertexes(&ctx, &info, &vtx);
                }

                CellGcmContextData gcmCtx;
                edgeGeomBeginCommandBufferHole(&ctx, &gcmCtx, holeEa, &info, 1);
                // Note: If any of the push buffer contents are changed from this sequence, the
                // spuConfigInfo.holeSize must be updated to reflect those changes.
                if (fragmentProgramEa)
                {
                    if (edgeGeomIsAllocatedFromRingBuffer(&info))
                        cellGcmSetInvalidateTextureCacheUnsafeInline(&gcmCtx, CELL_GCM_INVALIDATE_TEXTURE);

                    if (numVisibleIdxs)
                        cellGcmSetUpdateFragmentProgramParameterLocationUnsafeInline(&gcmCtx, fragmentProgramOffset, info.location);	
                }
                
                if (numVisibleIdxs) {
                    edgeGeomSetVertexDataArrays(&ctx, &gcmCtx, &vtx);
                    cellGcmSetDrawIndexArrayUnsafeInline(&gcmCtx, CELL_GCM_PRIMITIVE_TRIANGLES,
                                                         numVisibleIdxs, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, 
                                                         idx.location, idx.offset);
                }
                edgeGeomEndCommandBufferHole(&ctx, &gcmCtx, holeEa, &info, 1);
                return;
            }
        }
		
        CellGcmContextData gcmCtx;
		edgeGeomBeginCommandBufferHole(&ctx, &gcmCtx, holeEa, 0, 0);
        edgeGeomEndCommandBufferHole(&ctx, &gcmCtx, holeEa, 0, 0);
	}
}

