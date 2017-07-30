/* SCE CONFIDENTIAL
 * PlayStation(R)Edge 1.3.0
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef JOB_GEOM_INTERFACE_H
#define JOB_GEOM_INTERFACE_H

#include <cell/spurs/job_descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

// Input DMA list elements - all items larger than 16KB are split in multiple DMA list elements
enum JobGeomInputDma {
	kJobGeomInputDmaStart, 
		kJobGeomDmaOutputStreamDesc = kJobGeomInputDmaStart,						// output stream descriptor
		kJobGeomDmaIndexes,															// indices = up to 2 x 16KB = 32KB total
			kJobGeomDmaIndexes_0 = kJobGeomDmaIndexes,								// first list element
			kJobGeomDmaIndexes_1,													// indices - second list element = up to 32KB total
		kJobGeomDmaSkinMatrices,													// skinning matrices = up to 32KB total
			kJobGeomDmaSkinMatrices_0 = kJobGeomDmaSkinMatrices,					// skinning matrices - first list element
			kJobGeomDmaSkinMatrices_1,												// skinning matrices - second list element
		kJobGeomDmaSkinIndexesAndWeights,											// skinning info and weights  = up to 32KB total
			kJobGeomDmaSkinIndexesAndWeights_0 = kJobGeomDmaSkinIndexesAndWeights,	// skinning info and weights - first list element
			kJobGeomDmaSkinIndexesAndWeights_1,										// skinning info and weights - second list element
		kJobGeomDmaVertexes1,														// primary vertex stream = up to 3 x 16KB = 48KB
			kJobGeomDmaVertexes1_0 = kJobGeomDmaVertexes1,							// primary vertex stream - first list element
			kJobGeomDmaVertexes1_1,													// primary vertex stream - second list element
			kJobGeomDmaVertexes1_2,													// primary vertex stream - third list element
		kJobGeomDmaVertexes2,														// secondary vertex stream = up to 3 x 16KB = 48KB
			kJobGeomDmaVertexes2_0 = kJobGeomDmaVertexes2,							// secondary vertex stream - first list element
			kJobGeomDmaVertexes2_1,													// secondary vertex stream - second list element
			kJobGeomDmaVertexes2_2,													// secondary vertex stream - third list element
		kJobGeomDmaViewportInfo,													// viewport info - used for culling
		kJobGeomDmaLocalToWorldMatrix,												// see EdgeGeomLocalToWorldMatrix - used for culling
		kJobGeomDmaSpuConfigInfo,													// configuration info for this geometry segment
		kJobGeomDmaFixedPointOffsets1,												// software fixed point format offsets - primary stream
		kJobGeomDmaFixedPointOffsets2,												// software fixed point format offsets - secondary stream
		kJobGeomDmaInputStreamDesc1,												// primary input stream descriptor
		kJobGeomDmaInputStreamDesc2,												// secondary input stream descriptor
	kJobGeomInputDmaEnd
};

// The CellSpursJob256 for this geom job
typedef struct JobGeom256 {
	// Spurs job header / 48 bytes
	CellSpursJobHeader	header;														// regular spurs job header

	// Input DMA list / 160 bytes 
	struct {
		uint32_t	size;         
		uint32_t	eal;          		
	} inputDmaList[kJobGeomInputDmaEnd];											// input DMA list

	// User data / 32 bytes
	struct {
		uint32_t	eaOutputBufferInfo;												// effective address of output buffer info structure
		uint32_t	eaCommandBufferHole;											// effective address of the JTS command buffer hole
		uint32_t	eaBlendShapeInfo;												// effective address of blend shape data, if any
		uint32_t	blendShapeInfoCount;											// number of blendshape infos
		uint32_t	fragmentProgramSize;											// size of the fragment program, in bytes
		uint32_t	eaFragmentProgram;												// effective address of the fragment program
		uint32_t	eaPatchInfo;													// effective address of the patch info structure
		uint32_t	eaConstantData;													// effective address of the fragment program constants
	} userData;

	// Pad to make it a CellSpursJob256 / 16 bytes
	uint32_t	pad[4];
} __attribute__((aligned(128))) JobGeom256;

#ifdef __cplusplus
} // extern "C" 
#endif

#endif//JOB_GEOM_INTERFACE_H

