/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *                Copyright (C) 2010 Sony Computer Entertainment Inc.
 *                                               All Rights Reserved.
 */

// createdevice video setup example
#include <math.h>
#include <stdio.h>
#include <PSGL/psgl.h>

#include <sys/spu_initialize.h>
#include <sys/paths.h>

#include <sysutil/sysutil_sysparam.h>  // used for cellVideoOutGetResolutionAvailability() and videoOutIsReady()

float pos[] = { -1,-1,0,  -1,1,0,  1,-1,0,  1,1,0, };

void drawAxisAlignedLine(float cntrX, float cntrY, float halfWidth, float halfHeight)
{
  glPushMatrix();
  glTranslatef(cntrX,cntrY,0);
  glScalef(halfWidth,halfHeight,1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glPopMatrix();
}

void drawGrid(const int numLines)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, pos);

  const float hw=0.005f, hh=1.0f;
  float dwh = 2.0f/numLines;
  
  glColor4f(1,1,1,1);
  for (float x=-1+(dwh*0.5f); x<1; x+=dwh)
    drawAxisAlignedLine(x,0,hw,hh);
  for (float y=-1+(dwh*0.5f); y<1; y+=dwh)
    drawAxisAlignedLine(0,y,hh,hw);
    
  glColor4f(1,0,0,1);
  drawAxisAlignedLine(-1,0,hw,hh);
  drawAxisAlignedLine(1,0,hw,hh);
  drawAxisAlignedLine(0,-1,hh,hw);
  drawAxisAlignedLine(0,1,hh,hw);
  drawAxisAlignedLine(0,0,hw*2,hw*2);
}

void drawRotatingGrid(const int numSteps)
{
  float zRot=0;
  for (int j=0; j<numSteps; j++)
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    glRotatef(zRot,0,0,1);

    drawGrid(20);
    zRot+=0.01f; if (zRot>360) zRot-=360;

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    psglSwap();
  }
}

void initGraphics(PSGLdevice *device)
{
  // get render target buffer dimensions and set viewport
  GLuint renderWidth, renderHeight;
  psglGetRenderBufferDimensions(device,&renderWidth,&renderHeight);

  glViewport(0, 0, renderWidth, renderHeight);

  // get display aspect ratio (width / height) and set projection
  // (it is important to use this value and NOT renderWidth/renderHeight since
  // pixel ratios do not necessarily match the 16/9 or 4/3 display aspect ratios)
  GLfloat aspectRatio = psglGetDeviceAspectRatio(device);  

  float l=aspectRatio, r=-l, b=-1, t=1;
	glMatrixMode(GL_PROJECTION);
	  glLoadIdentity();
    glOrthof(l,r,b,t,0,1);

  glClearColor(0.f, 0.f, 0.f, 1.f);
  glDisable(GL_CULL_FACE);

  // PSGL doesn't clear the screen on startup, so let's do that here.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  psglSwap();
}

// Given an priority ordered array of desired resolutions (from most desired to least), chooses
// the first mode that is available on the current display device.
//
// The list of modes are chosen from the following standard modes:
//   
//   CELL_VIDEO_OUT_RESOLUTION_480   (720x480)
//   CELL_VIDEO_OUT_RESOLUTION_576   (720x576)
//   CELL_VIDEO_OUT_RESOLUTION_720   (1280x720)
//   CELL_VIDEO_OUT_RESOLUTION_1080  (1920x1080)
//
// or these modes that allow a lower resolution buffer to automatically be 
// upscaled (in hardware) to the 1920x1080 standard:
//
//   CELL_VIDEO_OUT_RESOLUTION_1600x1080
//   CELL_VIDEO_OUT_RESOLUTION_1440x1080
//   CELL_VIDEO_OUT_RESOLUTION_1280x1080
//   CELL_VIDEO_OUT_RESOLUTION_960x1080
//
// if none of the desired resolutions are available, 0 is returned
//
// example, choose 1920x1080 if possible, but otherwise try for 1280x720:
//
//   unsigned int resolutions[] = { CELL_VIDEO_OUT_RESOLUTION_1080, CELL_VIDEO_OUT_RESOLUTION_720 };
//   int numResolutions = 2;
static int chooseBestResolution(const unsigned int *resolutions, unsigned int numResolutions)
{
  unsigned int bestResolution=0;
  for (unsigned int i=0; bestResolution==0 && i<numResolutions; i++)
    if( cellVideoOutGetResolutionAvailability(CELL_VIDEO_OUT_PRIMARY,resolutions[i],CELL_VIDEO_OUT_ASPECT_AUTO,0) )
      bestResolution = resolutions[i];
  return bestResolution;
}

// Given one of the valid video resolution IDs, assigns the associated dimensions in w and h.
// If the video resolution ID is invalid, 0 is returned, 1 if valid
static int getResolutionWidthHeight(const unsigned int resolutionId, unsigned int &w, unsigned int &h)
{
  switch(resolutionId)
  {
    case CELL_VIDEO_OUT_RESOLUTION_480       : w=720;  h=480;  return(1);
    case CELL_VIDEO_OUT_RESOLUTION_576       : w=720;  h=576;  return(1);
    case CELL_VIDEO_OUT_RESOLUTION_720       : w=1280; h=720;  return(1);
    case CELL_VIDEO_OUT_RESOLUTION_1080      : w=1920; h=1080; return(1);
    case CELL_VIDEO_OUT_RESOLUTION_1600x1080 : w=1600; h=1080; return(1);
    case CELL_VIDEO_OUT_RESOLUTION_1440x1080 : w=1440; h=1080; return(1);
    case CELL_VIDEO_OUT_RESOLUTION_1280x1080 : w=1280; h=1080; return(1);
    case CELL_VIDEO_OUT_RESOLUTION_960x1080  : w=960;  h=1080; return(1);
  };
  printf("getResolutionWidthHeight: resolutionId %d not a valid video mode\n",resolutionId);
  return(0);
}

// Checks if the video output device is ready for initialization by psglInit.
// Call this before calling psglInit, until it returns true. This is mainly used to make
// sure HDMI devices are turned on and connected before calling psglInit. psglInit
// will busy wait until the device is ready, so repeatedly calling this allows 
// processing while waiting. For non-HDMI devices, this routine always returns true.
bool videoOutIsReady()
{
  CellVideoOutState videoState;
  cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);
  return( videoState.state==CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED );
}

int main()
{
	sys_spu_initialize(6, 1);

  // Check if the video output device is ready BEFORE calling psglInit.
  // This will make sure HDMI devices are turned on and connected, and allow
  // background processing while waiting (psglInit will busy wait until ready).
  // Non-HDMI video out is always "ready", so they pass through this loop trivially.
  while(!videoOutIsReady())
  {
    // do your background processing here until the video is ready
    printf("video not ready!\n");
  };
  printf("VIDEO READY!\n");

  PSGLinitOptions initOpts = 
  {
    enable: PSGL_INIT_MAX_SPUS | PSGL_INIT_INITIALIZE_SPUS,
    maxSPUs: 1,
    initializeSPUs: false,
    persistentMemorySize: 0,
    transientMemorySize: 0,
    errorConsole: 0,
    fifoSize: 0,  
    hostMemorySize: 128* 1024*1024,  // 128 mbs for host memory 
  };
  psglInit(&initOpts);

  // (1) create array of all desired resolutions in priority order (most desired to least).
  //     In this example, we choose 1080p if possible, then 960x1080-to-1920x1080 horizontal scaling,
  //     and in the worst case, 720p.
  const unsigned int resolutions[] = { CELL_VIDEO_OUT_RESOLUTION_1080, CELL_VIDEO_OUT_RESOLUTION_960x1080, CELL_VIDEO_OUT_RESOLUTION_720 };
  const int numResolutions = sizeof(resolutions)/sizeof(resolutions[0]);

  // (2) loop through the modes and grab the first available
  int bestResolution = chooseBestResolution(resolutions,numResolutions);

  // (3) get the chosen video mode's pixel dimensions
  unsigned int deviceWidth=0, deviceHeight=0;
  getResolutionWidthHeight(bestResolution,deviceWidth,deviceHeight);

  // (3) if desired resolution is available, create the PSGL device and context
  if (bestResolution)
  {
    printf("%d x %d is available...\n",deviceWidth,deviceHeight);

    // (4) create the PSGL device based on the selected resolution mode
    PSGLdeviceParameters params;
    params.enable = PSGL_DEVICE_PARAMETERS_COLOR_FORMAT | PSGL_DEVICE_PARAMETERS_DEPTH_FORMAT | PSGL_DEVICE_PARAMETERS_MULTISAMPLING_MODE;
    params.colorFormat = GL_ARGB_SCE;
    params.depthFormat = GL_DEPTH_COMPONENT24;
    params.multisamplingMode = GL_MULTISAMPLING_NONE_SCE;
    
    params.enable |= PSGL_DEVICE_PARAMETERS_WIDTH_HEIGHT;
    params.width = deviceWidth;
    params.height = deviceHeight;
    
    PSGLdevice *device = psglCreateDeviceExtended(&params);
	
    // (5) create context
    PSGLcontext *context = psglCreateContext();
    psglMakeCurrent(context, device);
    psglResetCurrentContext();

    // Init PSGL and draw
    initGraphics(device);
    drawRotatingGrid(1);

    // Destroy the context, then the device (before psglExit)
    psglDestroyContext(context);
    psglDestroyDevice(device);
  }
  else
  {
    printf("%d x %d is NOT available...\n",deviceWidth,deviceHeight);
  }

  psglExit();
}
