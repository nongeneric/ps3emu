/*  SCE CONFIDENTIAL                                       */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*  Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*  All Rights Reserved.                                   */
/*  File: saveload.h
 *  Description:
 *  simple sample to show how to use savedata system utility
 *
 */

#include <sysutil_savedata.h>

/* Exec cellSaveDataListSave2() */
int list_save( void );

/* Exec cellSaveDataListLoad2() */
int list_load( void );

/* Exec cellSaveDataFixedSave2() */
int fixed_save( void );

/* Exec cellSaveDataFixedLoad2() */
int fixed_load( void );

/* Exec cellSaveDataAutoSave2() */
int auto_save( void );

/* Exec cellSaveDataAutoLoad2() */
int auto_load( void );

/* Exec cellSaveDataListAutoSave() */
int list_auto_save( void );

/* Exec cellSaveDataListAutoLoad() */
int list_auto_load( void );

/* Dump CellSaveDataListGet structure */
void dumpCellSaveDataListGet( CellSaveDataListGet *get );
