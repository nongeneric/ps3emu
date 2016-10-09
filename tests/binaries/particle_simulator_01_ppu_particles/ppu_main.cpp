/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */
//////////////////////////////////////////////////////////////////////
// E Author: Steven Osman
// E Simple particle simulator on the PPU
//
// E With this sample we have:
//
// E 1. 256k particles simulating (we'll build on this through more
// E    and more samples)
// E    The particles use the vectormath library for an efficient,
// E    easy to use, and portable API for vectors and matrices.
//
// E    NOTE
// E    ----
// E    THIS PPU VERSION RUNS SLOWLY.  It is intended to illustrate some
// E    beginner concepts that will be expanded upon and optimized in
// E    future samples.
//
// E    NOTE 2
// E    ------
// E    With the SPU version of the samples we will double the number of
// E    particles.
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#include <sys/process.h>
#include <sys/spu_initialize.h>
#include <sys/paths.h>

#include <sysutil/sysutil_common.h>

#include <PSGL/psgl.h>
#include <PSGL/psglu.h>

#include "clock.h"
#include "performance_clock.h"

#include <vectormath_aos.h>
using namespace Vectormath;
using namespace Vectormath::Aos;

#ifndef M_PI
#define M_PI 3.1415926535f
#endif

// E These clock methods are defined in performance_clock.h
// E They are just used as a simple benchmarking tool
CLOCK(CLOCK_INIT, "Initializing particles");
CLOCK(CLOCK_LOOP, "Loop");
CLOCK(CLOCK_SIM, "Simulation");
CLOCK(CLOCK_DRAW, "Draw");

// E Screen resolution
static int g_iGLWidth;
static int g_iGLHeight;

// E Maximum number of particles and active number of particles
#define PARTICLE_COUNT 1  * 1024
int g_iActiveParticles=PARTICLE_COUNT;

// E Length to run the simulation before restarting
float g_fSimTime=15.0f;

// E The radius of the ball that the particles explode in
#define BALL_RADIUS 15

// E Particle position, velocity, and "phase" (to be used later)
Point3 aP3Positions[PARTICLE_COUNT]  __attribute__ ((aligned(128)));
Vector3 aV3Velocities[PARTICLE_COUNT] __attribute__ ((aligned(128)));
float afPhases[PARTICLE_COUNT] __attribute__ ((aligned(128)));

/**
 * E Function Prototypes
 */
// E Sysutil callback function, that responds to exit requests from the system
void sysutilCallback(uint64_t uiStatus, uint64_t uiParam, void *pvUserData);
// E Returns a random number between fMin and fMax
float randRange(float fMin, float fMax);
// E Initializes the particles
void initParticles(int iParticles);
// E Simulates the particles
void updateParticles(float fClock, int iParticles);
// E Initializes the graphics system and buffers
void initializeGraphics(void);

// E Random between fMin and fMax
float randRange(float fMin, float fMax) {
	return (float) fMin + (fMax-fMin) * rand()/RAND_MAX;

}

/**
 * E Initializes the particles.
 */
void initParticles(int iParticles) {
	int iParticle;
	// E This will be the direction of the ball/firework.  We want (at least)
	// E an upward velocity, and possibly some random side-to-side velocity.
	Vector3 v3BallDirection(randRange(-2.5f, 2.5f), 12.5f, 0.0f);

	for (iParticle=0;iParticle<iParticles;iParticle++) {
		aP3Positions[iParticle]=Point3(0, 10, 15);

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
		aV3Velocities[iParticle]=Vector3(fX, fY, fZ) * fRadius;

		// E Now add the velocity that defines the direction the ball will fly
		// E in
		aV3Velocities[iParticle]+=v3BallDirection;

		afPhases[iParticle]=randRange(-2.0f * M_PI, 2.0f * M_PI);
	}
}

/**
 * E This method loops throught the particles and updates their positions
 * and velocities based on previous position, velocity, and acceleration
 * information.
 */
void updateParticles(float fClock, int iParticles) {
	int iParticle;

	// E This is used to bounce the particle off the ground
	static Point3 p3PositionBounceScale(1, -1, 1);
	// E This is used to damp and mirror the velocity due to a bounce
	static Vector3 v3VelocityBounceScale(0.75, -0.5, 0.75);

	// E Moving data between the register files (e.g. altivec <--> fp) is very
	// E slow on the PPU because of the load-hit-store issue.  Because of this
	// E it is more efficient to splat scalar values into vectors outside of
	// E a loop if possible.
	//
	// E The load-hit-store issue is as follows: If a write to a memory address
	// E is followed by a read from the same memory address (even within
	// E several cycles of each other), if you try to use the data you read in,
	// E the entire pipeline will  be flushed before the execute can proceed.
	// E This could easily happen when sharing data between register files
	// E since the data is passed through memory.
	Vector3 v3SplattedHalfClock(0.5f * fClock);
	Vector3 v3SplattedClock(fClock);

	Vector3 v3Acceleration=Vector3(0, -9.8, 0) * 0.5f * fClock;

	for (iParticle=0; iParticle<iParticles; iParticle++) {
#ifdef SIMPLE_BUT_SLOW_IMPLEMENTATION
		// E This code is here to show you what the more optimized version
		// E below is trying to achieve.  It isn't run unless you define the
		// E macro.
		aV3Velocities[iParticle] += v3Acceleration;

		// E Update the particle position
		// E p=p0 + v0 * t + 0.5 * a * t^2
		aP3Positions[iParticle]=
			aP3Positions[iParticle] +
			mulPerElem(aV3Velocities[iParticle], v3SplattedClock);


		if (aP3Positions[iParticle][1]<0) {
			aP3Positions[iParticle]=mulPerElem(aP3Positions[iParticle],
											   p3PositionBounceScale);
			aV3Velocities[iParticle]=mulPerElem(aV3Velocities[iParticle],
												v3VelocityBounceScale);
		}
#else
		Point3 p3NewPosition, p3BouncedPosition;
		Vector3 v3NewVelocity, v3BouncedVelocity;

		// E This used to test against the "ground" (i.e. Y=0)
		const static floatInVec fivGround(0);

		// E Update the velocity, using half the acceleration
		// E v = v0 + 0.5 * a * t
		v3NewVelocity=aV3Velocities[iParticle] + v3Acceleration;

		// E Update the particle position
		// E p=p0 + v * t + 0.5 * a * t^2
		p3NewPosition=
			aP3Positions[iParticle] +
			mulPerElem(aV3Velocities[iParticle], v3SplattedClock);

		// E Bounce off the ground for particles that hit the ground.
		// E We'll compute this even if the particle doesn't bounce because
		// E computing it and using a selector to assign it is cheaper than the
		// E branch of an 'if' statement
		p3BouncedPosition=mulPerElem(p3NewPosition, p3PositionBounceScale);
		v3BouncedVelocity=mulPerElem(v3NewVelocity, v3VelocityBounceScale);

		// E Using floatInVec/boolInVec avoids having to switch from scalar to
		// E vector floating point registers.
		boolInVec bivBelowGround(p3NewPosition.getY()<fivGround);

		// E Using select avoids having to branch on the below ground condition
		aV3Velocities[iParticle]=
			select(v3NewVelocity, v3BouncedVelocity, bivBelowGround);
		aP3Positions[iParticle]=
			select(p3NewPosition, p3BouncedPosition, bivBelowGround);
#endif
	}
}

/**
 * E Initializes the graphics system.
 * Does so by initializing psgl, creating a device, and a device context.
 */
void initializeGraphics(void) {
	// E First, initialize PSGL
	// E Note that since we initialized the SPUs ourselves earlier we should
	// E make sure that PSGL doesn't try to do so as well.
	PSGLinitOptions initOpts={
		enable: PSGL_INIT_MAX_SPUS | PSGL_INIT_INITIALIZE_SPUS | 
		        PSGL_INIT_HOST_MEMORY_SIZE,
		maxSPUs: 1,
		initializeSPUs: false,

		// E We're not specifying values for these options, the code is only
		// E here to alleviate compiler warnings.
		persistentMemorySize: 0,
		transientMemorySize: 0,
		errorConsole: 0,
		fifoSize: 0,	

		// E Put aside 32 megabytes for VBOs.
		hostMemorySize: 32 * 1024 * 1024,
	};

	psglInit(&initOpts);

	// E Create the PSGL device, with double buffering, 4xAA,
	// E 32 bit R/G/B/A and 24 bit depth buffer with 8 bit stencil buffer.
	PSGLdevice *pGLDevice=
		psglCreateDeviceAuto(GL_ARGB_SCE, GL_DEPTH_COMPONENT24,
							 GL_MULTISAMPLING_4X_SQUARE_ROTATED_SCE);
	if (pGLDevice==NULL) {
		fprintf(stderr, "Error creating PSGL device\n");
		exit(-1);
	}

	// E Query the resolution that the system has been configured to output
	GLuint uiWidth, uiHeight;
	psglGetDeviceDimensions(pGLDevice, &uiWidth, &uiHeight);

	g_iGLWidth=(int) uiWidth;
	g_iGLHeight=(int) uiHeight;

	printf("Video mode configured as %ix%i\n", g_iGLWidth, g_iGLHeight);

	// E Now create a PSGL context
	PSGLcontext *pContext=psglCreateContext();

	if (pContext==NULL) {
		fprintf(stderr, "Error creating PSGL context\n");
		exit(-1);
	}

	// E Make this context current for the device we initialized
	psglMakeCurrent(pContext, pGLDevice);

	// E Since we're using fixed function stuff (i.e. not using our own shader
	// E yet), we need to load shaders.bin that contains the fixed function 
	// E shaders.
	psglLoadShaderLibrary(SYS_APP_HOME "/shaders.bin");

	// E Reset the context
	psglResetCurrentContext();
  
	glViewport(0, 0, g_iGLWidth, g_iGLHeight);
	glScissor(0, 0, g_iGLWidth, g_iGLHeight);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glEnable(GL_DEPTH_TEST);

	// E Disable VSYNC just for benchmarking reasons -- we don't want to have
	// E 60fps be the lower bound for this sample -- just to measure how fast
	// E it can really get.
	glDisable(GL_VSYNC_SCE);

	// E PSGL doesn't clear the screen on startup, so let's do that here.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	psglSwap();
}

/**
 * E This is our sysutil callback function, that responds to exit requests from
 * the system.  In this implementation, the userdata pointer points to a boolean
 * which will be flagged as true to indicate that the application needs to exit.
 * We need to register this function and poll for callbacks periodically.
 */
void sysutilCallback(uint64_t uiStatus, uint64_t uiParam, void *pvUserData) {
	// E For unused parameter warnings
	(void) uiParam;

	switch(uiStatus) {
		case CELL_SYSUTIL_REQUEST_EXITGAME:
			*((bool *) pvUserData) = true;
			break;
		default:
			break;
	}
}

/**
 * E Our main function
 * Initializes spus, graphics, and goes into the main loop of simulating and
 * drawing.
 */
int main(int argc, char *argv[]) {
	int iTotalFrames=0;
	GLuint iVBO;
	int iReturn;
	bool bExit=false;

	// E For unused parameter warnings
	(void) argc;
	(void) argv;

	// E Register with sysutil, to be notified if this application needs to quit
	iReturn=cellSysutilRegisterCallback(0, sysutilCallback, &bExit);
	if (iReturn < CELL_OK) {
		fprintf(stderr, "Error registering for sysutil callbacks: %i\n", iReturn);
		exit(-1);
	}

	// E Initialize the SPUs
	printf("SPU Initialize\n");
	// E Initialize 6 SPUs but reserve 1 SPU as a raw SPU for PSGL
	sys_spu_initialize(6, 1);

	// E Initialize the graphics system
	printf("Initializing Graphics\n");
	initializeGraphics();

	// E Initialize our clock
	clockInit();

	// E Initialize the VBO.  This is where the results will be stored.
	// E Using GL_SYSTEM_DRAW_SCE ensures that the VBO resides in main memory
	// E instead of RSX local memory.  This is good (in this case) because
	// E the RSX can pull from main memory much faster than the Cell can push
	// E to RSX.  If this were a more advanced sample, the VBO would actually
	// E be mapped and the data written directly into the VBO, instead of using
	// E a glBufferData command later on to copy the data.  This does require
	// E either double buffering the VBO data or some from of RSX
	// E synchronization to ensure that the data the RSX is using doesn't get
	// E overwritten.
	glGenBuffers(1, &iVBO);
	glBindBuffer(GL_ARRAY_BUFFER, iVBO);
	glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(Point3), 0,
				 GL_SYSTEM_DRAW_SCE);

	// E Set up the static graphics stuff
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspectivef(90.0f, (float) g_iGLWidth/(float) g_iGLHeight,
					1.0f, 1000.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAtf(0,15,-50, 0,20,15, 0,1,0);

	while (!bExit) {
		CLOCK_START(CLOCK_LOOP);
		static float fStartTime=clockSeconds();
		static float fPreviousTime=0;
		float fTime=clockSeconds()-fStartTime;
		float fDeltaTime=fTime-fPreviousTime;
		fPreviousTime=fTime;

		// E If the simulation ran for more than g_fSimTime seconds then start
		// E with a new set of particles.
		if (fTime>g_fSimTime || iTotalFrames==0) {
			printf("Initializing particles (this can take about 15-20 seconds "
				   "on the PPU)\n");

			CLOCK_START(CLOCK_INIT);
			initParticles(g_iActiveParticles);
			CLOCK_END(CLOCK_INIT);

			fStartTime=clockSeconds();
			fTime=fPreviousTime=fDeltaTime=0;

			CLOCK_REPORT_AND_RESET(CLOCK_INIT, 1);
		}

		CLOCK_START(CLOCK_SIM);
		if (!iTotalFrames)
			updateParticles(2, g_iActiveParticles);
		//updateParticles(fDeltaTime, g_iActiveParticles);
		CLOCK_END(CLOCK_SIM);

		CLOCK_START(CLOCK_DRAW);

		// E Render the results.
		// E We'll clear the depth buffer but not the stencil because it is
		// E faster that way.  If we were using stencil, we'd probably need
		// E to clear it too.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindBuffer(GL_ARRAY_BUFFER, iVBO);
		glBufferData(GL_ARRAY_BUFFER, g_iActiveParticles * sizeof(Point3), 
					 aP3Positions, GL_SYSTEM_DRAW_SCE);	       

		glColor4f(1.0f, 0.5f + fTime/10.0f, 0.5f + (0.5f-fTime)/10.0f, 1.0f);

		glEnableClientState(GL_VERTEX_ARRAY);
		// E Note that the offset of the vertices is relative to the beginning
		// E of the VBO
		glVertexPointer(3, GL_FLOAT, sizeof(Point3), 0);
		glDrawArrays(GL_POINTS, 0, g_iActiveParticles);
	
		psglSwap();
		CLOCK_END(CLOCK_DRAW);
		iTotalFrames++;

		CLOCK_END(CLOCK_LOOP);

		if (iTotalFrames % 25 == 0) {
			CLOCK_REPORT_AND_RESET(CLOCK_LOOP, (float) 1.0f/25);
			CLOCK_REPORT_AND_RESET(CLOCK_SIM, (float) 1.0f/25);
			CLOCK_REPORT_AND_RESET(CLOCK_DRAW, (float) 1.0f/25);
			printf("\n");
		}

		// E Poll sysutil to see if it has any callbacks to process
		iReturn=cellSysutilCheckCallback();

		if (iReturn < CELL_OK) {
			fprintf(stderr, "Error checking for sysutil callbacks: %i\n", iReturn);
			exit(-1);
		}

		bExit = iTotalFrames == 1;
	}

	printf("Sample is exiting.\n");

	// E Ensure all rendering tasks are complete
	glFinish();
  
	return 0;
}
