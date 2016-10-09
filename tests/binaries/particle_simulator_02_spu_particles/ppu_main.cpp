/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */
//////////////////////////////////////////////////////////////////////
// E Author: Steven Osman
// E Simple particle simulator on the SPU
//
// E With this sample we have:
//
// E 1. 512k particles simulating
// E    The particles use the vectormath library for an efficient,
// E    easy to use, and portable API for vectors and matrices.
// E 2. Simulation running on the SPU, so we can take advantage of the
// E    fast vector processing of the SPU.  We can now run twice as many
// E    particles as on the PPU.
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/spu_initialize.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_image.h>
#include <sys/ppu_thread.h>
#include <spu_printf.h>

#include <sys/event.h>
#include <sys/process.h>
#include <sys/paths.h>

#include <sys/prx.h>

#include <sysutil/sysutil_common.h>

#include <PSGL/psgl.h>
#include <PSGL/psglu.h>

#include "clock.h"
#include "performance_clock.h"
#include "spu_programs.h"

#include <vectormath_aos.h>
using namespace Vectormath::Aos;

// E Defines the shared data structures between the PPU and SPU
#include "ppuspu/particle_data.h"

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
#define PARTICLE_COUNT 1024 // (512 * 1024)
int g_iActiveParticles=PARTICLE_COUNT;

// E Length to run the simulation before restarting
float g_fSimTime=15.0f;

// E Particle position, velocity, and "phase" (to be used later)
Point3 aP3Positions[PARTICLE_COUNT]  __attribute__ ((aligned(128)));
Vector3 aV3Velocities[PARTICLE_COUNT] __attribute__ ((aligned(128)));
float afPhases[PARTICLE_COUNT] __attribute__ ((aligned(128)));

// E Stores the points to the arrays written above.  Note that the SPU will be
// E grabbing this structure when it is sent a CMD_RELOAD_POINTERS command.
particlePointerInfo g_particlePointers __attribute__ ((aligned(128)));

/**
 * E SPU related variables and constants
 */
// E We will create two event queues for the SPU.  One will be used by the SPU
// E to signal completion of its tasks back to the PPU.  The other one will be
// E used mostly for debugging, when the spu wants to do a printf and display
// E some message.

// E The event queue key is just a unique identifier for messages passed from
// E the SPU back to the PPU.  We also define the size of the buffer, and the
// E port number that the SPU uses to talk to the PPU.  spu_printf uses port
// E 1 for communication.  We'll make two queues, one for the SPU to talk to
// E the PPU and vice-versa
#define SPU_BOUND_EVENT_QUEUE_KEY 0x92311293UL
#define SPU_BOUND_EVENT_QUEUE_SIZE 8
#define SPU_BOUND_EVENT_QUEUE_PORT 0x10

#define PPU_BOUND_EVENT_QUEUE_KEY 0x92311294UL
#define PPU_BOUND_EVENT_QUEUE_SIZE 8
#define PPU_BOUND_EVENT_QUEUE_PORT 0x10

sys_event_queue_t g_spuBoundEventQueue; /* Event queue to talk to SPU */
sys_event_queue_t g_ppuBoundEventQueue; /* Event queue to hear from SPU */
sys_event_port_t g_spuBoundEventPort;   /* Port used from PPU to write to SPU*/

sys_spu_thread_group_t g_spuGroup; /* SPU thread group ID */
sys_spu_thread_t g_spuThread;      /* SPU thread ID */
sys_spu_image_t g_spuELF;          /* SPU executable */

sys_prx_id_t g_spuElfPrx;          /* PPU PRX that contains SPU programs */


// E For the spu_printf, we'll need two things:  A queue attached to port 1 on
// E the SPU, and a thread on the PPU that listens on this queue and services
// E the printf requests as they come in -- the actual printing must be done by
// E the PPU.
#define SPU_PRINTF_EVENT_QUEUE_KEY 0x12345678UL
#define SPU_PRINTF_EVENT_QUEUE_SIZE 8
#define SPU_PRINTF_EVENT_QUEUE_PORT 0x1

sys_event_queue_t g_spuPrintfEventQueue;
sys_ppu_thread_t	g_spuPrintfThread;

// E The priority and stack size of the thread that handles spu_printf
// E requests.
#define SPU_PRINTF_THREAD_PRIO 1001
#define SPU_PRINTF_THREAD_STACK_SIZE (64 * 1024)

/**
 * E Function Prototypes
 */
// E Sysutil callback function, that responds to exit requests from the system
void sysutilCallback(uint64_t uiStatus, uint64_t uiParam, void *pvUserData);
// E Load a start PRX -- a dynamically loadable PPU module
int loadAndStartPrx(sys_prx_id_t *pPrxId, const char *pcFile);
// E Shut down and unload a PRX
static void spuPrintfThreadMain(uint64_t arg);
// E Blocks the PPU until the SPU is done processing
sys_event_t waitForSPU(void);
// E Seeds the structure that contains data pointers for the SPU
void sendPointersToSPU(void);
// E Loads and starts up the SPU programs
void startSPU(void);
// E Asks the SPU to initialize the particles and waits for completion
void initParticles(int iParticles);
// E Asks the SPU to simulate the particles and waits for completion
void updateParticles(float fClock, int iParticles);
// E Initializes the graphics system and buffers
void initializeGraphics(void);

/**
 * E This is the main function of the spu_printf service thread.
 * It listens for messages and calls the printf handler when it gets them,
 * then notifies the SPU of completion by using a mailbox.
 */
static void spuPrintfThreadMain(uint64_t arg) {
	sys_event_t event;
	int iReturn;

	// E For unused parameter warnings
	(void) arg;

	while (1) {
		iReturn=sys_event_queue_receive(g_spuPrintfEventQueue, &event,
										SYS_NO_TIMEOUT);
	
		if (iReturn!=CELL_OK) {
			fprintf(stderr, "Event queue receive wasn't successful: %i\n",
					iReturn);
			exit(-1);
		}

		iReturn=spu_thread_printf(event.data1, event.data3);
		sys_spu_thread_write_spu_mb(event.data1, iReturn);
	}
}

/**
 * E Wait for the SPU to send an event back to our event queue.
 */
sys_event_t waitForSPU(void) {
	sys_event_t event;
	int iReturn;

	iReturn=sys_event_queue_receive(g_ppuBoundEventQueue, &event,
									SYS_NO_TIMEOUT);
  
	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Event queue receive wasn't successful: %i\n",
				iReturn);
		exit(-1);
	}

	return event;  
}

/**
 * E Set up the pointers for the source data to be grabbed by the SPU.
 * and send the pointers to the SPU.
 */
void sendPointersToSPU(void) {
	g_particlePointers.ppu_P3Positions=aP3Positions;
	g_particlePointers.ppu_V3Velocities=aV3Velocities;
	g_particlePointers.ppu_fPhases=afPhases;

	

	sys_event_port_send(g_spuBoundEventPort, CMD_RELOAD_POINTERS,
						(uint32_t) &g_particlePointers, 0);
  
	// E Wait for the SPU to be done
	

	waitForSPU();

	
}

/**
 * E Loads and starts a PRX.
 * Returns the ID of the PRX in *pPrxId
 * pcFile contains a full path to the .PRX file to load.
 * The return value is the value returned by the sys_prx_start_module call
 * to the PRX.
 */
int loadAndStartPrx(sys_prx_id_t *pPrxId, const char *pcFile) {
	int iReturn;
	int iStartReturn;
	sys_prx_id_t prxId;

	prxId = sys_prx_load_module(pcFile, 0, NULL);

	if (prxId < CELL_OK) {
		fprintf(stderr, "Error loading PRX %s: %i\n", pcFile, (int) prxId);
		exit(-1);
	}

	iReturn = sys_prx_start_module(prxId, 0, NULL, &iStartReturn, 0, NULL);

	if (iReturn != CELL_OK) {
		fprintf(stderr, "Error starting PRX %s: %i\n", pcFile, iReturn);
		exit(-1);
	}

	*pPrxId = prxId;
	return iStartReturn;
}

/**
 * E Starting an SPU thread is a multi-step process.
 * First, create a group for the thread(s)
 * Second, load the code for the thread(s)
 * Third, create the thread.
 * Fourth, (optionally) free up the code loaded if you no longer need it
 * Fifth, start the SPU thread group
 *
 * We'll also create an event queue and attach the queue to the SPU for
 * communication.
 */
void startSPU(void) {
	sys_spu_thread_group_attribute_t group_attr;/* SPU thread group attribute*/

	sys_spu_thread_attribute_t thread_attr;     /* SPU thread attribute */
	sys_spu_thread_argument_t thread_args;      /* SPU thread arguments */

	sys_event_queue_attribute_t queue_attr;     /* Event queue attributes */

	int iReturn;

	// E Step one: Create and configure the thread group.
	group_attr.name="Particle Simulation Group";
	group_attr.nsize=strlen(group_attr.name) + 1;
	group_attr.type=SYS_SPU_THREAD_GROUP_TYPE_NORMAL;

	// E 1 Thread, priority is 100
	iReturn=sys_spu_thread_group_create(&g_spuGroup, 1, 100, &group_attr);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error creating SPU thread group: %i\n", iReturn);
		exit(-1);
	}
	
	// E Step two: Load the thread code
	// E The code is embedded in a .sprx file for TRC compliance,
	// E load that first:
	loadAndStartPrx(&g_spuElfPrx, SYS_APP_HOME "/spu_programs.sprx");

	iReturn=sys_spu_image_import(&g_spuELF, getParticleSpuElf(),
								 SYS_SPU_IMAGE_DIRECT);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error loading SPU elf: %i\n", iReturn);
		exit(-1);
	}

	// E Step three: Create and initialize the thread
	thread_attr.name="Particle simulation thread #1";
	thread_attr.nsize=strlen(thread_attr.name)+1;
	thread_attr.option=SYS_SPU_THREAD_OPTION_NONE;

	thread_args.arg1=
		SYS_SPU_THREAD_ARGUMENT_LET_32(SPU_BOUND_EVENT_QUEUE_PORT);
	thread_args.arg2=
		SYS_SPU_THREAD_ARGUMENT_LET_32(PPU_BOUND_EVENT_QUEUE_PORT);

	iReturn=sys_spu_thread_initialize(&g_spuThread, g_spuGroup, 0,
									  &g_spuELF, &thread_attr, 
									  &thread_args);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error initializing SPU thread: %i\n", iReturn);
		exit(-1);
	}

	// E Now create the event queue to send messages to the SPU
	sys_event_queue_attribute_initialize(queue_attr);
  
	// E This is an SPU-bound queue
	queue_attr.attr_protocol = SYS_SYNC_PRIORITY;
	queue_attr.type = SYS_SPU_QUEUE;

	iReturn=sys_event_queue_create(&g_spuBoundEventQueue, &queue_attr,
								   SPU_BOUND_EVENT_QUEUE_KEY,
								   SPU_BOUND_EVENT_QUEUE_SIZE);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error creating spu bound event queue: %i\n", iReturn);
		exit(-1);
	}

	iReturn=sys_event_port_create(&g_spuBoundEventPort,
								  SYS_EVENT_PORT_LOCAL,
								  SPU_BOUND_EVENT_QUEUE_PORT);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error creating port for spu bound event queue: %i\n",
				iReturn);
		exit(-1);
	}

	iReturn=sys_event_port_connect_local(g_spuBoundEventPort,
										 g_spuBoundEventQueue);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error connecting port to spu bound event queue: %i\n",
				iReturn);
		exit(-1);
	}

	// E And connect the event queue to the SPU.  Use the same port number we
	// E had sent to the SPU thread
	iReturn=sys_spu_thread_bind_queue(g_spuThread, g_spuBoundEventQueue,
									  SPU_BOUND_EVENT_QUEUE_PORT);

	if (iReturn!=CELL_OK) {
		fprintf(stderr,
				"Error binding spu thread to spu bound event queue: %i\n",
				iReturn);
		exit(-1);
	}
  
	// E And create one for the SPU to communicate to the PPU
	sys_event_queue_attribute_initialize(queue_attr);
	iReturn=sys_event_queue_create(&g_ppuBoundEventQueue, &queue_attr,
								   PPU_BOUND_EVENT_QUEUE_KEY,
								   PPU_BOUND_EVENT_QUEUE_SIZE);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error creating ppu bound event queue: %i\n", iReturn);
		exit(-1);
	}

	// E And connect the PPU-bound event queue to the SPU.
	iReturn=sys_spu_thread_connect_event(g_spuThread, g_ppuBoundEventQueue,
										 SYS_SPU_THREAD_EVENT_USER,
										 PPU_BOUND_EVENT_QUEUE_PORT);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error connecting spu thread to ppu event queue: %i\n",
				iReturn);
		exit(-1);
	}
  
	// E Also create an event queue that the SPU can use to send printf
	// E commands to the PPU for debugging.  This is optional but very useful.
	sys_event_queue_attribute_initialize(queue_attr);
	iReturn=sys_event_queue_create(&g_spuPrintfEventQueue, &queue_attr,
								   SPU_PRINTF_EVENT_QUEUE_KEY,
								   SPU_PRINTF_EVENT_QUEUE_SIZE);
  
	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error creating printf event queue: %i\n", iReturn);
		exit(-1);
	}
  
	// E We need a thread on the PPU side to handle SPU printf requests.
	// E It will wait indefinitely for messages in the queue we just created.
	iReturn = sys_ppu_thread_create (&g_spuPrintfThread,
									 spuPrintfThreadMain, 
									 (uint64_t) 0, SPU_PRINTF_THREAD_PRIO,
									 SPU_PRINTF_THREAD_STACK_SIZE, 
									 SYS_PPU_THREAD_CREATE_JOINABLE,
									 "spu_printf_handler");
  
	if (iReturn != CELL_OK) {
		printf ("sys_ppu_thread_create failed (%d)\n", iReturn);
		exit (-1);
	}
  
	// E Finally, we'll attach the printf event queue to the SPU thread.
	iReturn=sys_spu_thread_connect_event(g_spuThread, g_spuPrintfEventQueue,
										 SYS_SPU_THREAD_EVENT_USER,
										 SPU_PRINTF_EVENT_QUEUE_PORT);

	if (iReturn!=CELL_OK) {
		fprintf(stderr,
				"Error connecting spu thread to printf event queue: %i\n",
				iReturn);
		exit(-1);
	}

	// E Step five: Start the SPU thread group
	iReturn=sys_spu_thread_group_start(g_spuGroup);

	if (iReturn!=CELL_OK) {
		fprintf(stderr, "Error starting the SPU thread group: %i\n", iReturn);
		exit(-1);
	}

	// E Ask the SPU to load up the structure that contains the pointers to
	// E our particle data.
	sendPointersToSPU();
}

/**
 * E Initializes the particles.
 */
void initParticles(int iParticles) {
	sys_event_port_send(g_spuBoundEventPort, CMD_INIT_PARTICLES,
						iParticles, 0);

	// E Wait for the SPU to be done
	waitForSPU();
}

/**
 * E Ask the SPU to update the particles.
 */
void updateParticles(float fClock, int iParticles) {
	union {
		unsigned int ui;
		float f;
	} argument;

	argument.f=fClock;


	sys_event_port_send(g_spuBoundEventPort, CMD_SIMULATE_PARTICLES,
						iParticles, argument.ui);

	// E Wait for the SPU to be done

	waitForSPU();
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

	printf("Initializing SPU Thread\n");
	startSPU();

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
			printf("Initializing particles\n");

			CLOCK_START(CLOCK_INIT);
			initParticles(g_iActiveParticles);
			CLOCK_END(CLOCK_INIT);

			fStartTime=clockSeconds();
			fTime=fPreviousTime=fDeltaTime=0;

			CLOCK_REPORT_AND_RESET(CLOCK_INIT, 1);
		}

		CLOCK_START(CLOCK_SIM);
		//updateParticles(fDeltaTime, g_iActiveParticles);
		if (!iTotalFrames) {
			updateParticles(2, g_iActiveParticles);
		}
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

		if (iTotalFrames % 100 == 0) {
			CLOCK_REPORT_AND_RESET(CLOCK_LOOP, (float) 1.0f/100);
			CLOCK_REPORT_AND_RESET(CLOCK_SIM, (float) 1.0f/100);
			CLOCK_REPORT_AND_RESET(CLOCK_DRAW, (float) 1.0f/100);
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
