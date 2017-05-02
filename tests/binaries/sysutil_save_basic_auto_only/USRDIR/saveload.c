/*  SCE CONFIDENTIAL                                       */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*  Copyright (C) 2008 Sony Computer Entertainment Inc.    */
/*  All Rights Reserved.                                   */
/*  File: saveload.c
 *  Description:
 *  simple sample to show how to use savedata system utility
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/paths.h>
#include <cell/cell_fs.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>

#include "common.h"
#include "saveload.h"

/**********************************************************************************/
/* function */
static void thr_list_save(uint64_t arg);
static void thr_list_load(uint64_t arg);
static void thr_fixed_save(uint64_t arg);
static void thr_fixed_load(uint64_t arg);
static void thr_auto_save(uint64_t arg);
static void thr_auto_load(uint64_t arg);
static void thr_list_auto_save(uint64_t arg);
static void thr_list_auto_load(uint64_t arg);

static void cb_data_list_save( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set );
static void cb_data_list_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set );
static void cb_data_fixed_save( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set );
static void cb_data_fixed_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set );
static void cb_list_auto_save( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set );
static void cb_list_auto_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set );
static void cb_data_status_save( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set );
static void cb_data_status_load( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set );
static void cb_file_operation_save( CellSaveDataCBResult *result, CellSaveDataFileGet *get, CellSaveDataFileSet *set );
static void cb_file_operation_load( CellSaveDataCBResult *result, CellSaveDataFileGet *get, CellSaveDataFileSet *set );

static void dumpCellSaveDataStatGet( CellSaveDataStatGet *get );
static int prepareIndicator( int mode, CellSaveDataAutoIndicator **_indicator );

static int fileSimpleSave(const char *filePath, void *buf, unsigned int fileSize);
static int fileAllocLoad(const char *filePath, void **buf, unsigned int *size);

/**********************************************************************************/
/* global variable */
extern int is_running;
extern int receiveExitGameRequest;
extern int last_result;

/**********************************************************************************/
#define SAVEDATA_CMD_PRIO			1001
#define SAVEDATA_CMD_STACKSIZE		(16*1024)

/* Name of the PPU thread can be specified with upto 27 characters (excluding the null-terminator) */
#define SAVEDATA_THREAD_NAME_LIST_SAVE		"sdu_sample_list_save"
#define SAVEDATA_THREAD_NAME_LIST_LOAD		"sdu_sample_list_load"
#define SAVEDATA_THREAD_NAME_FIXED_SAVE		"sdu_sample_fixed_save"
#define SAVEDATA_THREAD_NAME_FIXED_LOAD		"sdu_sample_fixed_load"
#define SAVEDATA_THREAD_NAME_AUTO_SAVE		"sdu_sample_auto_save"
#define SAVEDATA_THREAD_NAME_AUTO_LOAD		"sdu_sample_auto_load"
#define SAVEDATA_THREAD_NAME_LIST_AUTO_SAVE	"sdu_sample_list_auto_save"
#define SAVEDATA_THREAD_NAME_LIST_AUTO_LOAD	"sdu_sample_list_auto_load"

/* Encrypted ID for protected data file (*) You must edit these binaries!! */
const char secureFileId[CELL_SAVEDATA_SECUREFILEID_SIZE] = {
	0x23, 0x0, 0x0, 0x0, 0x0, 0x0, 0x13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x17
};

static int _fileOpCbStep = 0;		/* Progress counter in file operation callback */
static int _optionFileExist = 0;	/* existing of USR-DATA file */
static void *_file_buffer = NULL;	/* Buffer to read file */

/* Calculate size of file (kilobytes) */
#define SIZEKB(x)	((x+1023)/1024)
/* Get greater value from x and y */
#define MAX(x,y)	((x>y)? x:y)
/* Get lesser value from x and y */
#define MIN(x,y)	((x<y)? x:y)

/* Savedata main file (this file should exist) */
#define MUSTEXISTFILE_NAME	"SYS-DATA"
#define MUSTEXISTFILE_SIZE	(2345)
/* Savedata option file (even if this file doesn't exist, it is not a problem) */
#define OPTIONFILE_NAME		"USR-DATA"
#define OPTIONFILE_SIZE		(678)

/* Content information files and size */
#define CONTENTSFILE1_LIST_NAME		"CANONBALL/ICON0.PNG"
#define CONTENTSFILE1_LIST_SIZE		(68088)
#define CONTENTSFILE2_LIST_NAME		"CANONBALL/ICON1.PAM"
#define CONTENTSFILE2_LIST_SIZE		(1007616)
#define CONTENTSFILE3_LIST_NAME		"CANONBALL/PIC1.PNG"
#define CONTENTSFILE3_LIST_SIZE		(824134)

#define CONTENTSFILE1_FIXED_NAME	"MONG/ICON0.PNG"
#define CONTENTSFILE1_FIXED_SIZE	(70797)
#define CONTENTSFILE2_FIXED_NAME	"MONG/ICON1.PAM"
#define CONTENTSFILE2_FIXED_SIZE	(2465792)
#define CONTENTSFILE3_FIXED_NAME	"MONG/PIC1.PNG"
#define CONTENTSFILE3_FIXED_SIZE	(89252)

#define CONTENTSFILE1_AUTO_NAME		"FLYINGBLOCK/ICON0.PNG"
#define CONTENTSFILE1_AUTO_SIZE		(78504)
#define CONTENTSFILE2_AUTO_NAME		"FLYINGBLOCK/ICON1.PAM"
#define CONTENTSFILE2_AUTO_SIZE		(2631680)
#define CONTENTSFILE3_AUTO_NAME		"FLYINGBLOCK/PIC1.PNG"
#define CONTENTSFILE3_AUTO_SIZE		(715557)

/* Structure of file parameter*/
typedef struct {
	unsigned int fileSize;
	char fileName[CELL_SAVEDATA_FILENAME_SIZE+16]; /* 16 : +local directory path */
	char padding[3];
} fileParam_st;

/* Structure of savedata parameter */
typedef struct {
	char dirName[CELL_SAVEDATA_DIRNAME_SIZE];
	char subTitle[CELL_SAVEDATA_SYSP_TITLE_SIZE];
	char detail[CELL_SAVEDATA_SYSP_DETAIL_SIZE];
	char listParam[CELL_SAVEDATA_SYSP_LPARAM_SIZE];
	fileParam_st *fileList;
} dataParam_st;

/* Parameters for PARAM.SFO */
#define SAMPLE_PARAMSFO_TITLE		"SYSUTIL SAVEDATA SAMPLE"
#define SAMPLE_PARAMSFO_DETAIL		"This data was saved by sysutil savedata sample."
#define PADDING		"AAA"

/* Index of files this sample save */
enum {
	FILE_INDEX_MUSTEXIST = 0,
	FILE_INDEX_OPTION,
	FILE_INDEX_ICON0,
	FILE_INDEX_ICON1,
	FILE_INDEX_PIC1,
	FILE_INDEX_END
};

/* The step of saving operation for progress counter in file operation callback */
enum {
	SAVE_OPERATION_STEP_ICON0 = 0,
	SAVE_OPERATION_STEP_ICON1,
	SAVE_OPERATION_STEP_PIC1,
	SAVE_OPERATION_STEP_MUSTEXISTFILE,
	SAVE_OPERATION_STEP_OPTIONFILE,
	SAVE_OPERATION_STEP_END
};

/* The step of loading operation for progress counter in file operation callback */
enum {
	LOAD_OPERATION_STEP_MUSTEXISTFILE = 0,
	LOAD_OPERATION_STEP_OPTIONFILE,
	LOAD_OPERATION_STEP_END
};

/**********************************************************************************/
/* sample definition and constants for list save/load */

/* Maximum number of savedata listed up in data list callback */
#define MAX_LISTSAVEDATA_NUM	(5)
/* The title of the new save data displayed right side of thumbnail */
#define NEWDATA_NAME_LIST		"Create New Game Data"
/* The thumbnail of the new save data */
#define NEWDATA_THUMBNAIL_FILE_LIST	DATA_DIR "/" CONTENTSFILE1_LIST_NAME
/* Save datas with this prefix are listed up in data list callback */
#define LISTSAVEDATA_PREFIX	"ABCD12345-LIST-"

/* The list of files included in save data */
static fileParam_st sampleFileListList[] = {
	{ MUSTEXISTFILE_SIZE, MUSTEXISTFILE_NAME, PADDING },
	{ OPTIONFILE_SIZE, OPTIONFILE_NAME, PADDING },
	{ CONTENTSFILE1_LIST_SIZE, CONTENTSFILE1_LIST_NAME, PADDING },
	{ CONTENTSFILE2_LIST_SIZE, CONTENTSFILE2_LIST_NAME, PADDING },
	{ CONTENTSFILE3_LIST_SIZE, CONTENTSFILE3_LIST_NAME, PADDING }
	};
/* Parameters for PARAM.SFO */
static dataParam_st sampleListData[] = {
	{ LISTSAVEDATA_PREFIX "A", "Castle",  "En route to the castle to meet the King's summons.\n" SAMPLE_PARAMSFO_DETAIL, "FIELD", sampleFileListList },
	{ LISTSAVEDATA_PREFIX "B", "Town",    "Weapon shopping in town.\n" SAMPLE_PARAMSFO_DETAIL, "FIELD", sampleFileListList },
	{ LISTSAVEDATA_PREFIX "C", "Village", "Hanging around town.\n" SAMPLE_PARAMSFO_DETAIL, "FIELD", sampleFileListList },
	{ LISTSAVEDATA_PREFIX "D", "Cave",    "Battling monsters in the cave.\n" SAMPLE_PARAMSFO_DETAIL, "DUNGEON", sampleFileListList },
	{ LISTSAVEDATA_PREFIX "E", "Tower",   "Looking out for enemies from the Tower.\n" SAMPLE_PARAMSFO_DETAIL, "DUNGEON", sampleFileListList }
	};

/**********************************************************************************/
/* sample definition and constants for fixed save/load */

/* Maximum number of savedata listed up in fixed data callback */
#define MAX_FIXEDSAVEDATA_NUM	(1)
/* The title of the new save data displayed right side of thumbnail */
#define NEWDATA_NAME_FIXED		"Create New Settings Data"
/* The thumbnail of the new save data */
#define NEWDATA_THUMBNAIL_FILE_FIXED	DATA_DIR "/" CONTENTSFILE1_FIXED_NAME
/* Save datas with this prefix are listed up in fixed data callback */
#define FIXEDSAVEDATA_DIRNAME	"ABCD12345-FIXED-"

/* The list of files included in save data */
static fileParam_st sampleFileListFixed[] = {
	{ MUSTEXISTFILE_SIZE, MUSTEXISTFILE_NAME, PADDING },
	{ OPTIONFILE_SIZE, OPTIONFILE_NAME, PADDING },
	{ CONTENTSFILE1_FIXED_SIZE, CONTENTSFILE1_FIXED_NAME, PADDING },
	{ CONTENTSFILE2_FIXED_SIZE, CONTENTSFILE2_FIXED_NAME, PADDING },
	{ CONTENTSFILE3_FIXED_SIZE, CONTENTSFILE3_FIXED_NAME, PADDING }
	};
/* Parameters for PARAM.SFO */
static dataParam_st sampleFixedData = {
	FIXEDSAVEDATA_DIRNAME,	"Settings", "Your settings and edit data.\n" SAMPLE_PARAMSFO_DETAIL, "SYSTEM", sampleFileListFixed
};

/**********************************************************************************/
/* sample definition and constants for auto save/load */

/* Maximum number of savedata listed up in fixed data callback */
#define MAX_LISTAUTOSAVEDATA_NUM	(3)
/* Save datas with this prefix are listed up in data fixed data callback */
#define AUTOSAVEDATA_DIRNAME	"ABCD00002-AUTO-"

/* The list of files included in save data */
static fileParam_st sampleFileListAuto[] = {
	{ MUSTEXISTFILE_SIZE, MUSTEXISTFILE_NAME, PADDING },
	{ OPTIONFILE_SIZE, OPTIONFILE_NAME, PADDING },
	{ CONTENTSFILE1_AUTO_SIZE, CONTENTSFILE1_AUTO_NAME, PADDING },
	{ CONTENTSFILE2_AUTO_SIZE, CONTENTSFILE2_AUTO_NAME, PADDING },
	{ CONTENTSFILE3_AUTO_SIZE, CONTENTSFILE3_AUTO_NAME, PADDING }
	};
/* Parameters for PARAM.SFO */
static dataParam_st sampleAutoData = {
	AUTOSAVEDATA_DIRNAME,	"History",	SAMPLE_PARAMSFO_DETAIL, "CONFIG", sampleFileListAuto
};

/* Settings for indicator */
#define INDICATOR_MESSAGE_SAVING							"Saving system data..."
#define INDICATOR_MESSAGE_LOADING							"Loading system data..."
static const char INDICATOR_SAVING_IMAGE_FILE_PATH[]	= 	DATA_DIR "/MISC/NOW-SAVING.PNG";
static const char INDICATOR_LOADING_IMAGE_FILE_PATH[]	=	DATA_DIR "/MISC/NOW-LOADING.PNG";

/**********************************************************************************/
/* Dump CellSaveDataListGet structure received in the fixed data callback and data list callback */
void dumpCellSaveDataListGet( CellSaveDataListGet *get )
{
	PRINTF("Dump CellSaveDataListGet in the CellSaveDataListCallback or CellSaveDataListCallback--------------------\n" );
	PRINTF("\tget->dirNum : %d\n", get->dirNum );
	PRINTF("\tget->dirListNum: %d\n", get->dirListNum );
	/* Dump every directory parameter of received directory list */
	for( unsigned int i = 0; i < get->dirListNum; i++ ) {
		PRINTF("\t%4d  DIRNAME: %s\t\tPARAM: %s\n", i, get->dirList[i].dirName, get->dirList[i].listParam );
	}
}

/* Dump CellSaveDataStatGet structure received in the data status callback */
void dumpCellSaveDataStatGet( CellSaveDataStatGet *get )
{
	PRINTF("Dump CellSaveDataStatGet in CellSaveDataStatCallback--------------------\n" );
	PRINTF("\tget->dir.dirName : %s\n", get->dir.dirName );
	PRINTF("\tget->isNewData: %d\n", get->isNewData );
	//PRINTF("\tget->hddFreeSizeKB 0x%x\n", get->hddFreeSizeKB);
	PRINTF("\tget->sizeKB : 0x%x MB\n", get->sizeKB / 1024);
	//PRINTF("\tget->sysSizeKB : 0x%x\n", get->sysSizeKB);
	//PRINTF("\tget->bind : %d\n", get->bind);
	PRINTF("\tget->dir : dirName : %s\n", get->dir.dirName);
	PRINTF("\tget->fileListNum: %d\n", get->fileListNum );
	/* Dump every file parameter of received file list */
	for( unsigned int i = 0; i < get->fileListNum; i++ ) {
		PRINTF("\t%3d  FILENAME: %s   type : %d  size : %lld\n", i,
			get->fileList[i].fileName,
			get->fileList[i].fileType,
			get->fileList[i].st_size );
	}
	PRINTF("\tget->fileNum: %d\n", get->fileNum );
	PRINTF("\n" );
	PRINTF("\tPARAM.SFO:TITLE: %s\n", get->getParam.title );
	PRINTF("\tPARAM.SFO:SUB_TITLE: %s\n", get->getParam.subTitle );
	PRINTF("\tPARAM.SFO:DETAIL: %s\n", get->getParam.detail );
	PRINTF("\tPARAM.SFO:ATTRIBUTE: %d\n", get->getParam.attribute );
	PRINTF("\tPARAM.SFO:LIST_PARAM: %s\n", get->getParam.listParam );
	PRINTF("\n" );
}

/* Prepare the indicator displayed in cellSaveDataXXXXAutoSave/Load() */
int prepareIndicator( int mode, CellSaveDataAutoIndicator **_indicator )
{
	static CellSaveDataAutoIndicator indicator = {
		dispPosition : 0,
		dispMode : 0,
		dispMsg : NULL,
		picBufSize : 0,
		picBuf : NULL,
		reserved : NULL
	};
	static char dispMsg[CELL_SAVEDATA_INDICATORMSG_MAX] = "";
	const char *picPath = NULL;

	if( mode != MODE_AUTO_SAVE &&
		mode != MODE_AUTO_LOAD &&
		mode != MODE_LIST_AUTO_SAVE &&
		mode != MODE_LIST_AUTO_LOAD )
	{
		*_indicator = NULL;
		return 0;
	}
	else {
		*_indicator = &indicator;
	}

	if( _file_buffer ) {
		free(_file_buffer);
		_file_buffer = NULL;
	}

	/* Setting for position of indicator */
	indicator.dispPosition = CELL_SAVEDATA_INDICATORPOS_LOWER_RIGHT|CELL_SAVEDATA_INDICATORPOS_MSGALIGN_RIGHT;
	/* Setting for blink of indicator */
	indicator.dispMode = CELL_SAVEDATA_INDICATORMODE_BLINK;

	if( mode == MODE_AUTO_SAVE || mode == MODE_LIST_AUTO_SAVE ) {
		/* Setting for resource of saving operation */
		strcpyUtf8( L10N_CODEPAGE_932, (uint8_t*)dispMsg, (uint8_t*)INDICATOR_MESSAGE_SAVING, CELL_SAVEDATA_INDICATORMSG_MAX );
		picPath = INDICATOR_SAVING_IMAGE_FILE_PATH;
	}
	else {
		/* Setting for resource of loading operation */
		strcpyUtf8( L10N_CODEPAGE_932, (uint8_t*)dispMsg, (uint8_t*)INDICATOR_MESSAGE_LOADING, CELL_SAVEDATA_INDICATORMSG_MAX );
		picPath = INDICATOR_LOADING_IMAGE_FILE_PATH;
	}

	/* The case of using original resources */
	fileAllocLoad( picPath, &_file_buffer, &indicator.picBufSize );
	indicator.picBuf = _file_buffer;
	dispMsg[0] = 0;

	/* The case of using default resources of system */
	/*
	indicator.picBuf = NULL;
	indicator.picBufSize = 0;
	*/

	indicator.dispMsg = dispMsg;


	/* The reserved member must be zero */
	indicator.reserved = NULL;

	return 0;
}

/* Load binary from file to memory */
int fileAllocLoad(const char *filePath, void **buf, unsigned int *size)
{
	int ret;
	int fd;
	CellFsStat status;
	uint64_t readlen;

	*buf = NULL;
	*size = 0;

	ret = cellFsOpen(filePath, CELL_FS_O_RDONLY, &fd, NULL, 0);
	if(ret != CELL_FS_SUCCEEDED){
		ERR_PRINTF("file %s open error : 0x%x\n", filePath, ret);
		return -1;
	}

	ret = cellFsFstat(fd, &status);
	if(ret != CELL_FS_SUCCEEDED){
		ERR_PRINTF("file %s get stat error : 0x%x\n", filePath, ret);
		cellFsClose(fd);
		return -1;
	}

	*buf = malloc( (size_t)status.st_size );
	if( *buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		cellFsClose(fd);
		return -1;
	}

	ret = cellFsRead(fd, *buf, status.st_size, &readlen);
	if(ret != CELL_FS_SUCCEEDED || status.st_size != readlen ) {
		ERR_PRINTF("file %s read error : 0x%x\n", filePath, ret);
		cellFsClose(fd);
		free(*buf);
		*buf = NULL;
		return -1;
	}

	ret = cellFsClose(fd);
	if(ret != CELL_FS_SUCCEEDED){
		ERR_PRINTF("file %s close error : 0x%x\n", filePath, ret);
		free(*buf);
		*buf = NULL;
		return -1;
	}

	*size = status.st_size;

	return 0;
}

/* Save binary from memory to file */
int fileSimpleSave(const char *filePath, void *buf, unsigned int fileSize)
{
	int ret;
	int fd;
	uint64_t writelen;

	if( buf == NULL ) {
		ERR_PRINTF("buffer is null\n");
	}

	ret = cellFsOpen(filePath, CELL_FS_O_WRONLY|CELL_FS_O_CREAT|CELL_FS_O_TRUNC, &fd, NULL, 0);
	if(ret != CELL_FS_SUCCEEDED){
		ERR_PRINTF("file %s open error : 0x%x\n", filePath, ret);
		return -1;
	}

	ret = cellFsWrite(fd, buf, fileSize, &writelen);
	if(ret != CELL_FS_SUCCEEDED || fileSize != writelen ) {
		ERR_PRINTF("file %s read error : 0x%x\n", filePath, ret);
		cellFsClose(fd);
		return -1;
	}

	ret = cellFsClose(fd);
	if(ret != CELL_FS_SUCCEEDED){
		ERR_PRINTF("file %s close error : 0x%x\n", filePath, ret);
		return -1;
	}

	return 0;
}


/**********************************************************************************/
int list_save( void )
{
	int ret;
	sys_ppu_thread_t tid;

	/* To prevent interrupting the processing of the main thread, save data utility function must be called using a sub thread */
	ret = sys_ppu_thread_create(&tid,
		thr_list_save, NULL,
		SAVEDATA_CMD_PRIO, SAVEDATA_CMD_STACKSIZE,
		0, SAVEDATA_THREAD_NAME_LIST_SAVE);
	if(ret<0) {
		ERR_PRINTF("list save thread create failed %d\n", ret);
		return -1;
	}

	return 0;
}

int list_load( void )
{
	int ret;
	sys_ppu_thread_t tid;

	/* To prevent interrupting the processing of the main thread, save data utility function must be called using a sub thread */
	ret = sys_ppu_thread_create(&tid,
		thr_list_load, NULL,
		SAVEDATA_CMD_PRIO, SAVEDATA_CMD_STACKSIZE,
		0, SAVEDATA_THREAD_NAME_LIST_LOAD);


	if (ret != 0) {
		ERR_PRINTF("list load thread create failed %d\n", ret);
		return -1;
	}
	
	return 0;
}

int fixed_save( void )
{
	int ret;
	sys_ppu_thread_t tid;

	/* To prevent interrupting the processing of the main thread, save data utility function must be called using a sub thread */
	ret = sys_ppu_thread_create(&tid,
		thr_fixed_save, NULL,
		SAVEDATA_CMD_PRIO, SAVEDATA_CMD_STACKSIZE,
		0, SAVEDATA_THREAD_NAME_FIXED_SAVE);


	if (ret != 0) {
		ERR_PRINTF("fixed save thread create failed %d\n", ret);
		return -1;
	}
	
	return 0;
}

int fixed_load( void )
{
	int ret;
	sys_ppu_thread_t tid;

	/* To prevent interrupting the processing of the main thread, save data utility function must be called using a sub thread */
	ret = sys_ppu_thread_create(&tid,
		thr_fixed_load, NULL,
		SAVEDATA_CMD_PRIO, SAVEDATA_CMD_STACKSIZE,
		0, SAVEDATA_THREAD_NAME_FIXED_LOAD);


	if (ret != 0) {
		ERR_PRINTF("fixed save thread create failed %d\n", ret);
		return -1;
	}
	
	return 0;
}

int auto_save( void )
{
	int ret;
	sys_ppu_thread_t tid;

	/* To prevent interrupting the processing of the main thread, save data utility function must be called using a sub thread */
	ret = sys_ppu_thread_create(&tid,
		thr_auto_save, NULL,
		SAVEDATA_CMD_PRIO, SAVEDATA_CMD_STACKSIZE,
		0, SAVEDATA_THREAD_NAME_AUTO_SAVE);


	if (ret != 0) {
		ERR_PRINTF("auto save thread create failed %d\n", ret);
		return -1;
	}
	
	return 0;
}

int auto_load( void )
{
	int ret;
	sys_ppu_thread_t tid;

	/* To prevent interrupting the processing of the main thread, save data utility function must be called using a sub thread */
	ret = sys_ppu_thread_create(&tid,
		thr_auto_load, NULL,
		SAVEDATA_CMD_PRIO, SAVEDATA_CMD_STACKSIZE,
		0, SAVEDATA_THREAD_NAME_AUTO_LOAD);


	if (ret != 0) {
		ERR_PRINTF("auto load thread create failed %d\n", ret);
		return -1;
	}

	return 0;
}

int list_auto_save( void )
{
	int ret;
	sys_ppu_thread_t tid;

	/* To prevent interrupting the processing of the main thread, save data utility function must be called using a sub thread */
	ret = sys_ppu_thread_create(&tid,
		thr_list_auto_save, NULL,
		SAVEDATA_CMD_PRIO, SAVEDATA_CMD_STACKSIZE,
		0, SAVEDATA_THREAD_NAME_LIST_AUTO_SAVE);


	if (ret != 0) {
		ERR_PRINTF("list auto save thread create failed %d\n", ret);
		return -1;
	}

	return 0;
}

int list_auto_load( void )
{
	int ret;
	sys_ppu_thread_t tid;

	/* To prevent interrupting the processing of the main thread, save data utility function must be called using a sub thread */
	ret = sys_ppu_thread_create(&tid,
		thr_list_auto_load, NULL,
		SAVEDATA_CMD_PRIO, SAVEDATA_CMD_STACKSIZE,
		0, SAVEDATA_THREAD_NAME_LIST_AUTO_LOAD);


	if (ret != 0) {
		ERR_PRINTF("list auto load thread create failed %d\n", ret);
		return -1;
	}

	return 0;
}

void thr_list_save(uint64_t arg)
{
	int ret = 0;
	char dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE];
	CellSaveDataSetList setList;
	CellSaveDataSetBuf setBuf;
	(void)arg;

	PRINTF( "thr_list_save() start\n");

	/* The save datas with the prefix specified here, */
	/* will be listed up in the data list callback.   */
	strncpy( dirNamePrefix, LISTSAVEDATA_PREFIX, CELL_SAVEDATA_PREFIX_SIZE );
	dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE-1] = 0;

	/* Settings for obtaining save data list */
	setList.sortType      = CELL_SAVEDATA_SORTTYPE_SUBTITLE;
	setList.sortOrder     = CELL_SAVEDATA_SORTORDER_ASCENT;
	setList.dirNamePrefix = dirNamePrefix;
	setList.reserved = NULL;	/* The reserved member must be zero */

	/* Settings for general buffer */
	setBuf.dirListMax  = MAX_LISTSAVEDATA_NUM;					/* Maximum number of directories listed up in the data list callback */
	setBuf.fileListMax = FILE_INDEX_END;						/* Maximum number of files listed up in the data status callback */
	memset( setBuf.reserved, 0x0, sizeof(setBuf.reserved) );	/* The reserved member must be zero-filled */

	/* Size of buffer must be such that it can store data of the size specified by the above maximums */
	setBuf.bufSize = MAX( MAX_LISTSAVEDATA_NUM * sizeof(CellSaveDataDirList), FILE_INDEX_END * sizeof(CellSaveDataFileStat) );
	setBuf.buf = malloc(setBuf.bufSize);
	if( setBuf.buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		goto end;
	}

	DUMP_SYSMEMORY;

	/* Execute save from a list */
	ret = cellSaveDataListSave2(
					CELL_SAVEDATA_VERSION_CURRENT,	/* Version of save data format */
					&setList,						/* Settings for obtaining save data list */
					&setBuf,						/* Settings of the general buffer */
					cb_data_list_save,				/* Data list callback function */
					cb_data_status_save,			/* Data status callback function */
					cb_file_operation_save,			/* File operation callback function */
					SYS_MEMORY_CONTAINER_ID_INVALID,/* Memory container ID */
					(void*)MODE_LIST_SAVE			/* Application-defined data (This sample specify userdata with mode of operation) */
				);
 
	last_result = ret;

	PRINTF("cellSaveDataListSave2() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

end:
	if( setBuf.buf ) {
		free( setBuf.buf );
	}
	if( _file_buffer ) {
		free( _file_buffer );
		_file_buffer = NULL;
	}

	PRINTF( "thr_list_save() end\n");
	
	is_running = FALSE;

	sys_ppu_thread_exit(0);
}


void thr_list_load(uint64_t arg)
{
	int ret = 0;
	char dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE];
	CellSaveDataSetList setList;
	CellSaveDataSetBuf setBuf;
	(void)arg;

	PRINTF( "thr_list_load() start\n");

	/* The save datas with the prefix specified here, */
	/* will be listed up in the data list callback.   */
	strncpy( dirNamePrefix, LISTSAVEDATA_PREFIX, CELL_SAVEDATA_PREFIX_SIZE );
	dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE-1] = 0;

	/* Settings for obtaining save data list */
	setList.sortType      = CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME;
	setList.sortOrder     = CELL_SAVEDATA_SORTORDER_DESCENT;
	setList.dirNamePrefix = dirNamePrefix;
	setList.reserved = NULL;	/* The reserved member must be zero */

	/* Settings for general buffer */
	setBuf.dirListMax  = MAX_LISTSAVEDATA_NUM;					/* Maximum number of directories listed up in the data list callback */
	setBuf.fileListMax = FILE_INDEX_END;						/* Maximum number of files listed up in the data status callback */
	memset( setBuf.reserved, 0x0, sizeof(setBuf.reserved) );	/* The reserved member must be zero-filled */
	/* Size of buffer must be such that it can store data of the size specified by the above maximums */
	setBuf.bufSize = MAX( MAX_LISTSAVEDATA_NUM * sizeof(CellSaveDataDirList), FILE_INDEX_END * sizeof(CellSaveDataFileStat) );
	setBuf.buf = malloc(setBuf.bufSize);
	if( setBuf.buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		goto end;
	}

	DUMP_SYSMEMORY;

	/* Execute load from a list */
	ret = cellSaveDataListLoad2(
					CELL_SAVEDATA_VERSION_CURRENT,	/* Version of save data format */
					&setList,						/* Settings for obtaining save data list */
					&setBuf,						/* Settings of the general buffer */
					cb_data_list_load,				/* Data list callback function */
					cb_data_status_load,			/* Data status callback function */
					cb_file_operation_load,			/* File operation callback function */
					SYS_MEMORY_CONTAINER_ID_INVALID,/* Memory container ID */
					(void*)MODE_LIST_LOAD			/* Application-defined data (This sample specify userdata with mode of operation) */
				);

	last_result = ret;

	PRINTF("cellSaveDataListLoad2() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

end:
	if( setBuf.buf ) {
		free( setBuf.buf );
	}
	if( _file_buffer ) {
		free( _file_buffer );
		_file_buffer = NULL;
	}

	PRINTF( "thr_list_load() end\n");
	
	is_running = FALSE;

	sys_ppu_thread_exit(0);
}

void thr_fixed_save(uint64_t arg)
{
	int ret = 0;
	char dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE];
	CellSaveDataSetList setList;
	CellSaveDataSetBuf setBuf;
	(void)arg;

	PRINTF( "thr_fixed_save() start\n");

	/* The save datas with the prefix specified here, */
	/* will be listed up in the fixed data callback.   */
	strncpy( dirNamePrefix, FIXEDSAVEDATA_DIRNAME, CELL_SAVEDATA_PREFIX_SIZE );
	dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE-1] = 0;

	/* Settings for obtaining save data list */
	setList.sortType      = CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME;
	setList.sortOrder     = CELL_SAVEDATA_SORTORDER_DESCENT;
	setList.dirNamePrefix = dirNamePrefix;
	setList.reserved = NULL;	/* The reserved member must be zero */

	/* Settings for general buffer */
	setBuf.dirListMax  = MAX_FIXEDSAVEDATA_NUM;					/* Maximum number of directories listed up in the fixed data callback */
	setBuf.fileListMax = FILE_INDEX_END;						/* Maximum number of files listed up in the data status callback */
	memset( setBuf.reserved, 0x0, sizeof(setBuf.reserved) );	/* The reserved member must be zero-filled */
	/* Size of buffer must be such that it can store data of the size specified by the above maximums */
	setBuf.bufSize = MAX( MAX_FIXEDSAVEDATA_NUM * sizeof(CellSaveDataDirList), FILE_INDEX_END * sizeof(CellSaveDataFileStat) );
	setBuf.buf = malloc(setBuf.bufSize);
	if( setBuf.buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		goto end;
	}

	DUMP_SYSMEMORY;

	/* Execute fixed save */
	ret = cellSaveDataFixedSave2(
					CELL_SAVEDATA_VERSION_CURRENT,	/* Version of save data format */
					&setList,						/* Settings for obtaining save data list */
					&setBuf,						/* Settings of the general buffer */
					cb_data_fixed_save,				/* Fixed data callback function */
					cb_data_status_save,			/* Data status callback function */
					cb_file_operation_save,			/* File operation callback function */
					SYS_MEMORY_CONTAINER_ID_INVALID,/* Memory container ID */
					(void*)MODE_FIXED_SAVE			/* Application-defined data (This sample specify userdata with mode of operation) */
				);

	last_result = ret;

	PRINTF("cellSaveDataFixedSave2() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

end:
	if( setBuf.buf ) {
		free( setBuf.buf );
	}
	if( _file_buffer ) {
		free( _file_buffer );
		_file_buffer = NULL;
	}

	PRINTF( "thr_fixed_save() end\n");
	
	is_running = FALSE;

	sys_ppu_thread_exit(0);
}

void thr_fixed_load(uint64_t arg)
{
	int ret = 0;
	char dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE];
	CellSaveDataSetList setList;
	CellSaveDataSetBuf setBuf;
	(void)arg;

	PRINTF( "thr_fixed_load() start\n");

	/* The save datas with the prefix specified here, */
	/* will be listed up in the fixed data callback.   */
	strncpy( dirNamePrefix, FIXEDSAVEDATA_DIRNAME, CELL_SAVEDATA_PREFIX_SIZE );
	dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE-1] = 0;

	/* Settings for obtaining save data list */
	setList.sortType      = CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME;
	setList.sortOrder     = CELL_SAVEDATA_SORTORDER_DESCENT;
	setList.dirNamePrefix = dirNamePrefix;
	setList.reserved = NULL;	/* The reserved member must be zero */

	/* Settings for general buffer */
	setBuf.dirListMax  = MAX_FIXEDSAVEDATA_NUM;					/* Maximum number of directories listed up in the fixed data callback */
	setBuf.fileListMax = FILE_INDEX_END;						/* Maximum number of files listed up in the data status callback */
	memset( setBuf.reserved, 0x0, sizeof(setBuf.reserved) );	/* The reserved member must be zero-filled */
	/* Size of buffer must be such that it can store data of the size specified by the above maximums */
	setBuf.bufSize = MAX( MAX_FIXEDSAVEDATA_NUM * sizeof(CellSaveDataDirList), FILE_INDEX_END * sizeof(CellSaveDataFileStat) );
	setBuf.buf = malloc(setBuf.bufSize);
	if( setBuf.buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		goto end;
	}

	DUMP_SYSMEMORY;

	/* Execute fixed load */
	ret = cellSaveDataFixedLoad2(
					CELL_SAVEDATA_VERSION_CURRENT,	/* Version of save data format */
					&setList,						/* Settings for obtaining save data list */
					&setBuf,						/* Settings of the general buffer */
					cb_data_fixed_load,				/* Fixed data callback function */
					cb_data_status_load,			/* Data status callback function */
					cb_file_operation_load,			/* File operation callback function */
					SYS_MEMORY_CONTAINER_ID_INVALID,/* Memory container ID */
					(void*)MODE_FIXED_LOAD			/* Application-defined data (This sample specify userdata with mode of operation) */
				);

	last_result = ret;

	PRINTF("cellSaveDataFixedLoad2() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

end:
	if( setBuf.buf ) {
		free( setBuf.buf );
	}
	if( _file_buffer ) {
		free( _file_buffer );
		_file_buffer = NULL;
	}

	PRINTF( "thr_fixed_load() end\n");
	
	is_running = FALSE;

	sys_ppu_thread_exit(0);
}

void thr_auto_save(uint64_t arg)
{
	int ret = 0;
	char dirName[CELL_SAVEDATA_DIRNAME_SIZE];
	CellSaveDataSetBuf setBuf;
	(void)arg;

	PRINTF( "thr_auto_save() start\n");

	/* The directory name of the target save data */
	strncpy( dirName, AUTOSAVEDATA_DIRNAME, CELL_SAVEDATA_DIRNAME_SIZE );
	dirName[CELL_SAVEDATA_DIRNAME_SIZE-1] = 0;

	/* Settings for general buffer */
	setBuf.dirListMax  = 0;										/* This member is not used (specify zero) */
	setBuf.fileListMax = FILE_INDEX_END;						/* Maximum number of files listed up in the data status callback */
	memset( setBuf.reserved, 0x0, sizeof(setBuf.reserved) );	/* The reserved member must be zero-filled */
	/* Size of buffer must be such that it can store data of the size specified by the above maximums */
	setBuf.bufSize = FILE_INDEX_END * sizeof(CellSaveDataFileStat);
	setBuf.buf = malloc(setBuf.bufSize);
	if( setBuf.buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		goto end;
	}

	DUMP_SYSMEMORY;

	/* Execute auto save */
	ret = cellSaveDataAutoSave2(
					CELL_SAVEDATA_VERSION_CURRENT,	/* Version of save data format */
					dirName,						/* Directory name for save data */
					CELL_SAVEDATA_ERRDIALOG_ALWAYS, /* Settings for outputting error dialog */
					&setBuf,						/* Settings of the general buffer */
					cb_data_status_save,			/* Data status callback function */
					cb_file_operation_save,			/* File operation callback function */
					SYS_MEMORY_CONTAINER_ID_INVALID,/* Memory container ID */
					(void*)MODE_AUTO_SAVE			/* Application-defined data (This sample specify userdata with mode of operation) */
				);

	last_result = ret;

	PRINTF("cellSaveDataAutoSave2() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

end:
	if( setBuf.buf ) {
		free( setBuf.buf );
	}
	if( _file_buffer ) {
		free( _file_buffer );
		_file_buffer = NULL;
	}

	PRINTF( "thr_auto_save() end\n");

	is_running = FALSE;

	sys_ppu_thread_exit(0);
}

void thr_auto_load(uint64_t arg)
{
	int ret = 0;
	char dirName[CELL_SAVEDATA_DIRNAME_SIZE];
	CellSaveDataSetBuf setBuf;
	(void)arg;

	PRINTF( "thr_auto_load() start\n");

	/* The directory name of the target save data */
	strncpy( dirName, AUTOSAVEDATA_DIRNAME, CELL_SAVEDATA_DIRNAME_SIZE );
	dirName[CELL_SAVEDATA_DIRNAME_SIZE-1] = 0;
	
	/* Settings for general buffer */
	setBuf.dirListMax  = 0;										/* This member is not used (specify zero) */
	setBuf.fileListMax = FILE_INDEX_END;						/* Maximum number of files listed up in the data status callback */
	memset( setBuf.reserved, 0x0, sizeof(setBuf.reserved) );	/* The reserved member must be zero-filled */
	/* Size of buffer must be such that it can store data of the size specified by the above maximums */
	setBuf.bufSize = FILE_INDEX_END * sizeof(CellSaveDataFileStat);
	setBuf.buf = malloc(setBuf.bufSize);
	if( setBuf.buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		goto end;
	}

	DUMP_SYSMEMORY;

	/* Execute auto load */
	ret = cellSaveDataAutoLoad2(
					CELL_SAVEDATA_VERSION_CURRENT,	/* Version of save data format */
					dirName,						/* Directory name for save data */
					CELL_SAVEDATA_ERRDIALOG_ALWAYS, /* Settings for outputting error dialog */
					&setBuf,						/* Settings of the general buffer */
					cb_data_status_load,			/* Data status callback function */
					cb_file_operation_load,			/* File operation callback function */
					SYS_MEMORY_CONTAINER_ID_INVALID,/* Memory container ID */
					(void*)MODE_AUTO_LOAD			/* Application-defined data (This sample specify userdata with mode of operation) */
				);

	last_result = ret;

	PRINTF("cellSaveDataAutoLoad2() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

end:
	if( setBuf.buf ) {
		free( setBuf.buf );
	}
	if( _file_buffer ) {
		free( _file_buffer );
		_file_buffer = NULL;
	}

	PRINTF( "thr_auto_load() end\n");
	
	is_running = FALSE;

	sys_ppu_thread_exit(0);
}

void thr_list_auto_save(uint64_t arg)
{
	int ret = 0;
	char dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE];
	CellSaveDataSetList setList;
	CellSaveDataSetBuf setBuf;
	(void)arg;

	PRINTF( "thr_list_auto_save() start\n");

	/* The save datas with the prefix specified here, */
	/* will be listed up in the fixed data callback.   */
	strncpy( dirNamePrefix, AUTOSAVEDATA_DIRNAME, CELL_SAVEDATA_PREFIX_SIZE );
	dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE-1] = 0;

	/* Settings for obtaining save data list */
	setList.sortType      = CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME;
	setList.sortOrder     = CELL_SAVEDATA_SORTORDER_DESCENT;
	setList.dirNamePrefix = dirNamePrefix;
	setList.reserved = NULL;	/* The reserved member must be zero */

	/* Settings for general buffer */
	setBuf.dirListMax  = MAX_LISTAUTOSAVEDATA_NUM;				/* Maximum number of directories listed up in the fixed data callback */
	setBuf.fileListMax = FILE_INDEX_END;						/* Maximum number of files listed up in the data status callback */
	memset( setBuf.reserved, 0x0, sizeof(setBuf.reserved) );	/* The reserved member must be zero-filled */
	/* Size of buffer must be such that it can store data of the size specified by the above maximums */
	setBuf.bufSize = MAX( MAX_LISTAUTOSAVEDATA_NUM * sizeof(CellSaveDataDirList), FILE_INDEX_END * sizeof(CellSaveDataFileStat) );
	setBuf.buf = malloc(setBuf.bufSize);
	if( setBuf.buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		goto end;
	}

	DUMP_SYSMEMORY;

	/* Execute auto save from a list */
	ret = cellSaveDataListAutoSave(
					CELL_SAVEDATA_VERSION_CURRENT,	/* Version of save data format */
					CELL_SAVEDATA_ERRDIALOG_ALWAYS, /* Settings for outputting error dialog */
					&setList,						/* Settings for obtaining save data list */
					&setBuf,						/* Settings of the general buffer */
					cb_list_auto_save,				/* Fixed data callback function */
					cb_data_status_save,			/* Data status callback function */
					cb_file_operation_save,			/* File operation callback function */
					SYS_MEMORY_CONTAINER_ID_INVALID,/* Memory container ID */
					(void*)MODE_LIST_AUTO_SAVE		/* Application-defined data (This sample specify userdata with mode of operation) */
				);

	last_result = ret;

	PRINTF("cellSaveDataListAutoSave() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

end:
	if( setBuf.buf ) {
		free( setBuf.buf );
	}
	if( _file_buffer ) {
		free( _file_buffer );
		_file_buffer = NULL;
	}

	PRINTF( "thr_list_auto_save() end\n");
	
	is_running = FALSE;

	sys_ppu_thread_exit(0);
}

void thr_list_auto_load(uint64_t arg)
{
	int ret = 0;
	char dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE];
	CellSaveDataSetList setList;
	CellSaveDataSetBuf setBuf;
	(void)arg;

	PRINTF( "thr_list_auto_load() start\n");

	/* The save datas with the prefix specified here, */
	/* will be listed up in the fixed data callback.   */
	strncpy( dirNamePrefix, AUTOSAVEDATA_DIRNAME, CELL_SAVEDATA_PREFIX_SIZE );
	dirNamePrefix[CELL_SAVEDATA_PREFIX_SIZE-1] = 0;

	/* Settings for obtaining save data list */
	setList.sortType      = CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME;
	setList.sortOrder     = CELL_SAVEDATA_SORTORDER_DESCENT;
	setList.dirNamePrefix = dirNamePrefix;
	setList.reserved = NULL;	/* The reserved member must be zero */

	/* Settings for general buffer */
	setBuf.dirListMax  = MAX_LISTAUTOSAVEDATA_NUM;				/* Maximum number of directories listed up in the fixed data callback */
	setBuf.fileListMax = FILE_INDEX_END;						/* Maximum number of files listed up in the data status callback */
	memset( setBuf.reserved, 0x0, sizeof(setBuf.reserved) );	/* The reserved member must be zero-filled */
	/* Size of buffer must be such that it can store data of the size specified by the above maximums */
	setBuf.bufSize = MAX( MAX_LISTAUTOSAVEDATA_NUM * sizeof(CellSaveDataDirList), FILE_INDEX_END * sizeof(CellSaveDataFileStat) );
	setBuf.buf = malloc(setBuf.bufSize);
	if( setBuf.buf == NULL ) {
		ERR_PRINTF("alloc failed\n");
		goto end;
	}

	DUMP_SYSMEMORY;

	/* Execute auto save from a list */
	ret = cellSaveDataListAutoLoad(
					CELL_SAVEDATA_VERSION_CURRENT,	/* Version of save data format */
					CELL_SAVEDATA_ERRDIALOG_ALWAYS, /* Settings for outputting error dialog */
					&setList,						/* Settings for obtaining save data list */
					&setBuf,						/* Settings of the general buffer */
					cb_list_auto_load,				/* Fixed data callback function */
					cb_data_status_load,			/* Data status callback function */
					cb_file_operation_load,			/* File operation callback function */
					SYS_MEMORY_CONTAINER_ID_INVALID,/* Memory container ID */
					(void*)MODE_LIST_AUTO_LOAD		/* Application-defined data (This sample specify userdata with mode of operation) */
				);

	last_result = ret;

	PRINTF("cellSaveDataListAutoLoad() : 0x%x\n", ret);

	DUMP_SYSMEMORY;

end:
	if( setBuf.buf ) {
		free( setBuf.buf );
	}
	if( _file_buffer ) {
		free( _file_buffer );
		_file_buffer = NULL;
	}

	PRINTF( "thr_list_auto_load() end\n");
	
	is_running = FALSE;

	sys_ppu_thread_exit(0);
}

/* Data list callback function for cellSaveDataListSave2() */
void cb_data_list_save( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set )
{
	static char newDataTitle[] = NEWDATA_NAME_LIST;
	static CellSaveDataListNewData newData;
	static CellSaveDataNewDataIcon newDataIcon;
	unsigned int thumb_size = 0;

	memset( &newData, 0x0, sizeof(CellSaveDataListNewData) );
	memset( &newDataIcon, 0x0, sizeof(CellSaveDataNewDataIcon) );

	if( _file_buffer ) {
		free(_file_buffer);
		_file_buffer = NULL;
	}

	PRINTF( "cb_data_list_save() start\n");

	/* Dump obtained list */
	dumpCellSaveDataListGet(get);

	if( get->dirNum > get->dirListNum ) {
		/* This case can be handled in any way that is appropriate for the application's specifications */
		ERR_PRINTF( "found too many datas. expected[%d] < found[%d]\n", get->dirListNum, get->dirNum );
	}

	set->fixedList = get->dirList;			/* You can specify get->dirList to set->fixedList directly */
	if( get->dirListNum>CELL_SAVEDATA_LISTITEM_MAX ) {
		set->fixedListNum = CELL_SAVEDATA_LISTITEM_MAX;
	}
	else {
		set->fixedListNum = get->dirListNum;
	}
	set->focusPosition = CELL_SAVEDATA_FOCUSPOS_LISTHEAD;
	set->focusDirName = NULL;
	set->reserved = NULL;					/* The reserved member must be zero */

	/* Settings for new save data */
	if( get->dirListNum < MAX_LISTSAVEDATA_NUM ) {
		/* Specify the name that doesn't overlap with the existing data */
		unsigned int i, j;
		for( i = 0; i < MAX_LISTSAVEDATA_NUM; i++ ) {
			for( j = 0; j < get->dirListNum; j++ ) {
				if( strcmp( sampleListData[i].dirName, get->dirList[j].dirName ) == 0 ) {
					break;
				}
			}
			if( j == get->dirListNum ) {
				break;
			}
		}
		if( i < MAX_LISTSAVEDATA_NUM ) {
			/* If name of new save data is found */
			set->newData = &newData;			
			newData.dirName = sampleListData[i].dirName;		/* Directory name for new save data */
			newData.iconPosition = CELL_SAVEDATA_ICONPOS_HEAD;	/* Setting for position of new save data in list */
			newData.reserved = NULL;
			/* Settings for new save data icon */
			newData.icon = &newDataIcon;						/* Icon of new save data */
			newDataIcon.title = newDataTitle;					/* Title */
			fileAllocLoad( NEWDATA_THUMBNAIL_FILE_LIST, &_file_buffer, &thumb_size );
			newDataIcon.iconBuf = _file_buffer;					/* Buffer storing still-image icon */
			newDataIcon.iconBufSize = thumb_size;				/* Size of still-image icon */
			newDataIcon.reserved = NULL;						/* The reserved member must be zero */
		}
		else {
			/* If not, then hide new data icon */
			set->newData = NULL;
		}
	}
	else {
		/* When MAX_LISTSAVEDATA_NUM data or more exists, hide new data icon */
		set->newData = NULL;
	}

	/* Specify OK_NEXT to continue */
	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

#if 0
	/* This is the sample of showing an error dialog with original message */
	result->result = CELL_SAVEDATA_CBRESULT_ERR_INVALID;	/* Specify INVALID to display an error dialog */
	static uint8_t str[CELL_SAVEDATA_INVALIDMSG_MAX];
	/* for SJIS */
	strcpyUtf8( L10N_CODEPAGE_932, str, (uint8_t*)"This is sample code to display local error message.", CELL_SAVEDATA_INVALIDMSG_MAX );
	result->invalidMsg = (char*)str;
#endif

	return;
}

/* Data list callback function for cellSaveDataListLoad2() */
void cb_data_list_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set )
{
	PRINTF( "cb_data_list_load() start\n");

	/* Dump obtained list */
	dumpCellSaveDataListGet(get);

	if( get->dirNum > get->dirListNum ) {
		/* This case can be handled in any way that is appropriate for the application's specifications */
		ERR_PRINTF( "found too many datas. expected[%d] < found[%d]\n", get->dirListNum, get->dirNum );
	}

	set->fixedList = get->dirList;						/* You can specify get->dirList to set->fixedList directly */
	if( get->dirListNum>CELL_SAVEDATA_LISTITEM_MAX ) {
		set->fixedListNum = CELL_SAVEDATA_LISTITEM_MAX;
	}
	else {
		set->fixedListNum = get->dirListNum;
	}
	set->focusPosition = CELL_SAVEDATA_FOCUSPOS_LISTHEAD;
	set->focusDirName = NULL;
	set->reserved = NULL;					/* The reserved member must be zero */

	/* Specify zero if you don't want to display new data icon on a list */
	set->newData = NULL;

	/* Specify OK_NEXT to continue */
	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	return;
}

/* Fixed data callback function for cellSaveDataFixedSave2() */
void cb_data_fixed_save( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set )
{
	unsigned int i;
	static char newDataTitle[] = NEWDATA_NAME_FIXED;
	static CellSaveDataNewDataIcon newDataIcon;
	unsigned int thumb_size = 0;

	memset( &newDataIcon, 0x0, sizeof(CellSaveDataNewDataIcon) );

	if( _file_buffer ) {
		free(_file_buffer);
		_file_buffer = NULL;
	}

	PRINTF( "cb_data_fixed_save() start\n");

	/* Dump obtained list */
	dumpCellSaveDataListGet(get);

	set->dirName = sampleFixedData.dirName;
	set->option = CELL_SAVEDATA_OPTION_NONE;	/* You also specify CELL_SAVEDATA_OPTION_NOCONFIRM */

	if( get->dirNum > get->dirListNum ) {
		/* This case can be handled in any way that is appropriate for the application's specifications */
		ERR_PRINTF( "found too many datas. expected[%d] < found[%d]\n", get->dirListNum, get->dirNum );
	}

	/* Settings for new save data */
	i = 0;
	while( i < get->dirListNum ) {
		if( strcmp( get->dirList[i].dirName, FIXEDSAVEDATA_DIRNAME ) == 0 ) {
			break;
		}
		i++;
	}
	if( i < get->dirListNum ) {
		/* If there is the target save data, it's no need to specify new save data settings */
		set->newIcon = NULL;
	}
	else {
		set->newIcon = &newDataIcon;
		set->newIcon->title = newDataTitle;		/* Title */
		fileAllocLoad( NEWDATA_THUMBNAIL_FILE_FIXED, &_file_buffer, &thumb_size );
		set->newIcon->iconBuf = _file_buffer;	/* Buffer storing still-image icon */
		set->newIcon->iconBufSize = thumb_size;	/* Size of still-image icon */
		set->newIcon->reserved = NULL;			/* The reserved member must be zero */
	}

	/* Specify OK_NEXT to continue */
	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	return;
}

/* Fixed data callback function for cellSaveDataFixedLoad2() */
void cb_data_fixed_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set )
{
	unsigned int i;

	PRINTF( "cb_data_fixed_load() start\n");

	/* Dump obtained list */
	dumpCellSaveDataListGet(get);

	if( get->dirNum > get->dirListNum ) {
		/* This case can be handled in any way that is appropriate for the application's specifications */
		ERR_PRINTF( "found too many datas. expected[%d] < found[%d]\n", get->dirListNum, get->dirNum );
	}

	set->dirName = sampleFixedData.dirName;
	set->newIcon = NULL;
	set->option = CELL_SAVEDATA_OPTION_NONE;	/* You also specify CELL_SAVEDATA_OPTION_NOCONFIRM */

	i = 0;
	while( i < get->dirListNum ) {
		if( strcmp( get->dirList[i].dirName, FIXEDSAVEDATA_DIRNAME ) == 0 ) {
			break;
		}
		i++;
	}

	if( i < get->dirListNum ) {
		/* Specify OK_NEXT to continue */
		result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
	}
	else {
		/* Specify ERR_NODATA if the target save data to read cannot be found */
		result->result = CELL_SAVEDATA_CBRESULT_ERR_NODATA;
	}

	return;
}

/* Fixed data callback function for cellSaveDataListAutoSave() */
void cb_list_auto_save( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set )
{
	unsigned int i;

	PRINTF( "cb_list_auto_save() start\n");

	/* Dump obtained list */
	dumpCellSaveDataListGet(get);

	set->dirName = sampleAutoData.dirName;
	set->newIcon = NULL;
	set->option = CELL_SAVEDATA_OPTION_NONE;	/* You can only specify CELL_SAVEDATA_OPTION_NONE in this SDK version */

	if( get->dirNum > get->dirListNum ) {
		/* This case can be handled in any way that is appropriate for the application's specifications */
		ERR_PRINTF( "found too many datas. expected[%d] < found[%d]\n", get->dirListNum, get->dirNum );
	}

	i = 0;
	while( i < get->dirListNum ) {
		if( strcmp( get->dirList[i].dirName, sampleAutoData.dirName ) == 0 ) {
			break;
		}
		i++;
	}

	if( i < get->dirListNum ) {
		PRINTF( "found savedata for cellSaveDataListAutoSave() : %s\n", get->dirList[i].dirName );
	}
	else {
		PRINTF( "found no savedata for cellSaveDataListAutoSave()\n" );
	}

	/* Specify OK_NEXT to continue */
	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	return;
}

/* Fixed data callback function for cellSaveDataListAutoLoad() */
void cb_list_auto_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set )
{
	unsigned int i;

	PRINTF( "cb_list_auto_load() start\n");

	/* Dump obtained list */
	dumpCellSaveDataListGet(get);

	if( get->dirNum > get->dirListNum ) {
		/* This case can be handled in any way that is appropriate for the application's specifications */
		ERR_PRINTF( "found too many datas. expected[%d] < found[%d]\n", get->dirListNum, get->dirNum );
	}

	set->dirName = sampleAutoData.dirName;
	set->newIcon = NULL;
	set->option = CELL_SAVEDATA_OPTION_NONE;	/* You can only specify CELL_SAVEDATA_OPTION_NONE in this SDK version */

	i = 0;
	while( i < get->dirListNum ) {
		if( strcmp( get->dirList[i].dirName, sampleAutoData.dirName ) == 0 ) {
			break;
		}
		i++;
	}

	if( i < get->dirListNum ) {
		/* Specify OK_NEXT to continue */
		result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
	}
	else {
		/* If the target save data to read cannot be found */
		result->result = CELL_SAVEDATA_CBRESULT_ERR_NODATA;
	}

	return;
}

/* Data status callback for cellSaveDataXXXXSave() */
void cb_data_status_save( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	PRINTF( "cb_data_status_save() start\n");

	/* Dump obtained save data status */
	dumpCellSaveDataStatGet(get);

	dataParam_st *dataParam;

	/* Application-defined data (This sample specify userdata with mode of operation) */
	switch( (int)result->userdata ) {
	case MODE_LIST_SAVE:
	case MODE_LIST_LOAD:
		{
			int i;
			for( i = 0; i < MAX_LISTSAVEDATA_NUM; i++ ) {
				if( strcmp( sampleListData[i].dirName, get->dir.dirName ) == 0 ) {
					break;
				}
			}
			if( i < MAX_LISTSAVEDATA_NUM ) {
				dataParam = (void*)&sampleListData[i];
			}
			else {
				/* This won't actually happen */
				ERR_PRINTF(" list save error.\n" );
				dataParam = (void*)&sampleListData[0];
			}
		}
		break;
	case MODE_FIXED_SAVE:
	case MODE_FIXED_LOAD:
		{
			dataParam = (void*)&sampleFixedData;
		}
		break;
	case MODE_LIST_AUTO_SAVE:
	case MODE_LIST_AUTO_LOAD:
	case MODE_AUTO_SAVE:
	case MODE_AUTO_LOAD:
		{
			dataParam = (void*)&sampleAutoData;
		}
		break;
	default:
		ERR_PRINTF("unknown userdata value : %d\n", (int)result->userdata );
		result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
		return;
	}

	set->reCreateMode = CELL_SAVEDATA_RECREATE_NO;				/* Whether or not to first delete save data */
	set->setParam = &get->getParam; 							/* You can specify get->getParam to set->setParam directly */
	prepareIndicator( (int)result->userdata, &set->indicator );	/* Settings for indicator which appear during saving operation */

	if (!get->isNewData) {
		/* When target save data already exists */

		int CURRENT_SIZEKB = 0;
		int DIFF_SIZEKB = 0;
		int MINIMUM_SIZEKB = 0;
		unsigned int i, j;

		PRINTF("Data %s is already exist.\n", get->dir.dirName );

		/* Update only two files. SYS-DATA and USR-DATA */
		_fileOpCbStep = SAVE_OPERATION_STEP_MUSTEXISTFILE;

		/* Save the currently free space of the internal HDD */
		CURRENT_SIZEKB = get->hddFreeSizeKB;

		/* Initialize MINIMUM_SIZEKB */
		MINIMUM_SIZEKB = CURRENT_SIZEKB;
		
		/* Calculate the size of the new file in KB */
		for( i = 0; i <= FILE_INDEX_OPTION; i++ ) {
			PRINTF("checking size of %s ...\n", dataParam->fileList[i].fileName );

			/* Search the list of existing files for the name of the new file */
			for( j = 0; j < get->fileListNum; j++) {
				if (strcmp(dataParam->fileList[i].fileName, get->fileList[j].fileName) == 0) {
					break;
				}
			}
			if(j < get->fileListNum) {
				/* If found, calculate the difference. A negative value is also possible */
				DIFF_SIZEKB = SIZEKB(dataParam->fileList[i].fileSize) - SIZEKB(get->fileList[j].st_size);
				PRINTF("    file %s already exists. DIFF_SIZEKB : %d\n", dataParam->fileList[i].fileName, DIFF_SIZEKB );
			}
			else {
				/* If none exist, use the size of the new file itself */
				DIFF_SIZEKB = SIZEKB(dataParam->fileList[i].fileSize);
				PRINTF("    file %s is not exist. DIFF_SIZEKB : %d\n", dataParam->fileList[i].fileName, DIFF_SIZEKB );
			}
			/* Calculate the difference from CURRENT_SIZEKB */
			CURRENT_SIZEKB = CURRENT_SIZEKB - DIFF_SIZEKB;
			/* Then update the minimum value of CURRENT_SIZEKB */
			MINIMUM_SIZEKB = MIN( MINIMUM_SIZEKB, CURRENT_SIZEKB );
		}

		/* If it is a negative value, the free space check will result in an error */
		if (MINIMUM_SIZEKB < 0) {
			result->errNeedSizeKB = MINIMUM_SIZEKB;
			result->result = CELL_SAVEDATA_CBRESULT_ERR_NOSPACE;
			ERR_PRINTF("HDD size check error. needs %d KB disc space more.\n", result->errNeedSizeKB * -1 );
			return;
		}
	}
	else {
		/* When no save data exists */
		int NEW_SIZEKB = 0;
		int NEED_SIZEKB = 0;

		PRINTF("Data %s is not exist yet.\n", get->dir.dirName );

		/* Set parameters of PARAM.SFO */
		strncpy( set->setParam->title,	SAMPLE_PARAMSFO_TITLE, CELL_SAVEDATA_SYSP_TITLE_SIZE );
		set->setParam->title[CELL_SAVEDATA_SYSP_TITLE_SIZE-1] = 0;
		strncpy( set->setParam->subTitle,	dataParam->subTitle, CELL_SAVEDATA_SYSP_SUBTITLE_SIZE );
		set->setParam->subTitle[CELL_SAVEDATA_SYSP_SUBTITLE_SIZE-1] = 0;
		strncpy( set->setParam->detail,	dataParam->detail, CELL_SAVEDATA_SYSP_DETAIL_SIZE );
		set->setParam->detail[CELL_SAVEDATA_SYSP_DETAIL_SIZE-1] = 0;
		set->setParam->attribute = CELL_SAVEDATA_ATTR_NORMAL;
		strncpy( set->setParam->listParam, dataParam->listParam, CELL_SAVEDATA_SYSP_LPARAM_SIZE );
		set->setParam->listParam[CELL_SAVEDATA_SYSP_LPARAM_SIZE-1] = 0;
		memset( set->setParam->reserved, 0x0, sizeof(set->setParam->reserved) );	/* The reserved member must be zero-filled */
		memset( set->setParam->reserved2, 0x0, sizeof(set->setParam->reserved2) );	/* The reserved member must be zero-filled */

		/* Save all files. ICON0, ICON1, PIC1, SYS-DATA and USR-DATA */
		_fileOpCbStep = SAVE_OPERATION_STEP_ICON0;

		/* Calculate the size of the new save data in KB */
		for( unsigned int i = 0; i < FILE_INDEX_END; i++ ) {
			NEW_SIZEKB += SIZEKB(dataParam->fileList[i].fileSize);
		}

		/* Add the size of the system files(sysSizeKB) */
		NEW_SIZEKB += get->sysSizeKB;

		/* Then calculate NEED_SIZEKB */
		NEED_SIZEKB = get->hddFreeSizeKB - NEW_SIZEKB;
		
		//PRINTF("hddFreeSizeKB=[%d] NEW_SIZEKB=[%d] NEED_SIZEKB=[%d]\n", get->hddFreeSizeKB, NEW_SIZEKB, NEED_SIZEKB);

		/* If NEED_SIZEKB is a negative value, the free space check will result in an error */
		if (NEED_SIZEKB < 0) {
			result->errNeedSizeKB = NEED_SIZEKB;
			result->result = CELL_SAVEDATA_CBRESULT_ERR_NOSPACE;
			ERR_PRINTF("HDD size check error. needs %d KB disc space more.\n", result->errNeedSizeKB * -1 );
			return;
		}
	}

	/* Specify OK_NEXT to continue */
	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
	result->userdata = (void*)dataParam;	/* You can overwrite the userdata. also can receive this value in next callback */

	PRINTF( "result->result=[%d]\n", result->result );
	PRINTF( "cb_data_status_save() end\n");
	return;
}

/* Data status callback for cellSaveDataXXXXLoad() */
void cb_data_status_load( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	int foundMustExistData = 0;

	PRINTF( "cb_data_status_load() start\n");
	
	/* Dump obtained save data status */
	dumpCellSaveDataStatGet(get);

	dataParam_st *dataParam;

	/* Application-defined data (This sample specify userdata with mode of operation) */
	switch( (int)result->userdata ) {
	case MODE_LIST_SAVE:
	case MODE_LIST_LOAD:
		{
			int i;
			for( i = 0; i < MAX_LISTSAVEDATA_NUM; i++ ) {
				if( strcmp( sampleListData[i].dirName, get->dir.dirName ) == 0 ) {
					break;
				}
			}
			if( i < MAX_LISTSAVEDATA_NUM ) {
				dataParam = (void*)&sampleListData[i];
			}
			else {
				/* This won't actually happen */
				ERR_PRINTF(" list load error.\n" );
				dataParam = (void*)&sampleListData[0];
			}
		}
		break;
	case MODE_FIXED_SAVE:
	case MODE_FIXED_LOAD:
		{
			dataParam = (void*)&sampleFixedData;
		}
		break;
	case MODE_LIST_AUTO_SAVE:
	case MODE_LIST_AUTO_LOAD:
	case MODE_AUTO_SAVE:
	case MODE_AUTO_LOAD:
		{
			dataParam = (void*)&sampleAutoData;
		}
		break;
	default:
		ERR_PRINTF("unknown userdata value : %d\n", (int)result->userdata );
		result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
		return;
	}

	if(get->isNewData) {
		/* If the target save data to read cannot be found */
		ERR_PRINTF("Data %s is not found.\n", get->dir.dirName );
		result->result = CELL_SAVEDATA_CBRESULT_ERR_NODATA;
		return;
	}
	else {
		/* When target save data already exists */

		PRINTF("Data exist. fileListNum=[%d] fileNum=[%d]\n", get->fileListNum, get->fileNum);

		if ( get->fileListNum < get->fileNum ) {
			/* If number of file overflow, the save data should be judged as broken data */
			/* Because you may not find file needed in obtained list */
			result->result = CELL_SAVEDATA_CBRESULT_ERR_BROKEN;
			return;
		}

		/* Search the file to load from obtained list */
		for( unsigned int i = 0; i < get->fileListNum; i++ ) {
			/* Check SYS-DATA which must exist */
			if ( foundMustExistData == 0 && strcmp(get->fileList[i].fileName, MUSTEXISTFILE_NAME) == 0){
				PRINTF("found [%s]\n", get->fileList[i].fileName);
				foundMustExistData = 1;
				if(get->fileList[i].st_size != MUSTEXISTFILE_SIZE) {
					/* If it's not expected size, the save data should be judged as broken data */
					ERR_PRINTF("    size mismatch : expected[%u] != listed[%llu]\n",
							MUSTEXISTFILE_SIZE, get->fileList[i].st_size );
					result->result = CELL_SAVEDATA_CBRESULT_ERR_BROKEN;
					return;
				}
				continue;
			}
			/* Check USR-DATA. It's not a problem if this file doesn't exist */
			if ( _optionFileExist == 0 && strcmp(get->fileList[i].fileName, OPTIONFILE_NAME) == 0) {
				PRINTF("found [%s]\n", get->fileList[i].fileName);
				_optionFileExist = 1;
				if(get->fileList[i].st_size != OPTIONFILE_SIZE) {
					/* If it's not expected size, the save data should be judged as broken data */
					ERR_PRINTF("    size mismatch : expected[%u] != listed[%llu]\n",
							MUSTEXISTFILE_SIZE, get->fileList[i].st_size );
					result->result = CELL_SAVEDATA_CBRESULT_ERR_BROKEN;
					return;
				}
			}
		}
		if(foundMustExistData == 0) {
			/* When the data which must exist is not found, the save data should be judged as broken data */
			ERR_PRINTF("%s is not found\n", dataParam->fileList[FILE_INDEX_MUSTEXIST].fileName);
			result->result = CELL_SAVEDATA_CBRESULT_ERR_BROKEN;
			return;
		}

	}

	set->reCreateMode = CELL_SAVEDATA_RECREATE_NO_NOBROKEN;		/* Do not handle save data as corrupt data. */
	set->setParam = NULL;		 								/* PARAM.SFO won't be updated */
	prepareIndicator( (int)result->userdata, &set->indicator );	/* Settings for indicator which appear during loading operation */

	/* Specify OK_NEXT to continue */
	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
	result->userdata = (void*)dataParam;	/* You can overwrite the userdata. also can receive this value in next callback */

	/* Initialize the progress counter */
	_fileOpCbStep = LOAD_OPERATION_STEP_MUSTEXISTFILE;

	PRINTF( "result->result=[%d]\n", result->result );
	PRINTF( "cb_data_status_load() end\n");
	return;
}

/* File operation callback for cellSaveDataXXXXSave() */
void cb_file_operation_save( CellSaveDataCBResult *result, CellSaveDataFileGet *get, CellSaveDataFileSet *set )
{
	PRINTF( "cb_file_operation_save() start\n");

	int ret;
	(void)get;

	char filePath[CELL_FS_MAX_FS_PATH_LENGTH];
	unsigned int fsize = 0;
	
	/* In data status callback function, this sample overwrote userdata with structure of data */
	dataParam_st *dataParam = (dataParam_st*)result->userdata;

	if( _file_buffer ) {
		free(_file_buffer);
		_file_buffer = NULL;
	}

	/* Specify OK_NEXT to continue */
	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	switch (_fileOpCbStep) {
	case SAVE_OPERATION_STEP_ICON0:
		{
			PRINTF("saving ICON0.PNG ...\n");
			sprintf(filePath, "%s/%s", DATA_DIR, dataParam->fileList[FILE_INDEX_ICON0].fileName );
			ret = fileAllocLoad(filePath, &_file_buffer, &fsize);
			if(ret != 0){
				/* If the file cound not be opened, file operation will result in an error */
				ERR_PRINTF("file load error : %s\n", filePath);
				result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
				return;
			}

			set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
			set->fileBuf = _file_buffer;
			set->fileBufSize = fsize;
			set->fileSize = fsize;
			set->fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON0;
			set->reserved = NULL;
			/* At this fileType, No need to specify fileName */
		}
		break;
	case SAVE_OPERATION_STEP_ICON1:
		{
			/* The size of the file treated by processing immediately before is stored in the excSize */
			PRINTF("saved ICON0.PNG size : %u\n", get->excSize );

			PRINTF("saving ICON1.PAM ...\n");
			sprintf( filePath, "%s/%s", DATA_DIR, dataParam->fileList[FILE_INDEX_ICON1].fileName );
			ret = fileAllocLoad(filePath, &_file_buffer, &fsize);
			if(ret != 0){
				/* If the file cound not be opened, file operation will result in an error */
				ERR_PRINTF("file load error : %s\n", filePath);
				result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
				return;
			}

			set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
			set->fileBuf = _file_buffer;
			set->fileBufSize = fsize;
			set->fileSize = fsize;
			set->fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON1;
			set->reserved = NULL;
			/* At this fileType, No need to specify fileName */
		}
		break;
	case SAVE_OPERATION_STEP_PIC1:
		{
			/* The size of the file treated by processing immediately before is stored in the excSize */
			PRINTF("saved ICON1.PMF size : %u\n", get->excSize );

			PRINTF("saving PIC1.PNG ...\n");
			sprintf( filePath, "%s/%s", DATA_DIR, dataParam->fileList[FILE_INDEX_PIC1].fileName );
			ret = fileAllocLoad(filePath, &_file_buffer, &fsize);
			if(ret != 0){
				/* If the file cound not be opened, file operation will result in an error */
				ERR_PRINTF("file load error : %s\n", filePath);
				result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
				return;
			}

			set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
			set->fileBuf = _file_buffer;
			set->fileBufSize = fsize;
			set->fileSize = fsize;
			set->fileType = CELL_SAVEDATA_FILETYPE_CONTENT_PIC1;
			set->reserved = NULL;
			/* At this fileType, No need to specify fileName */
		}
		break;
	case SAVE_OPERATION_STEP_MUSTEXISTFILE:
		{
			/* The size of the file treated by processing immediately before is stored in the excSize */
			PRINTF("saved PIC1.PNG size : %u\n", get->excSize );

			PRINTF("saving SYS-DATA ...\n");
			sprintf( filePath, "%s/%s", DATA_DIR, dataParam->fileList[FILE_INDEX_MUSTEXIST].fileName );
			ret = fileAllocLoad(filePath, &_file_buffer, &fsize);
			if(ret != 0 || fsize < dataParam->fileList[FILE_INDEX_MUSTEXIST].fileSize){
				/* If the file cound not be opened, file operation will result in an error */
				ERR_PRINTF("file load error : %s\n", filePath);
				result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
				return;
			}

			set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
			set->fileBuf = _file_buffer;
			set->fileBufSize = dataParam->fileList[FILE_INDEX_MUSTEXIST].fileSize;
			set->fileName = dataParam->fileList[FILE_INDEX_MUSTEXIST].fileName;
			set->fileSize = dataParam->fileList[FILE_INDEX_MUSTEXIST].fileSize;
			set->fileOffset = 0;
			/* Save as protected data file */
			set->fileType = CELL_SAVEDATA_FILETYPE_SECUREFILE;
			memcpy( set->secureFileId, secureFileId, CELL_SAVEDATA_SECUREFILEID_SIZE );
			set->reserved = NULL;
		}
		break;
	case SAVE_OPERATION_STEP_OPTIONFILE:
		{
			/* The size of the file treated by processing immediately before is stored in the excSize */
			PRINTF("saved SYS-DATA size : %u\n", get->excSize );

			PRINTF("saving USR-DATA ...\n");
			sprintf( filePath, "%s/%s", DATA_DIR, dataParam->fileList[FILE_INDEX_OPTION].fileName );
			ret = fileAllocLoad(filePath, &_file_buffer, &fsize);
			if(ret != 0 || fsize < dataParam->fileList[FILE_INDEX_OPTION].fileSize){
				/* If the file cound not be opened, file operation will result in an error */
				ERR_PRINTF("file load error : %s\n", filePath);
				result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
				return;
			}

			set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
			set->fileBuf = _file_buffer;
			set->fileBufSize = dataParam->fileList[FILE_INDEX_OPTION].fileSize;
			set->fileName = dataParam->fileList[FILE_INDEX_OPTION].fileName;
			set->fileSize = dataParam->fileList[FILE_INDEX_OPTION].fileSize;
			set->fileOffset = 0;
			/* Save as protected data file */
			set->fileType = CELL_SAVEDATA_FILETYPE_SECUREFILE;
			memcpy( set->secureFileId, secureFileId, CELL_SAVEDATA_SECUREFILEID_SIZE );
			set->reserved = NULL;

		}
		break;
	case SAVE_OPERATION_STEP_END:
		{
			/* The size of the file treated by processing immediately before is stored in the excSize */
			PRINTF("saved USR-DATA size : %u\n", get->excSize );
		}
	default:
		{
			PRINTF("SAVE_OPERATION_STEP_END\n");
			/* Specify OK_LAST to return when all file operations are complete */
			result->result = CELL_SAVEDATA_CBRESULT_OK_LAST;
		}
		break;
	}

	/* Calculate the progress of the saving process and set to progressBarInc */
	result->progressBarInc = 100/SAVE_OPERATION_STEP_END;

	_fileOpCbStep++;

	PRINTF( "cb_file_operation_save() end\n");
	return;
}

/* File operation callback for cellSaveDataXXXXLoad() */
void cb_file_operation_load( CellSaveDataCBResult *result, CellSaveDataFileGet *get, CellSaveDataFileSet *set )
{
	PRINTF( "cb_file_operation_load() start\n");

	/* In data status callback function, this sample overwrote userdata with structure of data */
	dataParam_st *dataParam = (dataParam_st*)result->userdata;

	/* Specify OK_NEXT to continue */
	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	if(_fileOpCbStep == LOAD_OPERATION_STEP_OPTIONFILE) {
		/* The size of the file treated by processing immediately before is stored in the excSize */
		if( get->excSize != dataParam->fileList[FILE_INDEX_MUSTEXIST].fileSize ) {
			/* If the size of loaded file is not valid, file operation will result in an error */
			ERR_PRINTF("loaded file size mismatch : expected[%u] != excSize[%u]\n",
						dataParam->fileList[FILE_INDEX_MUSTEXIST].fileSize, get->excSize );
			result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
			free(_file_buffer);
			_file_buffer = NULL;
			return;
		}
#ifdef DEBUG_MODE
		else {	/* For confirmation, save obtained image to the file */
			char filePath[CELL_FS_MAX_FS_PATH_LENGTH];
			sprintf(filePath, "%s/Loaded_%s", DATA_DIR, dataParam->fileList[FILE_INDEX_MUSTEXIST].fileName);
			fileSimpleSave(filePath, _file_buffer, dataParam->fileList[FILE_INDEX_MUSTEXIST].fileSize);
		}
#endif
	}
	else if(_fileOpCbStep == LOAD_OPERATION_STEP_END) {
		/* The size of the file treated by processing immediately before is stored in the excSize */
		if( get->excSize != dataParam->fileList[FILE_INDEX_OPTION].fileSize ) {
			/* If the size of loaded file is not valid, file operation will result in an error */
			ERR_PRINTF("loaded file size mismatch : expected[%u] != excSize[%u]\n",
						dataParam->fileList[FILE_INDEX_OPTION].fileSize, get->excSize );
			result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
			free(_file_buffer);
			_file_buffer = NULL;
			return;
		}
#ifdef DEBUG_MODE
		else {	/* For confirmation, save obtained image to the file */
			char filePath[CELL_FS_MAX_FS_PATH_LENGTH];
			sprintf(filePath, "%s/Loaded_%s", DATA_DIR, dataParam->fileList[FILE_INDEX_OPTION].fileName);
			fileSimpleSave(filePath, _file_buffer, dataParam->fileList[FILE_INDEX_OPTION].fileSize);
		}
#endif
	}

	if( _file_buffer ) {
		free(_file_buffer);
		_file_buffer = NULL;
	}

	switch (_fileOpCbStep) {
	case LOAD_OPERATION_STEP_MUSTEXISTFILE:
		{
			/* Prepare the buffer to load SYS-DATA */
			unsigned int buf_size = dataParam->fileList[FILE_INDEX_MUSTEXIST].fileSize;
			_file_buffer = malloc( buf_size );
			if( _file_buffer == NULL ) {
				ERR_PRINTF("alloc failed\n");
				result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
				return;
			}

			set->fileOperation = CELL_SAVEDATA_FILEOP_READ;
			set->fileBuf = _file_buffer;
			set->fileBufSize = buf_size;
			set->fileOffset = 0;	/* Read from the head of the file */
			set->fileName = dataParam->fileList[FILE_INDEX_MUSTEXIST].fileName;
			set->fileSize = dataParam->fileList[FILE_INDEX_MUSTEXIST].fileSize;
			set->fileType = CELL_SAVEDATA_FILETYPE_SECUREFILE;
			memcpy( set->secureFileId, secureFileId, CELL_SAVEDATA_SECUREFILEID_SIZE );
			set->reserved = NULL;
		}
		break;
	case LOAD_OPERATION_STEP_OPTIONFILE:
		{
			if ( !_optionFileExist ) {
				PRINTF("skip %s load\n", dataParam->fileList[FILE_INDEX_OPTION].fileName); 
				break;
			}

			/* Prepare the buffer to load USR-DATA */
			unsigned int buf_size = dataParam->fileList[FILE_INDEX_OPTION].fileSize;
			_file_buffer = malloc( buf_size );
			if( _file_buffer == NULL ) {
				ERR_PRINTF("alloc failed\n");
				result->result = CELL_SAVEDATA_CBRESULT_ERR_FAILURE;
				return;
			}

			set->fileOperation = CELL_SAVEDATA_FILEOP_READ;
			set->fileBuf = _file_buffer;
			set->fileBufSize = buf_size;
			set->fileOffset = 0;	/* Read from the head of the file */
			set->fileName = dataParam->fileList[FILE_INDEX_OPTION].fileName;
			set->fileSize = dataParam->fileList[FILE_INDEX_OPTION].fileSize;
			set->fileType = CELL_SAVEDATA_FILETYPE_SECUREFILE;
			memcpy( set->secureFileId, secureFileId, CELL_SAVEDATA_SECUREFILEID_SIZE );
			set->reserved = NULL;
		}
		break;
	case LOAD_OPERATION_STEP_END:
	default:
		{
			PRINTF("SAVE_OPERATION_STEP_END\n");
			/* Specify OK_LAST to return when all file operations are complete */
			result->result = CELL_SAVEDATA_CBRESULT_OK_LAST;
		}
		break;
	}

	/* Calculate the progress of the loading process and set this rate to progressBarInc */
	result->progressBarInc = 100/LOAD_OPERATION_STEP_END;

	_fileOpCbStep++;
	
	PRINTF( "cb_file_operation_load() end\n");

	return;
}
