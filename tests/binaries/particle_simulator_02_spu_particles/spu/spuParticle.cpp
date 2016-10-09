/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

// E This is a simple particle simulator that works on the SPU.
// E Currently, it downloads some data, waits for the download
// E to complete, processes it, and uploads results.  In a later
// E sample we will introduce double buffering so that DMA
// E transfers and computations happen at the same time.

#include <stdlib.h>
#include <sys/spu_event.h>
#include <spu_printf.h>

#include <cell/dma.h>

#include <vectormath_aos.h>
using namespace Vectormath::Aos;

#include "ppuspu/particle_data.h"

#ifndef M_PI
#define M_PI 3.1415926535f
#endif

// E These are the tags that we'll use to refer to our DMA transfers.  The mask
// E is then used when waiting for the DMA to complete.
// E We'll use a different tag for uploading and downloading though since we're
// E not really interleaving these transfers it's not particularly necessary
// E The parameter will also allow us to have independent transfers, so one
// E batch of work could use TAG(1) and another independent batch could use
// E TAG(2)
#define DMA_UPLOAD_TAG(xfer) (2 * xfer + 1)
#define DMA_UPLOAD_MASK(xfer) (1 << DMA_UPLOAD_TAG(xfer))
#define DMA_DOWNLOAD_TAG(xfer) (2 * (xfer + 1))
#define DMA_DOWNLOAD_MASK(xfer) (1 << DMA_DOWNLOAD_TAG(xfer))

// E This will contain PPU-side pointers to the data
particlePointerInfo g_particlePointers __attribute__ ((aligned(128)));

#define WORK_BUFFER_SIZE 3072
#define BALL_RADIUS 15.0f

// E Stores the current positions of the particles, this can be passed into
// E PSGL for rendering
Point3 g_aP3CurrentPositions[WORK_BUFFER_SIZE] __attribute__ ((aligned(128)));

Point3 g_aP3Positions[WORK_BUFFER_SIZE] __attribute__ ((aligned(128)));
Vector3 g_aV3Velocities[WORK_BUFFER_SIZE] __attribute__ ((aligned(128)));
float g_afPhases[WORK_BUFFER_SIZE] __attribute__((aligned(128)));

/**
 * E Function Prototypes
 */
// E Returns a random number between fMin and fMax
float randRange(float fMin, float fMax);
// E Initializes a batch of particles in local memory
void initParticleBatch(int iBatchSize, 
					   Vector3 &v3BallDirection,
					   Point3 *pP3Positions,
					   Vector3 *pV3Velocities,
					   float *pfPhases);
// E Batches up particles, initializes them, and sends results to PPU memory
void initParticles(int iFirstParticle, int iParticleCount);
// E Simulates a batch of particles in local memory
void updateParticleBatch(float fClock, int iBatchSize,
						 Point3 *pP3OutputPositions,
						 Vector3 *pV3OutputVelocities,
						 Point3 *pP3Positions,
						 Vector3 *pV3Velocities);
// E Batches up particles, simulates them, and sends results to PPU memory
void updateParticles(float fClock, int iFirstParticle, int iParticleCount);


// E Random between fMin and fMax
float randRange(float fMin, float fMax) {
	return (float) fMin + (fMax-fMin) * rand()/RAND_MAX;
}

/**
 * E Initialize the particles.
 * This code is pretty much the same as the PPU version of the code.
 * Note that this operates on batches of particles which may be less than the
 * total number of particles in the system.
 */
void initParticleBatch(int iBatchSize, 
					   Vector3 &v3BallDirection,
					   Point3 *pP3Positions,
					   Vector3 *pV3Velocities,
					   float *pfPhases) {
	int iParticle;

	for (iParticle=0;iParticle<iBatchSize;iParticle++) {
		pP3Positions[iParticle]=Point3(0, 10, 15);

		// E We can use two random numbers to find a random point evenly
		// E distributed on a unit sphere
		float fX=randRange(-1.0f, 1.0f);
		float fYZRadius=sqrtf(1.0f-fX*fX);
		float fYZAngle=randRange(-M_PI, M_PI);
		float fY=fYZRadius * cosf(fYZAngle);
		float fZ=fYZRadius * sinf(fYZAngle);

		// E Now figure out how far along this direction to project.  We'll use
		// E the square root so that there are fewer particles in the center,
		// E to get a more even distribution density
		float fRadius = sqrtf(randRange(0, 1)) *  BALL_RADIUS;
		pV3Velocities[iParticle]=Vector3(fX, fY, fZ) * fRadius;

		// E Now add the velocity that defines the direction the ball will fly
		// E in
		pV3Velocities[iParticle]+=v3BallDirection;

		pfPhases[iParticle]=randRange(-2.0f * M_PI, 2.0f * M_PI);
	}
}

/**
 * E This function loops through the total number of particles, breaks them
 * up into workable batches, and then calls initParticleBatch to initialize
 * the individiual chunks.
 */
void initParticles(int iParticleCount) {
	// E Because of the limited memory on the SPU, we need to break down the 
	// E task into smaller phases whose code and data are loaded in, set up
	// E and the results sent back.

	int iCurrentParticle=0;

	uint32_t uiCurrentPositions=g_particlePointers.ppu_P3Positions;
	uint32_t uiCurrentVelocities=g_particlePointers.ppu_V3Velocities;
	uint32_t uiCurrentPhases=g_particlePointers.ppu_fPhases;

	// E This will be the direction of the ball/firework.  We want (at least)
	// E an upward velocity, and possibly some random side-to-side velocity.
	Vector3 v3BallDirection(randRange(-2.5f, 2.5f), 12.5f, 0.0f);

	while (iCurrentParticle<iParticleCount) {
		int iParticlesLeft=iParticleCount-iCurrentParticle;
		int iBatchSize = (iParticlesLeft > WORK_BUFFER_SIZE) ?
			WORK_BUFFER_SIZE : iParticlesLeft;

		initParticleBatch(iBatchSize, v3BallDirection, g_aP3Positions,
						  g_aV3Velocities, g_afPhases);

		cellDmaLargePut(g_aP3Positions, uiCurrentPositions, 
						iBatchSize * sizeof(Point3), DMA_UPLOAD_TAG(1), 0, 0);
		uiCurrentPositions+=iBatchSize*sizeof(Point3);

		cellDmaLargePut(g_aV3Velocities, uiCurrentVelocities,
						iBatchSize*sizeof(Vector3), DMA_UPLOAD_TAG(1), 0, 0);
		uiCurrentVelocities+=iBatchSize*sizeof(Vector3);

		cellDmaLargePut(g_afPhases, uiCurrentPhases,
						iBatchSize*sizeof(float), DMA_UPLOAD_TAG(1), 0, 0);
		uiCurrentPhases+=iBatchSize*sizeof(float);
		cellDmaWaitTagStatusAll(DMA_UPLOAD_MASK(1));

		iCurrentParticle+=iBatchSize;
	}
}


/**
 * E This function actually does the math of computing the new position and
 * velocity for a batch of particles.
 */
void updateParticleBatch(float fClock, int iBatchSize, 
						 Point3 *pP3OutputPositions,
						 Vector3 *pV3OutputVelocities,
						 Point3 *pP3Positions,
						 Vector3 *pV3Velocities) {
	int iParticle;

	// E This is used to bounce the particle off the ground
	static Point3 p3PositionBounceScale(1, -1, 1);
	// E This is used to damp and mirror the velocity due to a bounce
	static Vector3 v3VelocityBounceScale(0.75, -0.5, 0.75);

	Vector3 v3Acceleration=Vector3(0, -9.8, 0) * 0.5f * fClock;


	for (iParticle=0; iParticle<iBatchSize; iParticle++) {
		Point3 p3NewPosition, p3BouncedPosition;
		Vector3 v3NewVelocity, v3BouncedVelocity;

		// E Update the velocity, using half the acceleration
		// E v = v0 + 0.5 * a * t
		v3NewVelocity=pV3Velocities[iParticle] + v3Acceleration;

		// E Update the particle position
		// E p=p0 + v * t + 0.5 * a * t^2
		p3NewPosition=
			pP3Positions[iParticle] + (pV3Velocities[iParticle] * fClock);

		// E Bounce off the ground for particles that hit the ground.
		// E We'll compute this even if the particle doesn't bounce because
		// E computing it and using a selector to assign it is cheaper than the
		// E branch of an 'if' statement
		p3BouncedPosition=mulPerElem(p3NewPosition, p3PositionBounceScale);
		v3BouncedVelocity=mulPerElem(v3NewVelocity, v3VelocityBounceScale);

		pV3OutputVelocities[iParticle]=
			select(v3NewVelocity, v3BouncedVelocity, p3NewPosition[1]<0);
		pP3OutputPositions[iParticle]=
			select(p3NewPosition, p3BouncedPosition, p3NewPosition[1]<0);
	}
}

/**
 * E Batches up all of the particles and calls updateParticleBatch on each
 * group to update their positions and velocities.
 */
void updateParticles(float fClock, int iParticleCount) {
	int iCurrentParticle=0;

	uint32_t uiCurrentPositions=g_particlePointers.ppu_P3Positions;
	uint32_t uiCurrentVelocities=g_particlePointers.ppu_V3Velocities;

	while (iCurrentParticle<iParticleCount) {
		int iParticlesLeft=iParticleCount-iCurrentParticle;
		int iBatchSize = (iParticlesLeft > WORK_BUFFER_SIZE) ?
			WORK_BUFFER_SIZE : iParticlesLeft;

		cellDmaLargeGet(g_aP3Positions, uiCurrentPositions,
						iBatchSize*sizeof(Point3), DMA_DOWNLOAD_TAG(1), 0, 0);
		cellDmaLargeGet(g_aV3Velocities, uiCurrentVelocities,
						iBatchSize*sizeof(Vector3), DMA_DOWNLOAD_TAG(1), 0, 0);
		cellDmaWaitTagStatusAll(DMA_DOWNLOAD_MASK(1));

		updateParticleBatch(fClock, iBatchSize, 
							// E destination buffers
							g_aP3Positions, g_aV3Velocities,
							// E source buffers
							g_aP3Positions, g_aV3Velocities);

		// E Write back the results
		cellDmaLargePut(g_aP3Positions, uiCurrentPositions,
						iBatchSize*sizeof(Point3), DMA_UPLOAD_TAG(1), 0, 0);
		uiCurrentPositions+=iBatchSize*sizeof(Point3);
		cellDmaLargePut(g_aV3Velocities, uiCurrentVelocities,
						iBatchSize*sizeof(Vector3), DMA_UPLOAD_TAG(1), 0, 0);
		uiCurrentVelocities+=iBatchSize*sizeof(Vector3);
		cellDmaWaitTagStatusAll(DMA_UPLOAD_MASK(1));

		iCurrentParticle+=iBatchSize;
	}
}

/**
 * E For this SPU sample, our main function is structured as an infinite loop
 * that waits for requests from the PPU and services them as they come in.
 * We use mailboxes to receive the request, then process the request, and
 * finally send a message back through the event queue to notify the PPU of
 * completion.
 */
int main(uint32_t uiSpuBoundEventPort, uint32_t uiPpuBoundEventPort) {
	unsigned int uiCommand, uiArg1, uiArg2;
	bool bEndLoop=false;

	while (!bEndLoop) {
		sys_spu_thread_receive_event(uiSpuBoundEventPort, &uiCommand, 
									 &uiArg1, &uiArg2);

		switch(uiCommand) {
		case CMD_RELOAD_POINTERS:
			{	
				cellDmaGet(&g_particlePointers, uiArg1,
						   sizeof(g_particlePointers), DMA_DOWNLOAD_TAG(1),
						   0, 0);
				cellDmaWaitTagStatusAll(DMA_DOWNLOAD_MASK(1));
	
				// E Inform the PU of completion
				sys_spu_thread_send_event(uiPpuBoundEventPort, 0, 0);
				break;
			}
		case CMD_INIT_PARTICLES:
			{
				int iParticleCount=uiArg1;

				// E Initialize the particles.
				initParticles(iParticleCount);
		
				// E Inform the PPU of completion
				sys_spu_thread_send_event(uiPpuBoundEventPort, 0, 0);
			}
			break;

		case CMD_SIMULATE_PARTICLES:
			{
				float fSimulationClock;
				int iParticleCount=uiArg1;
				union {
					float f;
					unsigned int ui;
				} longToFloat;

				longToFloat.ui=uiArg2;
				fSimulationClock=longToFloat.f;
	
				// E Simulate the particles
				updateParticles(fSimulationClock, iParticleCount);
		
				// E Inform the PU of completion
				sys_spu_thread_send_event(uiPpuBoundEventPort, 0, 0);
			}
			break;      

		case CMD_EXIT:
			bEndLoop=true;
			break;
		}
	}

	exit(0);
}
