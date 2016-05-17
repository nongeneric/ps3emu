/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include "buffers.h"
#include "display.h"
#include "pngdec_ppu.h"


/*E definition of static functions. */
static int 		displayInit( void );
static int 		displayClose(void);
static int 		displayData(MyBuffs *dispBuffs,int msec);
static int 		displayAllocateBuffer(MyBuffs *dispBuffs);
static int		displayFreeBuffer(MyBuffs *dispBuffs);

static void*	pngDecCbControlMalloc(uint32_t size, void *cbCtrlMallocArg);
static int32_t	pngDecCbControlFree(void *ptr, void *cbCtrlFreeArg);

static int 		loadModules( void );
static int 		unloadModules( void );
static int 		createModules( SPngDecCtlInfo	*pPngdecCtlInfo );
static int 		destoryModules( SPngDecCtlInfo	*pPngdecCtlInfo );
static int 		setDecodeParam( SPngDecCtlInfo	*pPngdecCtlInfo );
static int 		openStream( SPngDecCtlInfo	*pPngdecCtlInfo , const char *pStreamName);
static int 		closeStream( SPngDecCtlInfo	*pPngdecCtlInfo );
static int 		decodeStream( SPngDecCtlInfo	*pPngdecCtlInfo ,MyBuffs *dispBuffs);
static int 		errorLog(int ret);


/*E primary PPU thread entry. */
SYS_PROCESS_PARAM(1001, 0x10000)

int main(void)
{
	
	int					ret;
	MyBuffs				dispBuffs;
	SPngDecCtlInfo		pngdecCtlInfo;
	
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
	
	/*E Create the PNG Decoder. */
	ret = createModules(&pngdecCtlInfo);
	if(ret != CELL_OK){
		EINFO(ret);
		goto DISP_INIT_ERR;
	}
	
	/*E Open PNG stream */
	ret = openStream(&pngdecCtlInfo , INPUT_STREAM_NAME );
	if(ret != CELL_OK){
		EINFO(ret);
		goto DEC_ERR;
	}
	
	/*E Set parameter  */
	ret = setDecodeParam( &pngdecCtlInfo );
	if(ret != CELL_OK){
		EINFO(ret);
		goto DEC_ERR;
	}
	
	/*E Decode PNG stream */
	ret = decodeStream(&pngdecCtlInfo , &dispBuffs );
	if(ret != CELL_OK){
		EINFO(ret);
		goto DEC_ERR;
	}
	
	/*E Display decoding data for 2 seconds. */
	ret = displayData( &dispBuffs ,1);
	if(ret != CELL_OK){
		EINFO(ret);
		goto DEC_ERR;
	}
	
DEC_ERR:
	/*E Close PNG stream */
	ret = closeStream( &pngdecCtlInfo );
	if(ret != CELL_OK){
		EINFO(ret);
	}
	
	/*E Destroy the PNG Decoder. */
	ret = destoryModules( &pngdecCtlInfo );
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
	
	DP("Call Malloc Function = %d\n", pngdecCtlInfo.ctrlCbArg.mallocCallCounts);
	DP("Call Free Function = %d\n",   pngdecCtlInfo.ctrlCbArg.freeCallCounts);
	
	return errorLog(ret);
}


/*E Create the PNG Decoder. */
static int createModules( SPngDecCtlInfo	*pPngdecCtlInfo )
{
	int							ret;
	CellPngDecThreadInParam 	threadInParam;
	CellPngDecThreadOutParam 	threadOutParam;
	
	/*E Set the spu thread disable/enable. */
	pPngdecCtlInfo->ctrlCbArg.mallocCallCounts	= 0;
	pPngdecCtlInfo->ctrlCbArg.freeCallCounts	= 0;
	threadInParam.spuThreadEnable	= CELL_PNGDEC_SPU_THREAD_DISABLE;
	threadInParam.ppuThreadPriority	= 512;
	threadInParam.spuThreadPriority	= 200;
	threadInParam.cbCtrlMallocFunc	= pngDecCbControlMalloc;
	threadInParam.cbCtrlMallocArg	= &pPngdecCtlInfo->ctrlCbArg;
	threadInParam.cbCtrlFreeFunc	= pngDecCbControlFree;
	threadInParam.cbCtrlFreeArg		= &pPngdecCtlInfo->ctrlCbArg;
	
	/*E Create the PNG Decoder. */
	ret = cellPngDecCreate( &pPngdecCtlInfo->mainHandle, &threadInParam, &threadOutParam );
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellPngDecCreate() returned CELL_OK\n");
	
	return ret;
}

/*E Destroy the PNG Decoder. */
static int destoryModules( SPngDecCtlInfo	*pPngdecCtlInfo )
{
	int		ret;
	
	ret = cellPngDecDestroy(pPngdecCtlInfo->mainHandle);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	
	DP("cellPngDecDestroy() returned CELL_OK\n");
	
	return ret;
}

/*E load the  module*/
static int loadModules()
{
	int ret;
	
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_PNGDEC);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	return ret;
}


/*E Unload the module*/
static int unloadModules( ){
	int ret;
	
	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_PNGDEC);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	
	return ret;
}

/*E Set parameter */
static int setDecodeParam( SPngDecCtlInfo	*pPngdecCtlInfo )
{
	int 						ret;
	CellPngDecInfo				pngdecInfo;
	CellPngDecInParam 			pngdecInParam;
	CellPngDecOutParam			pngdecOutParam;
	
	/*E Read the png header by PNG Decoder. */
	ret = cellPngDecReadHeader(pPngdecCtlInfo->mainHandle, pPngdecCtlInfo->subHandle, &pngdecInfo);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellPngDecReadHeader() returned CELL_OK\n");
	
	/*E Display the info */
	DP("info.imageWidth       = %d\n",   pngdecInfo.imageWidth);
	DP("info.imageHeight      = %d\n",   pngdecInfo.imageHeight);
	DP("info.numComponents    = %d\n",   pngdecInfo.numComponents);
	DP("info.bitDepth         = %d\n",   pngdecInfo.bitDepth);
	DP("info.colorSpace       = %d\n",   pngdecInfo.colorSpace);
	DP("info.interlaceMethod  = %d\n",   pngdecInfo.interlaceMethod);
	DP("info.chunkInformation = %x\n\n", pngdecInfo.chunkInformation);

	/*E Set the parameter for PNG Decoder. */
	pngdecInParam.commandPtr		= NULL;
	pngdecInParam.outputMode		= CELL_PNGDEC_TOP_TO_BOTTOM;
	pngdecInParam.outputColorSpace	= CELL_PNGDEC_RGBA;
	pngdecInParam.outputBitDepth	= 8;
	pngdecInParam.outputPackFlag	= CELL_PNGDEC_1BYTE_PER_1PIXEL;
	
	if(( pngdecInfo.colorSpace 	== CELL_PNGDEC_GRAYSCALE_ALPHA )
		||(pngdecInfo.colorSpace	== CELL_PNGDEC_RGBA)
		||(pngdecInfo.chunkInformation & 0x10))
	{
		pngdecInParam.outputAlphaSelect = CELL_PNGDEC_STREAM_ALPHA;
	}else
	{
		pngdecInParam.outputAlphaSelect = CELL_PNGDEC_FIX_ALPHA;
	}
	pngdecInParam.outputColorAlpha  = 0xff;
	
	ret = cellPngDecSetParameter(pPngdecCtlInfo->mainHandle, pPngdecCtlInfo->subHandle, &pngdecInParam, &pngdecOutParam);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellPngDecSetParameter() returned CELL_OK\n");
	
	DP("pngdecOutParam.outputWidthByte         = %lld\n", pngdecOutParam.outputWidthByte);
	DP("pngdecOutParam.outputWidth             = %d\n",   pngdecOutParam.outputWidth);
	DP("pngdecOutParam.outputHeight            = %d\n",   pngdecOutParam.outputHeight);
	DP("pngdecOutParam.outputComponents        = %d\n",   pngdecOutParam.outputComponents);
	DP("pngdecOutParam.outputBitDepth          = %d\n",   pngdecOutParam.outputBitDepth);
	DP("pngdecOutParam.outputMode              = %d\n",   pngdecOutParam.outputMode);
	DP("pngdecOutParam.outputColorSpace        = %d\n",   pngdecOutParam.outputColorSpace);
	DP("pngdecOutParam.useMemorySpace          = %d\n\n", pngdecOutParam.useMemorySpace);
	
	return ret;
}

/*E Open PNG stream */
static int openStream( SPngDecCtlInfo *pPngdecCtlInfo , const char *pStreamName ){
	
	int 					ret;
	CellPngDecSrc 			pngdecSrc; 
	CellPngDecOpnInfo		pngdecOpenInfo;
	
	/*E Set the stream source. */
	pngdecSrc.srcSelect		= CELL_PNGDEC_FILE;
	pngdecSrc.fileName		= pStreamName;
	pngdecSrc.fileOffset	= 0;
	pngdecSrc.fileSize		= 0;
	pngdecSrc.streamPtr		= NULL;
	pngdecSrc.streamSize	= 0;
	
	/*E Set the spu thread disable/enable. */
	pngdecSrc.spuThreadEnable  = CELL_PNGDEC_SPU_THREAD_DISABLE;
	
	DP("open filename = %s\n",pStreamName);
	/*E Open the stream. */
	ret = cellPngDecOpen( pPngdecCtlInfo->mainHandle, &pPngdecCtlInfo->subHandle, &pngdecSrc, &pngdecOpenInfo);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellPngDecOpen() returned CELL_OK\n");
	
	return ret;
}

/*E Close PNG stream */
static int closeStream( SPngDecCtlInfo	*pPngdecCtlInfo ){
	
	int ret;
	/*E Close stream */
	ret = cellPngDecClose( pPngdecCtlInfo->mainHandle, pPngdecCtlInfo->subHandle);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellPngDecClose() returned CELL_OK\n");
	
	return CELL_OK;
}

/*E Decode PNG stream */
static int decodeStream( SPngDecCtlInfo	*pPngdecCtlInfo ,MyBuffs *dispBuffs )
{
	
	int							ret;
	uint8_t						*pngdecDecodeData;
	CellPngDecDataCtrlParam		pngdecCtrlParam;
	CellPngDecDataOutInfo 		pngdecDataOutInfo;
	
	/*E Set the output area */
	pngdecCtrlParam.outputBytesPerLine 	= DISPLAY_WIDTH * 4;
	pngdecDecodeData 					= dispBuffs->element->buffer;
	memset(pngdecDecodeData, 0, (DISPLAY_WIDTH * DISPLAY_HEIGHT * 4)); 
	
	/*E Decode PNG */
	ret = cellPngDecDecodeData(pPngdecCtlInfo->mainHandle, pPngdecCtlInfo->subHandle, pngdecDecodeData, &pngdecCtrlParam, &pngdecDataOutInfo);
	if(ret != CELL_OK){
		EINFO(ret);
		return ret;
	}
	DP("cellPngDecDecodeData() returned CELL_OK\n");
	DP("pngdecDataOutInfo.chunkInformation  = %x\n",	pngdecDataOutInfo.chunkInformation);
	DP("pngdecDataOutInfo.numText           = %d\n",	pngdecDataOutInfo.numText);
	DP("pngdecDataOutInfo.numUnknownChunk   = %d\n",	pngdecDataOutInfo.numUnknownChunk);
	DP("pngdecDataOutInfo.status            = %d\n\n",	pngdecDataOutInfo.status);
	
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
	case CELL_PNGDEC_ERROR_HEADER:
		DP("PNG stream doesn't open \n");
		break;
	case CELL_PNGDEC_ERROR_STREAM_FORMAT:
		DP("PNG format error\n");
		break;
	case CELL_PNGDEC_ERROR_ARG:
		DP("The argument of function is a wrong.\n"); 
		break;
	case CELL_PNGDEC_ERROR_SEQ:
		DP("The execution sequence of function is a wrong.\n"); 
		break;
	case CELL_PNGDEC_ERROR_BUSY:
		DP("The PNG Decoder is busy.\n"); 
		break;
	case CELL_PNGDEC_ERROR_FATAL:
		DP("The PNG Decoder is fatal error.\n"); 
		break;
	case CELL_PNGDEC_ERROR_OPEN_FILE:
		DP("Can't open PNG stream\n");
		break;
	case CELL_PNGDEC_ERROR_SPU_UNSUPPORT:
		DP("Can't decode PNG stream with SPU.\n" );
		break;
	case CELL_PNGDEC_ERROR_SPU_ERROR:
		DP("Error occurs while decoding PNG stream with SPU.\n");
		break;
	default:
		DP("Error of Lv2/libc = 0x%x\n", ret);
		break;
	}
	return ret;
}

/*E Set the malloc callback function. */
static void *pngDecCbControlMalloc(uint32_t size, void *cbCtrlCbArg){
	CtrlCbArg *arg;
	arg = (CtrlCbArg *)cbCtrlCbArg;
	arg->mallocCallCounts++;
	return malloc(size);
}

/*E Set the free callback function. */
static int32_t pngDecCbControlFree(void *ptr, void *cbCtrlCbArg){
	CtrlCbArg *arg;
	arg = (CtrlCbArg *)cbCtrlCbArg;
	arg->freeCallCounts++;
	free(ptr);
	return 0;
}




