/*  SCE CONFIDENTIAL                                       */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*  Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*  All Rights Reserved.                                   */
/*  File: common.h
 *  Description:
 *  simple sample to show how to use savedata system utility
 *
 */

#include <sys/paths.h>
#include <cell/l10n.h>

/* --- debug -------------------------------------------------- */
#define DEBUG_MODE

#ifdef DEBUG_MODE
	#define PRINTF printf
	#define ERR_PRINTF printf
	#define TRACE printf( "TRACE : %s:%s - %d\n", __FILE__, __FUNCTION__, __LINE__ )
	#define DUMP_CONTAINER
	#define DUMP_SYSMEMORY

	/* Directory where local data exists */
	#define DATA_DIR		SYS_APP_HOME "/DATA"

#else
	#define PRINTF(...)
	#define ERR_PRINTF(...)
	#define TRACE
	#define DUMP_CONTAINER
	#define DUMP_SYSMEMORY

	/* Directory where local data exists */
	/* It's necessary to specify the directory on the disc, when you make the image for the disc */
	#define DATA_DIR		SYS_DEV_BDVD "/PS3_GAME/USRDIR/DATA"

#endif

#define TRUE	(1)
#define FALSE	(0)

/* running mode */
enum {
	MODE_IDLE = 0,
	MODE_RUNNING,
	MODE_LIST_SAVE,
	MODE_LIST_LOAD,
	MODE_FIXED_SAVE,
	MODE_FIXED_LOAD,
	MODE_AUTO_SAVE,
	MODE_AUTO_LOAD,
	MODE_LIST_AUTO_SAVE,
	MODE_LIST_AUTO_LOAD,
	MODE_DELETE,
	MODE_CHECK_CALLBACK,
	MODE_EXIT
};

/* menu command */
enum {
	MCMD_LIST_SAVE,
	MCMD_LIST_LOAD,
	MCMD_FIXED_SAVE,
	MCMD_FIXED_LOAD,
	MCMD_AUTO_SAVE,
	MCMD_AUTO_LOAD,
	MCMD_LIST_AUTO_SAVE,
	MCMD_LIST_AUTO_LOAD,
	MCMD_DELETE,
	MCMD_NUM
};

/* Convert string to Utf8 */
int strcpyUtf8( L10nCode sc, uint8_t *dst, uint8_t *src, unsigned int dst_len );
