/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include "buffers.h"
#include "display.h"
#include "jdec_ppu.h"


/*E definition of static functions. */
static int 		displayInit( void );
static int 		displayClose(void);
static int 		displayData(MyBuffs *dispBuffs,int msec);
static int 		displayAllocateBuffer(MyBuffs *dispBuffs);
static int		displayFreeBuffer(MyBuffs *dispBuffs);

static void*	jpgDecCbControlMalloc(uint32_t size, void *cbCtrlMallocArg);
static int32_t	jpgDecCbControlFree(void *ptr, void *cbCtrlFreeArg);

static int 		loadModules( void );
static int 		unloadModules( void );
static int 		createModules( SJpgDecCtlInfo	*pJpgdecCtlInfo );
static int 		destoryModules( SJpgDecCtlInfo	*pJpgdecCtlInfo );
static int 		setDecodeParam( SJpgDecCtlInfo	*pJpgdecCtlInfo );
static int 		openStream( SJpgDecCtlInfo	*pJpgdecCtlInfo , const char *pStreamName);
static int 		closeStream( SJpgDecCtlInfo	*pJpgdecCtlInfo );
static int 		decodeStream( SJpgDecCtlInfo	*pJpgdecCtlInfo ,MyBuffs *dispBuffs);
static int 		errorLog(int ret);


/*E primary PPU thread entry. */
SYS_PROCESS_PARAM(1001, 0x10000)

int main(void)
{
	int					ret;
	MyBuffs				dispBuffs;
	SJpgDecCtlInfo		jpgdecCtlInfo;
	
	/*E load the  module*/
	ret = loadModules();
	if(ret != CELL_OK){
		EINFO(ret);
		return errorLog(ret);
	}
	
	/*E Intialize display */
	ret = displayInit();
	if(ret != CELL_OK){
		EINFO(ret);
		goto DISP_INIT_ERR;
	}
	
	/*E allocate buffer */
	ret = displayAllocateBuffer(&dispBuffs);
	if(ret != CELL_OK){
		EINFO(ret);
		goto DISP_INIT_ERR;
	}
	
	/*E Create the JPG Decoder. */
	ret = createModules(&jpgdecCtlInfo);
	if(ret != CELL_OK){
		EINFO(ret);
		goto DISP_INIT_ERR;
	}
	
	/*E Open JPG stream */
	ret = openStream(&jpgdecCtlInfo , INPUT_STREAM_NAME );
	if(ret != CELL_OK){
		EINFO(ret);
		goto DEC_ERR;
	}
	
	/*E Set parameter  */
	ret = setDecodeParam( &jpgdecCtlInfo );
	if(ret != CELL_OK){
		EINFO(ret);
		goto DEC_ERR;
	}
	
	/*E Decode JPG stream */
	ret = decodeStream(&jpgdecCtlInfo , &dispBuffs );
	if(ret != CELL_OK){
		EINFO(ret);
		goto DEC_ERR;
	}
	
	/*E Display decoding data for 2 seconds. */
	ret = displayData( &dispBuffs ,120);
	if(ret != CELL_OK){
		EINFO(ret);
		goto DEC_ERR;
	}
	
DEC_ERR:
	/*E Close JPG stream */
	ret = closeStream( &jpgdecCtlInfo );
	if(ret != CELL_OK){
		EINFO(ret);
	}
	
	/*E Destroy the JPG Decoder. */
	ret = destoryModules( &jpgdecCtlInfo );
	if(ret != CELL_OK){
		EINFO(ret);
	}
	
DISP_INIT_ERR:
	/*E Close display */
	ret = displayClose();
	if(ret != CELL_OK){
		EINFO(ret);
	}
	
	/*E free buffer */
	ret = displayFreeBuffer(&dispBuffs);
	if(ret != CELL_OK){
		EINFO(ret);
	}
	
	/*E Unload the module*/
	ret= unloadModules();
	if(ret != CELL_OK){
		EINFO(ret);
	}
	
	//DP("Call Malloc Function = %d\n", jpgdecCtlInfo.ctrlCbArg.mallocCallCounts);
	//DP("Call Free Function = %d\n",   jpgdecCtlInfo.ctrlCbArg.freeCallCounts);
	
	return errorLog(ret);
}


/*E Create the JPG Decoder. */
static int createModules( SJpgDecCtlInfo	*pJpgdecCtlInfo )
{
	int							ret;
	CellJpgDecThreadInParam 	threadInParam;
	CellJpgDecThreadOutParam 	threadOutParam;
	
	/*E Set the spu thread disable/enable. */
	pJpgdecCtlInfo->ctrlCbArg.mallocCallCounts	= 0;
	pJpgdecCtlInfo->ctrlCbArg.freeCallCounts	= 0;
	threadInParam.spuThreadEnable	= CELL_JPGDEC_SPU_THREAD_DISABLE;
	threadInParam.ppuThreadPriority	= 512;
	threadInParam.spuThreadPriority	= 200;
	threadInParam.cbCtrlMallocFunc	= jpgDecCbControlMalloc;
	threadInParam.cbCtrlMallocArg	= &pJpgdecCtlInfo->ctrlCbArg;
	threadInParam.cbCtrlFreeFunc	= jpgDecCbControlFree;
	threadInParam.cbCtrlFreeArg		= &pJpgdecCtlInfo->ctrlCbArg;
	
	/*E Create the JPG Decoder. */
	ret = cellJpgDecCreate( &pJpgdecCtlInfo->mainHandle, &threadInParam, &threadOutParam );
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellJpgDecCreate() returned CELL_OK\n");
	
	return ret;
}

/*E Destroy the JPG Decoder. */
static int destoryModules( SJpgDecCtlInfo	*pJpgdecCtlInfo )
{
	int		ret;
	
	ret = cellJpgDecDestroy(pJpgdecCtlInfo->mainHandle);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	
	DP("cellJpgDecDestroy() returned CELL_OK\n");
	
	return ret;
}

/*E load the  module*/
static int loadModules()
{
	int ret;
	
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_JPGDEC);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	return ret;
}


/*E Unload the module*/
static int unloadModules( ){
	int ret;
	
	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_JPGDEC);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	
	return ret;
}

/*E Set parameter */
static int setDecodeParam( SJpgDecCtlInfo	*pJpgdecCtlInfo )
{
	int 						ret;
	CellJpgDecInfo				jpgdecInfo;
	CellJpgDecInParam 			jpgdecInParam;
	CellJpgDecOutParam			jpgdecOutParam;
	float						downScale;
	
	/*E Read the jpg header by JPG Decoder. */
	ret = cellJpgDecReadHeader(pJpgdecCtlInfo->mainHandle, pJpgdecCtlInfo->subHandle, &jpgdecInfo);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellJpgDecReadHeader() returned CELL_OK\n");
	
	if(jpgdecInfo.jpegColorSpace == CELL_JPG_UNKNOWN){
		EMSG("%s is not supported by this decoder.\n", INPUT_STREAM_NAME);
		ret = -1;
		return ret;
	}
	
	/*E Display the info */
	DP("info.imageWidth       = %d\n",   jpgdecInfo.imageWidth);
	DP("info.imageHeight      = %d\n",   jpgdecInfo.imageHeight);
	DP("info.numComponents    = %d\n",   jpgdecInfo.numComponents);
	
	 /*E Set the downscale */
	if( ((float)jpgdecInfo.imageWidth / DISPLAY_WIDTH) > ((float)jpgdecInfo.imageHeight / DISPLAY_HEIGHT ) ){
		downScale = (float)jpgdecInfo.imageWidth / DISPLAY_WIDTH;
	}else{
		downScale = (float)jpgdecInfo.imageHeight / DISPLAY_HEIGHT; 
	}
	
	if( downScale <= 1.f ){
		jpgdecInParam.downScale = 1;
	}else if( downScale <= 2.f ){
		jpgdecInParam.downScale = 2;
	}else if( downScale <= 4.f ){
		jpgdecInParam.downScale = 4;
	}else{
		jpgdecInParam.downScale = 8;
	}
	/*E Set the parameter for JPEG Decoder. */
	jpgdecInParam.commandPtr		= NULL;
	jpgdecInParam.method			= CELL_JPGDEC_FAST;
	jpgdecInParam.outputMode		= CELL_JPGDEC_TOP_TO_BOTTOM;
	jpgdecInParam.outputColorSpace	= CELL_JPG_RGBA;
	jpgdecInParam.outputColorAlpha	= 0xff;
	
	ret = cellJpgDecSetParameter(pJpgdecCtlInfo->mainHandle, pJpgdecCtlInfo->subHandle, &jpgdecInParam, &jpgdecOutParam);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellJpgDecSetParameter() returned CELL_OK\n");
	
	DP("jpgdecOutParam.outputWidthByte         = %lld\n", jpgdecOutParam.outputWidthByte);
	DP("jpgdecOutParam.outputWidth             = %d\n",   jpgdecOutParam.outputWidth);
	DP("jpgdecOutParam.outputHeight            = %d\n",   jpgdecOutParam.outputHeight);
	DP("jpgdecOutParam.outputComponents        = %d\n",   jpgdecOutParam.outputComponents);
	DP("jpgdecOutParam.outputMode              = %d\n",   jpgdecOutParam.outputMode);
	DP("jpgdecOutParam.outputColorSpace        = %d\n",   jpgdecOutParam.outputColorSpace);
	DP("jpgdecOutParam.useMemorySpace          = %d\n\n", jpgdecOutParam.useMemorySpace);
	
	return ret;
}

/*E Open JPG stream */
static int openStream( SJpgDecCtlInfo *pJpgdecCtlInfo , const char *pStreamName ){
	
	int 					ret;
	CellJpgDecSrc 			jpgdecSrc; 
	CellJpgDecOpnInfo		jpgdecOpenInfo;
	
	/*E Set the stream source. */
	jpgdecSrc.srcSelect		= CELL_JPGDEC_FILE;
	jpgdecSrc.fileName		= pStreamName;
	jpgdecSrc.fileOffset	= 0;
	jpgdecSrc.fileSize		= 0;
	jpgdecSrc.streamPtr		= NULL;
	jpgdecSrc.streamSize	= 0;
	
	/*E Set the spu thread disable/enable. */
	jpgdecSrc.spuThreadEnable  = CELL_JPGDEC_SPU_THREAD_DISABLE;
	
	DP("open filename = %s\n",pStreamName);
	/*E Open the stream. */
	ret = cellJpgDecOpen( pJpgdecCtlInfo->mainHandle, &pJpgdecCtlInfo->subHandle, &jpgdecSrc, &jpgdecOpenInfo);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellJpgDecOpen() returned CELL_OK\n");
	
	return ret;
}

/*E Close JPG stream */
static int closeStream( SJpgDecCtlInfo	*pJpgdecCtlInfo ){
	
	int ret;
	/*E Close stream */
	ret = cellJpgDecClose( pJpgdecCtlInfo->mainHandle, pJpgdecCtlInfo->subHandle);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellJpgDecClose() returned CELL_OK\n");
	
	return CELL_OK;
}

/*E Decode JPG stream */
static int decodeStream( SJpgDecCtlInfo	*pJpgdecCtlInfo ,MyBuffs *dispBuffs )
{
	int							ret;
	uint8_t						*jpgdecDecodeData;
	CellJpgDecDataCtrlParam		jpgdecCtrlParam;
	CellJpgDecDataOutInfo 		jpgdecDataOutInfo;
	
	/*E Set the output area */
	jpgdecCtrlParam.outputBytesPerLine 	= DISPLAY_WIDTH * 4;
	jpgdecDecodeData 					= dispBuffs->element->buffer;
	memset(jpgdecDecodeData, 0, (DISPLAY_WIDTH * DISPLAY_HEIGHT * 4)); 
	
	/*E Decode JPG */
	ret = cellJpgDecDecodeData(pJpgdecCtlInfo->mainHandle, pJpgdecCtlInfo->subHandle, jpgdecDecodeData, &jpgdecCtrlParam, &jpgdecDataOutInfo);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellJpgDecDecodeData() returned CELL_OK\n");
	
	return ret;
}

/*E Intialize display */
static int displayInit( void )
{
	int ret;
	
	ret = my_disp_create( DISPLAY_WIDTH,DISPLAY_HEIGHT );
	if(ret != 0){
		DP( "displayInit: my_disp_create fail ... exit\n" );
		return ret;
	}
	DP( "displayInit: create display ... WIDTH=%d, HEIGHT=%d\n",DISPLAY_WIDTH, DISPLAY_HEIGHT );
	
	return CELL_OK;
}

/*E allocate buffer */
static int displayAllocateBuffer(MyBuffs *dispBuffs){
	int 		ret;
	uint32_t	align_mask;
	size_t 		frame_buf_size;
	uint32_t 	frame_buf_align;
	
	/*E align must be 1MB for libgcm. */
	align_mask		= 0xfffff;
	frame_buf_align = 0x100000;

	/*E mem size for frame buffer. */
	frame_buf_size = FRAME_WIDTH * FRAME_HEIGHT * 4;

	/*E buffer must be integer multiple for libgcm. */
	frame_buf_size = ( frame_buf_size + align_mask ) & ( ~align_mask );

	/*E allocate buffers for display. */
	ret = my_buffs_create(dispBuffs, 1, frame_buf_size, frame_buf_align);
	if(ret != 0){
		DP( "displayInit: my_buffs_create fail ... exit\n" );
		return ret;
	}
	
	my_buffs_open( dispBuffs );
	my_disp_mapmem( dispBuffs->element->buffer, frame_buf_size ,&dispBuffs->offset );
	
	return CELL_OK;
}

/*E free buffer */
static int displayFreeBuffer(MyBuffs *dispBuffs){
	/*E free buffers for display */
	my_disp_unmapmem( dispBuffs->offset );
	my_buffs_destroy( dispBuffs );
	my_buffs_close( dispBuffs );
	return CELL_OK;
}

/*E Display decoding data  */
static int displayData(MyBuffs *dispBuffs,int msec)
{
	int ret=0;
	int i;
	
	for(i=0;i<msec;i++){
		my_disp_settex( dispBuffs->element->buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT );
		ret = my_disp_display();
		if(ret != 0){
			DP( "DisplayData fail ... exit\n" );
			return ret;
		}
	}
	return ret;
}

/*E Close display */
static int displayClose(void)
{
	my_disp_destroy();
	return CELL_OK;
}

/*E Check the return code. */
static int errorLog(int ret)
{
	switch(ret){
	case CELL_OK:
		DP("Finished decoding \n");
		break;
	case CELL_JPGDEC_ERROR_HEADER:
		DP("%s is not JPEG stream\n", INPUT_STREAM_NAME);
		break;
	case CELL_JPGDEC_ERROR_STREAM_FORMAT:
		DP("%s is not completely compliant with the JPEG format.\n", INPUT_STREAM_NAME);
		break;
	case CELL_JPGDEC_ERROR_ARG:
		DP("The argument of function is a wrong.\n"); 
		break;
	case CELL_JPGDEC_ERROR_SEQ:   
		DP("The execution sequence of function is a wrong.\n"); 
		break;
	case CELL_JPGDEC_ERROR_BUSY:   
		DP("The JPEG Decoder is busy.\n"); 
		break;
	case CELL_JPGDEC_ERROR_FATAL:   
		DP("The JPEG Decoder is fatal error.\n"); 
		break;
	case CELL_JPGDEC_ERROR_OPEN_FILE:  
		DP("Can't open %s\n", INPUT_STREAM_NAME);
		break;
	case CELL_JPGDEC_ERROR_SPU_UNSUPPORT:
		DP("Can't decode %s with SPU.\n", INPUT_STREAM_NAME);
		break;
	case CELL_JPGDEC_ERROR_CB_PARAM:
		DP("The parameter of callback function is a wrong.\n");
		break;
	default:
		DP("Error of Lv2/libc = 0x%x\n", ret);
		break;
	}
	return ret;
}

/*E Set the malloc callback function. */
static void *jpgDecCbControlMalloc(uint32_t size, void *cbCtrlCbArg){
	CtrlCbArg *arg;
	arg = (CtrlCbArg *)cbCtrlCbArg;
	arg->mallocCallCounts++;
	return malloc(size);
}

/*E Set the free callback function. */
static int32_t jpgDecCbControlFree(void *ptr, void *cbCtrlCbArg){
	CtrlCbArg *arg;
	arg = (CtrlCbArg *)cbCtrlCbArg;
	arg->freeCallCounts++;
	free(ptr);
	return 0;
}
