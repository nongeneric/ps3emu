/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#include <stdio.h>
#include <assert.h>
#include <cell/gcm.h>
#include <cell/dbgfont.h>

#include "gcmutil.h"
#include "snaviutil.h"

#include "main.h"
#include "FWCellGCMWindow.h"

//#define USE_MAIN_MEMORY
#define HEAP_SIZE 1048576

using namespace cell::Gcm;

DbgFontApp app;

//-----------------------------------------------------------------------------
//E  Description: Constructor
//-----------------------------------------------------------------------------
DbgFontApp::DbgFontApp()
{
	// 
}

//-----------------------------------------------------------------------------
//E  Description: Destructor
//-----------------------------------------------------------------------------
DbgFontApp::~DbgFontApp()
{
	//
}

//-----------------------------------------------------------------------------
//E  Description: Initialization callback
//-----------------------------------------------------------------------------
bool DbgFontApp::onInit(int argc, char **ppArgv)
{
	FWGCMApplication::onInit(argc, ppArgv);

	//E  degug font initialization
	return dbgFontInit();
}

//-----------------------------------------------------------------------------
//E  Description: Render callback
//-----------------------------------------------------------------------------
void DbgFontApp::onRender()
{
	//E  base implementation clears screen
	FWGCMApplication::onRender();

	//E  degug font rendering
	dbgFontDraw();

	if (mFrame > 1)
		exit(0);
}

//-----------------------------------------------------------------------------
//E  Description: Update callback
//-----------------------------------------------------------------------------
bool DbgFontApp::onUpdate()
{
	dbgFontPut();

	mFrame++;

	// for sample navigator
	if(cellSnaviUtilIsExitRequested(NULL)) return false;

	if(FWGCMApplication::onUpdate() == false) return false;
		
	return true;
}

//-----------------------------------------------------------------------------
//E  Description: Resize callback
//-----------------------------------------------------------------------------
void DbgFontApp::onSize(const FWDisplayInfo& rDispInfo)
{
	FWGCMApplication::onSize(rDispInfo);
}

//-----------------------------------------------------------------------------
//E  Description: Shutdown callback
//-----------------------------------------------------------------------------
void DbgFontApp::onShutdown()
{
	dbgFontExit();

	FWGCMApplication::onShutdown();
}


//-----------------------------------------------------------------------------
//E  Description: Debug font initialization function
//-----------------------------------------------------------------------------
bool DbgFontApp::dbgFontInit(void)
{
	int frag_size = CELL_DBGFONT_FRAGMENT_SIZE;
	int vertex_size = 512 * CELL_DBGFONT_VERTEX_SIZE;
 	int font_tex = CELL_DBGFONT_TEXTURE_SIZE;

	int local_size = frag_size + vertex_size + font_tex;
	void* localmem = (void*)cellGcmUtilAllocateLocalMemory(local_size, 128);

#ifdef USE_MAIN_MEMORY
	local_size = frag_size;
	int main_size = vertex_size + font_tex;
	cellGcmUtilInitializeMainMemory(HEAP_SIZE);
	void* mainmem = (void*)cellGcmUtilAllocateMainMemory(main_size, 128);
#endif //USE_MAIN_MEMORY

	CellDbgFontConfigGcm cfg;
	memset(&cfg, 0, sizeof(CellDbgFontConfigGcm));
	cfg.localBufAddr = (sys_addr_t)localmem; 
	cfg.localBufSize = local_size;
	cfg.mainBufAddr = NULL;
	cfg.mainBufSize  = 0;
#ifdef USE_MAIN_MEMORY
	cfg.mainBufAddr = (sys_addr_t)mainmem;;
	cfg.mainBufSize = main_size;
#endif //USE_MAIN_MEMORY
	cfg.screenWidth = mDispInfo.mWidth;
	cfg.screenHeight = mDispInfo.mHeight;

	cfg.option = CELL_DBGFONT_VERTEX_LOCAL;
	cfg.option |= CELL_DBGFONT_TEXTURE_LOCAL;
#ifdef USE_MAIN_MEMORY
	cfg.option = CELL_DBGFONT_VERTEX_MAIN;
	cfg.option |= CELL_DBGFONT_TEXTURE_MAIN;
#endif //USE_MAIN_MEMORY
	cfg.option |= CELL_DBGFONT_SYNC_ON;
	cfg.option |= CELL_DBGFONT_VIEWPORT_ON;
	cfg.option |= CELL_DBGFONT_MINFILTER_NEAREST;
	cfg.option |= CELL_DBGFONT_MAGFILTER_NEAREST;
	int ret = cellDbgFontInitGcm(&cfg);
	if (ret != CELL_OK){
		printf("libdbgfont init failed %x\n", ret);
		return false;
	}
	
	CellDbgFontConsoleConfig ccfg[2];
	memset(ccfg, 0, sizeof(ccfg));
	ccfg[0].posLeft     = 0.1f;
	ccfg[0].posTop      = 0.6f;
	ccfg[0].cnsWidth    = 20;
	ccfg[0].cnsHeight   = 6;
	ccfg[0].scale       = 1.5f;
	ccfg[0].color       = 0xffff88cc;
	mDbgFontID[0]       = cellDbgFontConsoleOpen(&ccfg[0]);
	if (mDbgFontID[0] < 0) {

		printf("cellDbgFontConsoleOpen() failed %x\n", mDbgFontID[0]);
		return false;
	}

	ccfg[1].posLeft     = 0.55f;
	ccfg[1].posTop      = 0.6f;
	ccfg[1].cnsWidth    = 30;
	ccfg[1].cnsHeight   = 8;
	ccfg[1].scale       = 1.0f;
	ccfg[1].color       = 0xff88ccff;
	mDbgFontID[1]       = cellDbgFontConsoleOpen(&ccfg[1]);
	if (mDbgFontID[1] < 0) {
		printf("cellDbgFontConsoleOpen() failed %x\n", mDbgFontID[0]);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//E  Description: Debug font put function
//-----------------------------------------------------------------------------
void DbgFontApp::dbgFontPut(void)
{
	static const int CONS_PUTS_INTERVAL = 50;
	static const int CONS_NUM_STRINGS0   = 6;
	static const int CONS_NUM_STRINGS1   = 3;
	
	char testchar0[CONS_NUM_STRINGS0][256] = {
		"------------------",
		"libdbgfont",
		"        for libgcm",
		"------------",
		"Sample program",
		"simple_console_gcm",
	};
	char testchar1[CONS_NUM_STRINGS1][256] = {
		"The quick brown fox ",
		"jumps over the lazy dog",
		"\n"
	};

	cellDbgFontConsolePuts(mDbgFontID[0], testchar0[0]);
	cellDbgFontConsolePuts(mDbgFontID[0], testchar0[1]);
	cellDbgFontConsolePuts(mDbgFontID[0], testchar0[2]);
	cellDbgFontConsolePuts(mDbgFontID[0], testchar0[3]);
	cellDbgFontConsolePuts(mDbgFontID[0], testchar0[4]);
	cellDbgFontConsolePrintf(mDbgFontID[1], "%d: %s\n", 1, testchar1[0]);
	cellDbgFontConsolePrintf(mDbgFontID[1], "%d: %s\n", 2, testchar1[1]);
	cellDbgFontConsolePrintf(mDbgFontID[1], "%d: %s", 3, testchar1[2]);

	cellDbgFontPuts(0.09f, 0.2f, 3.0f, 0xffccff88, "Debug Font Library GCM");
}

//-----------------------------------------------------------------------------
//E  Description: Debug font draw function
//-----------------------------------------------------------------------------
void DbgFontApp::dbgFontDraw(void)
{
	cellDbgFontDrawGcm();
}

//-----------------------------------------------------------------------------
//E  Description: Debug font exit function
//-----------------------------------------------------------------------------
void DbgFontApp::dbgFontExit(void)
{
	cellDbgFontConsoleClose(mDbgFontID[0]);
	cellDbgFontConsoleClose(mDbgFontID[1]);
	cellDbgFontExitGcm();
}
