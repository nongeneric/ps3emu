/*  SCE CONFIDENTIAL                                       */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*  Copyright (C) 2009 Sony Computer Entertainment Inc.    */
/*  All Rights Reserved.                                   */
/*  File: main.c
 *  Description:
 *  simple sample to show how to use savedata system utility
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cell/pad.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <cell/sysmodule.h>

#include <sysutil_sysparam.h>
#include <sysutil_savedata.h>

#include "common.h"
#include "graphics.h"
#include "saveload.h"
#include "delete.h"

/**********************************************************************************/
/* function */
int main( void );
static void savedata_sample_main( void );
static void sysutil_savedata_sample_sysutil_callback( uint64_t status, uint64_t param, void * userdata );

static int pad_read( void );
static int isCrossButtonEnter(void);

/**********************************************************************************/
/* global variable */
int is_running = FALSE;
int receiveExitGameRequest = FALSE;
int last_result = 0;

/**********************************************************************************/
/* local variable */
static int util_drawing = FALSE;
static int is_cross_enter = FALSE;
static int running_mode = 0;
static int command_selected = 0;

/**********************************************************************************/
/* pad */

/* button[2] */
#define	CTRL_SELECT		(1<<0)
#define	CTRL_L3			(1<<1)
#define	CTRL_R3			(1<<2)
#define	CTRL_START		(1<<3)
#define	CTRL_UP			(1<<4)
#define	CTRL_RIGHT		(1<<5)
#define	CTRL_DOWN		(1<<6)
#define	CTRL_LEFT		(1<<7)
/* button[3] */
#define	CTRL_L2			(1<<8)
#define	CTRL_R2			(1<<9)
#define	CTRL_L1			(1<<10)
#define	CTRL_R1			(1<<11)
#define	CTRL_TRIANGLE	(1<<12)
#define	CTRL_CIRCLE		(1<<13)
#define	CTRL_CROSS		(1<<14)
#define	CTRL_SQUARE		(1<<15)

static struct MenuCommand_st menu_command[] = {
	{ MCMD_LIST_SAVE,		MODE_LIST_SAVE,		"cellSaveDataListSave2()" },
	{ MCMD_LIST_LOAD,		MODE_LIST_LOAD,		"cellSaveDataListLoad2()" },
	{ MCMD_FIXED_SAVE,		MODE_FIXED_SAVE,	"cellSaveDataFixedSave2()" },
	{ MCMD_FIXED_LOAD,		MODE_FIXED_LOAD,	"cellSaveDataFixedLoad2()" },
	{ MCMD_AUTO_SAVE,		MODE_AUTO_SAVE,		"cellSaveDataAutoSave2()" },
	{ MCMD_AUTO_LOAD,		MODE_AUTO_LOAD,		"cellSaveDataAutoLoad2()" },
	{ MCMD_LIST_AUTO_SAVE,	MODE_LIST_AUTO_SAVE,"cellSaveDataListAutoSave()" },
	{ MCMD_LIST_AUTO_LOAD,	MODE_LIST_AUTO_LOAD,"cellSaveDataListAutoLoad()" },
	{ MCMD_DELETE,			MODE_DELETE,		"cellSaveDataDelete2()" }
};

/* Check if cross button is assigned to "Enter" */
int isCrossButtonEnter()
{
	int ret = 0;
	int enter_button_assign;
	ret = cellSysutilGetSystemParamInt( CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN, &enter_button_assign );
	if( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysutilGetSystemParamInt() = 0x%x\n", ret );
		return 0;
	}
	else {
		if( enter_button_assign == CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS ) {
			return 1;
		}
		else {
			return 0;
		}
	}
}

int pad_read( void )
{
	int ret;
	static uint32_t	opadd;
	uint32_t		padd;

	CellPadData databuf;
	CellPadInfo2 infobuf;
	static uint32_t old_info = 0;

	ret = cellPadGetInfo2( &infobuf );
	if ( ret != 0 ) {
		ERR_PRINTF( "cellPadGetInfo2() = 0x%x\n", ret );
		opadd = 0;
		return 1;
	}
	if ( (infobuf.port_status[0] & CELL_PAD_STATUS_CONNECTED) == 0 ) {
		opadd = 0;
		return 1;
	}

    if((infobuf.system_info & CELL_PAD_INFO_INTERCEPTED) && (!(old_info & CELL_PAD_INFO_INTERCEPTED))){
			PRINTF("This program lost the ownership of the game pad data\n");
            old_info = infobuf.system_info;
    }else if((!(infobuf.system_info & CELL_PAD_INFO_INTERCEPTED)) && (old_info & CELL_PAD_INFO_INTERCEPTED)){
			PRINTF("This program got the ownership of the game pad data\n");
            old_info = infobuf.system_info;
    }

	ret = cellPadGetData( 0, &databuf );
	if ( ret != 0 ) {
		ERR_PRINTF( "cellPadGetData() = 0x%x\n", ret );
		opadd = 0;
		return 1;
	}
	if ( databuf.len == 0 ) {
		opadd = 0;
		return 1;
	}

	padd = ( databuf.button[2] | ( databuf.button[3] << 8 ) ); 

	if ( padd & CTRL_UP ) {
		if ( opadd != padd ){
			if ( command_selected == 0 ) {
				command_selected = MCMD_NUM - 1;
			}
			else {
				command_selected--;
			}
		}
		if( running_mode != MODE_IDLE ) {
			PRINTF("CTRL_UP was pressed during utility running\n");
		}
	}
	else if ( padd & CTRL_DOWN ) {
		if ( opadd != padd ){
			if ( command_selected == MCMD_NUM - 1 ) {
				command_selected = 0;
			}
			else {
				command_selected++;
			}
		}
		if( running_mode != MODE_IDLE ) {
			PRINTF("CTRL_DOWN was pressed during utility running\n");
		}
	}
	else if ( padd & CTRL_CIRCLE ) {
		if ( opadd != padd ){
			if ( running_mode == MODE_IDLE ) {
				if( !is_cross_enter ) {
					running_mode = menu_command[command_selected].mode;
				}
			}
			else {
				PRINTF("press CTRL_CIRCLE during utility running\n");
			}
		}
	}
	else if ( padd & CTRL_CROSS ) {
		if ( opadd != padd ){
			if ( running_mode == MODE_IDLE ) {
				if( is_cross_enter ) {
					running_mode = menu_command[command_selected].mode;
				}
			}
			else {
				PRINTF("press CTRL_CROSS during utility running\n");
			}
		}
	}
	/* PRINTF("0x%x  >>  0x%x\n", opadd, padd ); */

	opadd = padd;

	return 1;
}

/**********************************************************************************/
/* main */

/* specify primary ppu thread's priority and stack size */
SYS_PROCESS_PARAM(1001, 0x10000)

#define SAVEDATA_SAMPLE_SYSUTIL_CALLBACK_SLOT_EXITGAME	(0)

int main( void )
{
	int ret = 0;

	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_IO );
	if ( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysmoduleLoadModule( CELL_SYSMODULE_IO ) = 0x%x\n", ret );
		return ret;
	}
	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_GCM );
	if ( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysmoduleLoadModule( CELL_SYSMODULE_GCM ) = 0x%x\n", ret );
		return ret;
	}
	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FS );
	if ( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysmoduleLoadModule( CELL_SYSMODULE_FS ) = 0x%x\n", ret );
		return ret;
	}
	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_L10N );
	if ( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysmoduleLoadModule( CELL_SYSMODULE_L10N ) = 0x%x\n", ret );
		return ret;
	}

	/* Check if cross button is assigned to "Enter" */
	//is_cross_enter = isCrossButtonEnter();

	void *host_addr = memalign(1024*1024, HOST_SIZE);
	if( (ret = cellGcmInit(CB_SIZE, HOST_SIZE, host_addr)) != CELL_OK) {
		ERR_PRINTF("ERROR : cellGcmInit() = 0x%x\n", ret );
		return -1;
	}

	if (initDisplay()!=0)	return -1;

	//initShader();

	//setDrawEnv();

	/*if (setRenderObject()) {
		return -1;
	}*/

	/*ret = cellPadInit( 1 );
	if ( ret != 0 ) {
		ERR_PRINTF( "cellPadInit() = 0x%x\n", ret );
		return ret;
	}*/

	ret = cellSysutilRegisterCallback( SAVEDATA_SAMPLE_SYSUTIL_CALLBACK_SLOT_EXITGAME, sysutil_savedata_sample_sysutil_callback, NULL );
	if ( ret != 0 ) {
		ERR_PRINTF( "ERROR : cellSysutilRegisterCallback() = 0x%x\n", ret );
		return ret;
	}

	/*J Setting for background rendering */
	/* cellSaveDataEnableOverlay( TRUE ); */

	//setRenderState();

	/* 1st time */
	//setRenderTarget();

	//initDbgFont();

	receiveExitGameRequest = FALSE;
	running_mode = MODE_IDLE;

	/* rendering loop */
	//while ( pad_read() != 0 && running_mode != MODE_EXIT) {
		/* clear frame buffer */
		/*cellGcmSetClearSurface(gCellGcmCurrentContext,
				CELL_GCM_CLEAR_Z|
				CELL_GCM_CLEAR_R|
				CELL_GCM_CLEAR_G|
				CELL_GCM_CLEAR_B|
				CELL_GCM_CLEAR_A);*/
		/* set draw command */
		//cellGcmSetDrawArrays(gCellGcmCurrentContext, CELL_GCM_PRIMITIVE_TRIANGLES, 0, 3);

		//if( !util_drawing ) {
			/* draw Menu */
			//drawListMenu( menu_command, MCMD_NUM, command_selected );
		//}

		/* draw ResultWindow */
		//drawResultWindow( last_result, is_running );
		//drawDbgFont();

		/* start reading the command buffer */
		//flip();

		//savedata_sample_main();

	is_running = true;
	auto_save();

	while (is_running) {
		ret = cellSysutilCheckCallback();
		if ( ret ) {
			ERR_PRINTF( "cellSysutilCheckCallback() = 0x%x\n", ret );
		}
		sys_timer_usleep(5000);
	}

	is_running = true;
	auto_load();

	while (is_running) {
		ret = cellSysutilCheckCallback();
		if ( ret ) {
			ERR_PRINTF( "cellSysutilCheckCallback() = 0x%x\n", ret );
		}
		sys_timer_usleep(5000);
	}

	//}

	//termDbgFont();

	free( host_addr );

	ret = cellSysutilUnregisterCallback(SAVEDATA_SAMPLE_SYSUTIL_CALLBACK_SLOT_EXITGAME);
	if( ret ) {
		return -1;
	}
	
	/*ret = cellPadEnd();
	if ( ret != 0 ) {
		ERR_PRINTF( "cellPadEnd() = 0x%x\n", ret );
		return ret;
	}*/

	ret = cellSysmoduleUnloadModule( CELL_SYSMODULE_L10N );
	if ( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysmoduleUnloadModule( CELL_SYSMODULE_L10N ) = 0x%x\n", ret );
		return ret;
	}
	ret = cellSysmoduleUnloadModule( CELL_SYSMODULE_FS );
	if ( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysmoduleUnloadModule( CELL_SYSMODULE_FS ) = 0x%x\n", ret );
		return ret;
	}
	ret = cellSysmoduleUnloadModule( CELL_SYSMODULE_GCM );
	if ( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysmoduleUnloadModule( CELL_SYSMODULE_GCM ) = 0x%x\n", ret );
		return ret;
	}
	ret = cellSysmoduleUnloadModule( CELL_SYSMODULE_IO );
	if ( ret != CELL_OK ) {
		ERR_PRINTF( "ERROR : cellSysmoduleUnloadModule( CELL_SYSMODULE_IO ) = 0x%x\n", ret );
		return ret;
	}
	//PRINTF("\n\n\nBYE\n\n\n");
	return 0;
}

void savedata_sample_main( void )
{
	int ret = 0;

	switch ( running_mode ) {
	case MODE_LIST_SAVE:
		{
			ret = list_save();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_LIST_LOAD:
		{
			ret = list_load();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_FIXED_SAVE:
		{
			ret = fixed_save();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_FIXED_LOAD:
		{
			ret = fixed_load();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_AUTO_SAVE:
		{
			ret = auto_save();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_AUTO_LOAD:
		{
			ret = auto_load();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_LIST_AUTO_SAVE:
		{
			ret = list_auto_save();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_LIST_AUTO_LOAD:
		{
			ret = list_auto_load();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_DELETE:
		{
			ret = delete_savedata();
			if( ret == 0 ) {
				is_running = TRUE;
				running_mode = MODE_RUNNING;
			}
			else {
				running_mode = MODE_IDLE;
			}
		}
		break;
	case MODE_IDLE:
	case MODE_RUNNING:
		/* Don't forget to check system callback */
		ret = cellSysutilCheckCallback();
		if ( ret ) {
			ERR_PRINTF( "cellSysutilCheckCallback() = 0x%x\n", ret );
		}
		if(!is_running) {
			running_mode = MODE_IDLE;
		}
		break;
	default:
		break;
	}

	/* If receive the request of CELL_SYSUTIL_REQUEST_EXITGAME */
	/* during calling API of savedata utility,                 */
	/* must wait for API's return before exiting game          */
	if( receiveExitGameRequest ) {
		if( !is_running ) {
			PRINTF("exit!!!\n");
			running_mode = MODE_EXIT;
		}
		PRINTF("waiting....\n");
		return;
	}

}

/* The callback function for handling common events, including the game termination request event */
void sysutil_savedata_sample_sysutil_callback( uint64_t status, uint64_t param, void * userdata )
{
	(void)param ;
	(void)userdata ;

	 /* PRINTF("sysutil savedata sample callback status=0x%x\n", (int)status); */

	switch(status)
	{
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		PRINTF("sysutil savedata sample receive CELL_SYSUTIL_REQUEST_EXITGAME \n");

		/* Game termination request */
		receiveExitGameRequest = TRUE ;

		break;
	case CELL_SYSUTIL_DRAWING_BEGIN:

		/* SystemUtility starts drawing something else */
		util_drawing = TRUE;

		break;
	case CELL_SYSUTIL_DRAWING_END:
		
		/* SystemUtility ends drawing */
		util_drawing = false;

		break;
	default:
		ERR_PRINTF("sysutil savedata sample receive status : 0x%llx\n", status);
		break;
	}
}

/**********************************************************************************/
int strcpyUtf8( L10nCode sc, uint8_t *dst, uint8_t *src, unsigned int dst_size )
{
	int result;
	l10n_conv_t cd;
	unsigned int src_len, dst_len;
	uint8_t *strUtf8 = NULL;

	src_len = strlen((char*)src);
	dst_len = dst_size;
	strUtf8 = (uint8_t*)malloc( dst_len );
	if( strUtf8 == NULL ) {
		ERR_PRINTF("strcpyUtf8: failed. no memory\n");
		goto exit;
	}
	memset( strUtf8, 0x00, dst_len );

	if( (cd = l10n_get_converter( sc, L10N_UTF8 )) == -1 ) {
		ERR_PRINTF("l10n_get_converter: no such converter\n");
		goto exit;
	}

	result = l10n_convert_str( cd, src, &src_len, strUtf8, &dst_len );
	if( result != ConversionOK ) {
		ERR_PRINTF("l10n_convert: conversion error : 0x%x\n", result);
		goto exit;
	}
	/* PRINTF(" src_len : %d  dst_len : %d\n", src_len, dst_len ); */
exit:
	if( strUtf8 != NULL ) {
		strncpy( (char*)dst, (char*)strUtf8, dst_size );
		dst[dst_size-1] = 0;
		free(strUtf8);
	}
	else {
		dst[0] = 0;
		return -1;
	}

	return 0;
}
