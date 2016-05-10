/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *                Copyright (C) 2010 Sony Computer Entertainment Inc.
 *                                               All Rights Reserved.
 */
#include <math.h>
#include <cell/fs/cell_fs_file_api.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/process.h>
#include <sys/spu_initialize.h>
#include <sys/paths.h>

#include <PSGL/psgl.h>
#include <PSGL/psglu.h>
#include <Cg/cg.h>

#include <cell/dbgfont.h>

#include "../../Common/gfxCommon.h"
#include "../../Common/gfxObject.h"
#include "../../Common/gfxPad.h"
#include "../../Common/gfxDdsLoader.h"

#include <sys/sys_time.h>

#include "../Data/cube.h"		// our test data

using namespace Vectormath;
using namespace Aos;

#define SAMPLE_NAME "Mip Mapping Example"

// switch to true to run-time compile the shaders 
const bool LOAD_SHADER_BINARY = true; 
bool VSYNC_ON = false; 

// shader binary 
#define VERTEX_PROGRAM_BINARY		REMOTE_PATH"/shaders/Tutorial/vs_basicSample.cgelf"
#define FRAGMENT_PROGRAM_BINARY		REMOTE_PATH"/shaders/Tutorial/fs_basicSample.cgelf"

// shader sources 
#define VERTEX_PROGRAM_SOURCE		REMOTE_PATH"/shaders/Tutorial/vs_basicSample.cg"
#define FRAGMENT_PROGRAM_SOURCE		REMOTE_PATH"/shaders/Tutorial/fs_basicSample.cg"

// Textures 
#define MIPMAP_TEXTURE			REMOTE_PATH"images/Tutorial/test512.dds"
#define NOMIPMAP_TEXTURE		REMOTE_PATH"images/Tutorial/test512nomip.dds"



gfxObject Object[2];			//a simple object to render

CGprogram 		VertexProgram;			//loaded vertex program
CGprogram 		FragmentProgram;		//loaded fragment program
	
CGparameter  		ModelViewProj_cgParam;		//uniform parameter in vertex program
CGparameter		DiffuseMap_cgParam;		//uniform parameter in fragment program

Vector3       		ViewPos;    
Vector3       		ViewAng;
Matrix4       		View;									//current view ( camera ) matrix

//-----------------------------------------------------------------------------
// DEFINES
//-----------------------------------------------------------------------------
#define CAM_SPEED 0.5f
#define CAM_RPY_SPEED 1.0f
#define DEFAULT_VIEW_DIST  -5.f		// setting up the view

#define NEAR_CLIP	0.1f
#define FAR_CLIP	8500.1f
#define FOV_Y		40.0f			//field of view in the y direction


// FPS REPORTING
float 			FPS = 100.0f; 
float			LastTime = 0;
const static float 	REPORT_TIME = 2.0f;  
int 			Frames = 0;
int 			FramesLastReport = 0;
double 			TimeElapsed = 0;
double 			TimeLastReport = 0;
double 			TimeReport = 0;
unsigned int 		Counter = 0;		//frame counter



void sampleInit() 
{
	
  Counter=0;
  // FPS REPORTING
  Frames=0;
  FramesLastReport=0;
  TimeElapsed=0.0;
  TimeLastReport=0.0;
  TimeReport=REPORT_TIME;

  glClearColor(0.3f,0.3f,0.7f, 0.0f);
  glClearDepthf(1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable (GL_CULL_FACE);

  //initialize the objects
  Object[0].setMesh(CUBEVCOUNT,cubeST,cubeNorm,cubeVert);
  Object[0].mTexture = gfxLoadDDSTexture(MIPMAP_TEXTURE);
  Object[0].mPos = Vector3(-1.0f,0.0f,0.0f);
  Object[0].mRot = Vector3(0.f,-0.15f,0.f);
  
  Object[1].setMesh(CUBEVCOUNT,cubeST,cubeNorm,cubeVert);
  Object[1].mTexture = gfxLoadDDSTexture(NOMIPMAP_TEXTURE);
  Object[1].mPos = Vector3( 1.0f,0.0f,0.0f);  
  Object[1].mRot = Vector3(0.f,0.15f,0.f);

  if ( LOAD_SHADER_BINARY )
  {
  	// load the shader programs
	VertexProgram      = gfxLoadProgramFromFile(cgGLGetLatestProfile(CG_GL_VERTEX), VERTEX_PROGRAM_BINARY);
	FragmentProgram    = gfxLoadProgramFromFile(cgGLGetLatestProfile(CG_GL_FRAGMENT), FRAGMENT_PROGRAM_BINARY);
  }
  else
  {
  	// load the shader programs
	VertexProgram      = gfxLoadProgramFromSource(cgGLGetLatestProfile(CG_GL_VERTEX), VERTEX_PROGRAM_SOURCE);
	FragmentProgram    = gfxLoadProgramFromSource(cgGLGetLatestProfile(CG_GL_FRAGMENT), FRAGMENT_PROGRAM_SOURCE);
  }

  // bind and enable the vertex and fragment programs
  cgGLBindProgram(VertexProgram);
  cgGLBindProgram(FragmentProgram);
  cgGLEnableProfile(cgGLGetLatestProfile(CG_GL_VERTEX));
  cgGLEnableProfile(cgGLGetLatestProfile(CG_GL_FRAGMENT));

  //get handles to the uniform cg parameters that need setting
  ModelViewProj_cgParam = cgGetNamedParameter(VertexProgram, "modelViewProj");      gfxCheckCgError (__LINE__);
  DiffuseMap_cgParam = cgGetNamedParameter (FragmentProgram, "diffuseMap");         gfxCheckCgError (__LINE__);

  //set the viewing parameters
  ViewPos = Vector3(0.0f,0.0f,DEFAULT_VIEW_DIST);
  ViewAng = Vector3(0.0f,0.0f,0.0f);

  //FPS reporting (disable vsync to get true time)
  glDisable(GL_VSYNC_SCE);
  
  // last the projection matrix 
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluPerspectivef(FOV_Y, gfxGetAspectRatio(), NEAR_CLIP,FAR_CLIP);
  glMatrixMode( GL_MODELVIEW );

}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// void sampleUpdateMatrices()
// Description: 
// Returns: NA
// Notes:
//-----------------------------------------------------------------------------
void sampleUpdateMatrices()
{
  Object[0].update();		//updates the objects orientation and local to world matrix
  Object[1].update();		//updates the objects orientation and local to world matrix
  
  // update the view matrix
  View   = Matrix4::identity ();
  Matrix4 trans   = Matrix4::translation (ViewPos);	 // set the translation
  Matrix4 rotX    = Matrix4::rotationX (ViewAng[0]);	 // set an x rotation  
  Matrix4 rotY    = Matrix4::rotationY (ViewAng[1]);	 // set a y roation 
  View =   trans * rotX * rotY ;	//multiply them all together

  Counter++;

}
//-----------------------------------------------------------------------------


void sampleUpdateInput()
{
	float Speed = .01f; 
	
	gfxPadRead(); 
	
	// Move the camera position around based on the DPad and RightStick movement. 
	if(gfxDpadUp(0))		
		ViewPos[1] += Speed; 
	if(gfxDpadDown(0))		
		ViewPos[1] -= Speed; 	
	if(gfxDpadLeft(0))		 
		ViewPos[0] -= Speed; 			
	if(gfxDpadRight(0))		
		ViewPos[0] += Speed; 			
	if(gfxR1Down(0))
		ViewPos[2] += Speed; 
	if(gfxR2Down(0))	
		ViewPos[2] -= Speed; 	

	// Rotated the camera angle about X and Y based on LeftStick movement. 
	if(gfxL1Down(0))
		ViewAng[0] += Speed; 
	if(gfxL2Down(0))
		ViewAng[1] -= Speed; 	


	// toggle VSYNC 
	if ( gfxDpadCross(0) )
		VSYNC_ON = true; 
	if ( gfxDpadSquare(0) )
		VSYNC_ON = false; 
	
	
	// update vsync state 
	if ( VSYNC_ON )
	  glEnable(GL_VSYNC_SCE);
	else 
   	  glDisable(GL_VSYNC_SCE); 

}

//-----------------------------------------------------------------------------
// void sampleRender()
// Description: 
// Returns: NA
// Notes:
//-----------------------------------------------------------------------------
void sampleRender()
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  //set the modelview matrix to contain the view matrix
  glPushMatrix();
  glLoadIdentity();
  glLoadMatrixf((GLfloat*)&View);                            

  glPushMatrix();

  cgGLEnableProfile(cgGLGetLatestProfile(CG_GL_VERTEX));	//enable the shaders
  cgGLEnableProfile(cgGLGetLatestProfile(CG_GL_FRAGMENT));
  
  //render the objects
  glPushMatrix();
  
  glMultMatrixf((GLfloat*)&Object[0].mLocalToWorld);		//load the local to world matrix
  cgGLSetStateMatrixParameter (ModelViewProj_cgParam, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY); gfxCheckCgError (__LINE__);
  cgGLSetTextureParameter (DiffuseMap_cgParam, Object[0].mTexture);   gfxCheckCgError (__LINE__);
  cgGLEnableTextureParameter (DiffuseMap_cgParam);                     gfxCheckCgError (__LINE__);
  Object[0].render();		//draws the object
  glPopMatrix();

  glPushMatrix();
  glMultMatrixf((GLfloat*)&Object[1].mLocalToWorld);		//load the local to world matrix
  cgGLSetStateMatrixParameter (ModelViewProj_cgParam, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY); gfxCheckCgError (__LINE__);
  cgGLSetTextureParameter (DiffuseMap_cgParam, Object[1].mTexture);   gfxCheckCgError (__LINE__);
  cgGLEnableTextureParameter (DiffuseMap_cgParam);                     gfxCheckCgError (__LINE__);
  Object[1].render();		//draws the object
  glPopMatrix();
	  
  cgGLDisableTextureParameter (DiffuseMap_cgParam);                  gfxCheckCgError (__LINE__);
  cgGLDisableProfile(cgGLGetLatestProfile(CG_GL_VERTEX));	//disable the shaders
  cgGLDisableProfile(cgGLGetLatestProfile(CG_GL_FRAGMENT));

  glPopMatrix();
  glPopMatrix();


  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);

}

void sampleUpdateFPS()
{


  // FPS REPORTING
  // get current timing info
  float timeNow = (float)sys_time_get_system_time() * .000001f;
  float fElapsedInFrame = (float)(timeNow - LastTime);
  LastTime = timeNow;
  ++Frames;
  TimeElapsed+=fElapsedInFrame;
  // report fps at appropriate interval
  if (TimeElapsed>=TimeReport)
  {
  	FPS = (Frames-FramesLastReport)*1.f/(float)(TimeElapsed-TimeLastReport);
	printf("FPS: %.2f\n",FPS);
	TimeReport+=REPORT_TIME;
	TimeLastReport=TimeElapsed;
	FramesLastReport=Frames;
  }

  dbgFontPrintf(40,60,0.5f,"%s %.5f FPS", SAMPLE_NAME, FPS );
  if( VSYNC_ON )
  	dbgFontPrintf(40,70,0.5f,"VSYNC ON" );
  else
  	dbgFontPrintf(40,70,0.5f,"VSYNC OFF" );

  	
  dbgFontDraw();

}

SYS_PROCESS_PARAM(1001, 0x10000)

int main()
{
	// Initialize 6 SPUs but reserve 1 SPU as a raw SPU for PSGL
	sys_spu_initialize(6, 1);	
	
	// init PSGL and get the current system width and height
	gfxInitGraphics();
	
	// initalize the PAD 	
	gfxInitPad(); 
	
	// initalize the dbgFonts 
	dbgFontInit();		
	
	// initialize sample data and projection matrix 
	sampleInit();
	
	while (1)
	{
		// PSGL doesn't clear the screen on startup, so let's do that here.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		// update based on new pad state 
		sampleUpdateInput();
		// update the matrices 
		sampleUpdateMatrices();
		// render 
		sampleRender(); 
		// update FPS
		//sampleUpdateFPS(); 
				
		// swap PSGL buffers 
		psglSwap();

		return 0;
	}
}

