/*  SCE CONFIDENTIAL                                       */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*  Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*  All Rights Reserved.                                   */
/*  File: graphics.h
 *  Description:
 *  simple sample to show how to use savedata system utility
 *
 */

#include <cell/gcm.h>
#include <cell/dbgfont.h>

#define HOST_SIZE (1*1024*1024)
#define CB_SIZE	(0x10000)

void initShader(void);
int32_t initDisplay(void);
void setDrawEnv(void);
void setRenderTarget(void);
void setRenderState(void);
int32_t setRenderObject(void);
void flip(void);

struct MenuCommand_st {
	int command;
	int mode;
	char string[32];
};

void drawListMenu( struct MenuCommand_st *menu_command, int size, int focused );
void drawResultWindow( int result, int busy );

int initDbgFont(void);
int termDbgFont(void);
int drawDbgFont(void);
int DbgPrintf(const char *string, ...);

