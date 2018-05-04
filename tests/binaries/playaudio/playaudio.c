/* SCE CONFIDENTIAL
 PlayStation(R)3 Programmer Tool Runtime Library 475.001
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#define USE_NOTIFY_EVENT	1	/* if !0, notify event is used for buffer consuming check */
#define USE_FAST_FEED		0	/* if !0, audio data feeding is done with least latency */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timer.h>
#include <sys/process.h>
#include <sys/paths.h>
#include <sys/ppu_thread.h>
#if USE_NOTIFY_EVENT != 0
#include <sys/event.h>					/* event queue */
#endif

#include <cell/sysmodule.h>
#include <cell/audio.h>
#include <sysutil/sysutil_sysparam.h>	/* CELL_SYSUTIL_* */

extern int  systemUtilityInit(void *);	/* init.c */
extern void systemUtilityQuit(void);	/* init.c */
extern void v_interval(void);			/* init.c */

#define DATA_LOCATION_CELL_DATA_DIR	0
#define DATA_LOCATION_HOME_DIR		1
#define DATA_LOCATION_DISC			2
#define DATA_LOCATION_HDD			3

#define DATA_LOCATION	DATA_LOCATION_CELL_DATA_DIR /* _HOME_DIR, _DISC or _HDD */

#if   DATA_LOCATION == DATA_LOCATION_HOME_DIR
#define DATA_DIR	SYS_APP_HOME "/"
#elif DATA_LOCATION == DATA_LOCATION_DISC
#define DATA_DIR	SYS_DEV_BDVD "/PS3_GAME/USRDIR/"
#elif DATA_LOCATION == DATA_LOCATION_HDD
#define DATA_DIR	SYS_DEV_HDD0 "/"
#else
#define DATA_DIR	SYS_HOST_ROOT "/" CELL_DATA_DIR "/sound/waveform/"
#endif

#define TEST_FILE_NAME	DATA_DIR "Sample-48k-stereo.raw"

#define CHANNEL		CELL_AUDIO_PORT_2CH
#define BLOCK		CELL_AUDIO_BLOCK_8

char				*dataBuffer = NULL;
unsigned int		portNum, portNum1, portNum2;
int					dataSize = 0;
sys_addr_t			portAddr;
sys_addr_t			readIndexAddr;

sys_ppu_thread_t	soundThread = 0;
volatile int		isRunning = 0;

int  fileOpen(char **theBuffer);
void soundMain(uint64_t ui __attribute__((unused)));
int  audioInit(void);
void audioQuit(void);
int  sysmoduleInit(void);
void sysmoduleQuit(void);
void sysutilCallback(uint64_t, uint64_t, void *);

#if USE_NOTIFY_EVENT != 0
sys_event_queue_t	queue;
uint64_t			queueKey;
#endif

/* file open */
int fileOpen(char **theBuffer)
{
	FILE			*fp;
	int				fileSize;
	unsigned int	blockSize;
	unsigned int	allBlockSize;
	unsigned int	count;
	unsigned int	mod;
	unsigned int	memSize;
	char			*buffer;

	fp = fopen(TEST_FILE_NAME, "r");
	if (fp == NULL){
		return -1;
	}
	
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	blockSize = CHANNEL * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES;
	allBlockSize = fileSize / blockSize;
	mod          = fileSize % blockSize;
	if (mod){
		allBlockSize++;
	}
	memSize = allBlockSize * blockSize;
	
	*theBuffer = NULL;
	*theBuffer = (char *)malloc(memSize);
	if (*theBuffer == NULL){
		fclose(fp);
		return -1;
	}
	
	memset(*theBuffer, 0, memSize);
	buffer = *theBuffer;
	
	count = fileSize / 4096;
	mod   = fileSize % 4096;
	if (count){
		fread((void *)buffer, 4096, count, fp);
		buffer += (4096 * count);
	}
	if (mod){
		fread((void *)buffer, mod, 1, fp);
	}
	
	fclose(fp);
	
	return memSize;
}

/* sound thread */
void soundMain(uint64_t ui __attribute__((unused)))
{
	int				err;
	char			*curBuffer;
	unsigned int	blockSize;
	uint64_t	currentReadBlock = 0;
#if USE_FAST_FEED == 0
	unsigned int	readByteCount = 0;
	unsigned int	readSize;
#endif
#if USE_NOTIFY_EVENT == 0
	unsigned int	lastReadBlock = 0;
#endif
	unsigned int	nextWriteBlock = 0;
	unsigned int	endFlag = 0;
	
	blockSize = CHANNEL * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES;
	curBuffer = (char *)dataBuffer;
	
#if USE_FAST_FEED == 0
	/* fill data */
	readSize = blockSize * BLOCK;
	memcpy((void *)portAddr, curBuffer, readSize);
	curBuffer += readSize;
	readByteCount += readSize;
#endif	
#if USE_NOTIFY_EVENT != 0
	sys_event_queue_drain(queue);
#endif

	/* start port */
	err = cellAudioPortStart(portNum);
	/*err = cellAudioPortStart(portNum1);
	err = cellAudioPortStart(portNum2);*/
	printf("cellAudioPortStart() : %x\n", err);
	
	char* buf = malloc(0x20010250 - 0x20010000);

	while (isRunning != 0){
#if USE_NOTIFY_EVENT != 0
		sys_event_t  event;
		int64_t tag, diff = 0;
		usecond_t sec;
		usecond_t lastSec;
		err = sys_event_queue_receive(queue, &event, 4 * 1000 * 1000);
		
		currentReadBlock = *(uint64_t*)readIndexAddr;

		/*
		err = cellAudioGetPortBlockTag(portNum, currentReadBlock, &tag);
		if (err) {
			printf("bad tag!\n");
		}

		int64_t currentTag = tag;

		while (tag > 0 && cellAudioGetPortTimestamp(portNum, tag, &sec)) {
			tag -= 2;
		}
		
		int64_t lastTsTag = tag;

		while (tag > 0 && !cellAudioGetPortTimestamp(portNum, tag, &sec)) {
			tag -= 2;
		}
		
		int64_t firstTsTag = tag;
		int64_t tsCount = lastTsTag - firstTsTag;
		cellAudioGetPortTimestamp(portNum, lastTsTag, &sec);
		usecond_t sinceLastSec = sec - lastSec;
		const char* direction = sec > lastSec ? "UP" : "DOWN !!!";

		printf("readIndex = %llx, currentTag = %llx, lastTsTag = %llx, tsCount = %llx, curTS = %llx, sinceLastSec = %lld, dir = %s\n",
			currentReadBlock, currentTag, lastTsTag, tsCount, sec, sinceLastSec, direction);

		*/

		lastSec = sec;

		//while (tag - diff > 1 && cellAudioGetPortTimestamp(portNum, tag - diff, &sec)) {
		//	diff += 2;
		//}
		//uint64_t first = diff;
		//while (tag - diff > 1 && ) {
		//	uint32_t ptr = 0x20010000;
		//	//memcpy(buf, (uint64_t*)ptr, 0x20010250 - 0x20010000);

		//	uint64_t curtag = tag - diff;
		//	uint64_t cursec = sec;

		//	//uint64_t tag_shift = 1;
		//	//uint64_t cached = *(uint64_t*)0x20010158;
		//	//uint64_t growing = *(uint64_t*)0x200101b8;
		//	//uint64_t offset = growing & 0xff;
		//	//uint64_t last = growing >> 8;
		//	//uint64_t x = ((last - (cached + curtag)) >> tag_shift) + offset;
		//	//int flag = 0;
		//	//if (x > 0xf) {
		//	//	x -= 0xf;
		//	//	flag = 1;
		//	//}
		//	//uint64_t final = 0x200101b8 + (x << 3);
		//	//uint64_t timestamp = (*(uint64_t*)final * 1000000ull) / 79800000ull;
		//	//printf("cached=%llx, offset=%llx, last=%llx, x=%llx, final=%llx, timestamp=%llx(raw %llx) [actual sec=%llx] flag = %d\n",
		//	//	cached, offset, last, x, final, timestamp, *(uint64_t*)final, cursec, flag);

		//	//printf("curtag = %llx; cursec = %llx\n", curtag, cursec);
		//	//for (; ptr != 0x20010250; ptr += 16) {
		//	//	uint64_t* dump = (uint64_t*)(buf + ptr - 0x20010000);
		//	//	printf("%08x:  %016llx  %016llx\n", ptr, dump[0], dump[1]);
		//	//}
		//	//printf("\n");


		//	diff += 2;
		//}
		//uint64_t last = diff;
		//printf("tag[currentReadBlock] = %llx (%llx us / first=%llx last=%llx)\n", tag, sec, first, last);

		if (err == ETIMEDOUT){
			printf(" *** Something wrong, there is no event for four seconds ...\n");
			continue;
		}
#endif

		//printf("reading from %x value %llx\n", (uint32_t)readIndexAddr, *(uint64_t*)readIndexAddr);

		currentReadBlock = *(uint64_t*)readIndexAddr;
#if USE_NOTIFY_EVENT == 0
		//printf("currentReadBlock(%x), lastReadBlock: %x %x\n", readIndexAddr, currentReadBlock, lastReadBlock);
		if (currentReadBlock != lastReadBlock){

#endif			
#if USE_FAST_FEED != 0
			if ((curBuffer + blockSize) >= ((char *)dataBuffer + dataSize)){
				endFlag = 1;
			}
			nextWriteBlock = (currentReadBlock + 1) % BLOCK; /* write target is next block */
			memcpy((void *)(portAddr + (nextWriteBlock * blockSize)), curBuffer, blockSize);
			if (endFlag){
				goto rep;
			}
			curBuffer += blockSize;
#else /* USE_FAST_FEED == 0 */
			if (currentReadBlock > nextWriteBlock){
				readSize = (currentReadBlock - nextWriteBlock) * blockSize;
				if ((curBuffer + readSize) >= ((char *)dataBuffer + dataSize)){
					readSize = dataSize - readByteCount;
					endFlag = 1;
				}
				//printf("memcpy to %x\n", (void *)(portAddr + (nextWriteBlock * blockSize)));
				memcpy((void *)(portAddr + (nextWriteBlock * blockSize)),
					   curBuffer, readSize);
				if (endFlag){
					goto rep;
				}
				curBuffer += readSize;
				readByteCount += readSize;
				nextWriteBlock = currentReadBlock;
			} else if (currentReadBlock < nextWriteBlock){
				readSize = (BLOCK - nextWriteBlock) * blockSize;
				if ((curBuffer + readSize) >= ((char *)dataBuffer + dataSize)){
					readSize = dataSize - readByteCount;
					endFlag = 1;
				}
				//printf("memcpy to %x\n", (void *)(portAddr + (nextWriteBlock * blockSize)));
				memcpy((void *)(portAddr + (nextWriteBlock * blockSize)),
					   curBuffer, readSize);
				if (endFlag){
					goto rep;
				}
				curBuffer += readSize;
				readByteCount += readSize;
				readSize = currentReadBlock * blockSize;
				if ((curBuffer + readSize) >= ((char *)dataBuffer + dataSize)){
					readSize = dataSize - readByteCount;
					endFlag = 1;
				}
				//printf("memcpy to %x\n", (void *)portAddr);
				memcpy((void *)portAddr, curBuffer, readSize);
				if (endFlag){
					goto rep;
				}
				curBuffer += readSize;
				readByteCount += readSize;
				nextWriteBlock = currentReadBlock;
			}
#endif /* USE_FAST_FEED */
#if USE_NOTIFY_EVENT == 0
			lastReadBlock = currentReadBlock;
		}
#endif		
rep:

		if (endFlag){
			isRunning = 0;
			curBuffer = (char *)dataBuffer;
#if USE_FAST_FEED == 0
			readByteCount = 0;
#endif
			endFlag = 0;
		}
#if USE_NOTIFY_EVENT == 0
		sys_timer_usleep(2 * 1000); /* 2msec */
#endif
	}

	err = cellAudioPortStop(portNum);
	printf("cellAudioPortStop() : %x\n", err);

	sys_ppu_thread_exit(0);
}

/* init */
int audioInit(void)
{
	int					err;
	CellAudioPortParam	audioParam;
	CellAudioPortConfig	portConfig;

	/* audio system init. */
	err = cellAudioInit();
	printf("cellAudioInit() : %x\n", err);
	if (err != CELL_OK){
		free(dataBuffer);
		return -1;
	}

	printf("opening first port\n");
	
	/* audio port open. */
	audioParam.nChannel = 2;
	audioParam.nBlock   = 8;
	audioParam.attr     = 0;
	err = cellAudioPortOpen(&audioParam, &portNum);
	printf("cellAudioPortOpen() : %x  port %d\n", err, portNum);
	if (err != CELL_OK){
		cellAudioQuit();
		free(dataBuffer);
		return -1;
	}
	
	/* get port config. */
	err = cellAudioGetPortConfig(portNum, &portConfig);
	printf("cellAudioGetPortConfig() : %x\n", err);
	if (err != CELL_OK){
		cellAudioPortClose(portNum);
		cellAudioQuit();
		free(dataBuffer);
		return -1;
	}
	
	printf("readIndexAddr=%x\n", portConfig.readIndexAddr);
	printf("status=%x\n", portConfig.status);
	printf("nChannel=%llx\n", portConfig.nChannel);
	printf("nBlock=%llx\n", portConfig.nBlock);
	printf("portSize=%x\n", portConfig.portSize);
	//printf("portAddr=%x\n", portConfig.portAddr);

	/*printf("opening second port\n");

	audioParam.nChannel = 2;
	audioParam.nBlock   = 8;
	audioParam.attr     = 0;
	err = cellAudioPortOpen(&audioParam, &portNum1);
	printf("cellAudioPortOpen() : %x  port %d\n", err, portNum1);
	if (err != CELL_OK){
		cellAudioQuit();
		free(dataBuffer);
		return -1;
	}
	
	err = cellAudioGetPortConfig(portNum1, &portConfig);
	printf("cellAudioGetPortConfig() : %x\n", err);
	if (err != CELL_OK){
		cellAudioPortClose(portNum);
		cellAudioQuit();
		free(dataBuffer);
		return -1;
	}
	
	printf("readIndexAddr=%x\n", portConfig.readIndexAddr);
	printf("status=%x\n", portConfig.status);
	printf("nChannel=%llx\n", portConfig.nChannel);
	printf("nBlock=%llx\n", portConfig.nBlock);
	printf("portSize=%x\n", portConfig.portSize);
	printf("portAddr=%x\n", portConfig.portAddr);


	printf("opening third port\n");

	audioParam.nChannel = CELL_AUDIO_PORT_8CH;
	audioParam.nBlock   = 32;
	audioParam.attr     = 0;
	err = cellAudioPortOpen(&audioParam, &portNum2);
	printf("cellAudioPortOpen() : %x  port %d\n", err, portNum2);
	if (err != CELL_OK){
		cellAudioQuit();
		free(dataBuffer);
		return -1;
	}
	
	err = cellAudioGetPortConfig(portNum2, &portConfig);
	printf("cellAudioGetPortConfig() : %x\n", err);
	if (err != CELL_OK){
		cellAudioPortClose(portNum);
		cellAudioQuit();
		free(dataBuffer);
		return -1;
	}
	
	printf("readIndexAddr=%x\n", portConfig.readIndexAddr);
	printf("status=%x\n", portConfig.status);
	printf("nChannel=%llx\n", portConfig.nChannel);
	printf("nBlock=%llx\n", portConfig.nBlock);
	printf("portSize=%x\n", portConfig.portSize);
	printf("portAddr=%x\n", portConfig.portAddr);*/

	portAddr      = portConfig.portAddr;
	readIndexAddr = portConfig.readIndexAddr;
	
#if USE_NOTIFY_EVENT != 0
	err = cellAudioCreateNotifyEventQueue(&queue, &queueKey);
	printf("cellAudioCreateNotifyEventQueue() : %x\n", err);
	if (err != CELL_OK){
		cellAudioPortClose(portNum);
		cellAudioQuit();
		free(dataBuffer);
		return -1;
	}

	/* register event queue to libaudio */
	err = cellAudioSetNotifyEventQueue(queueKey);
	printf("cellAudioSetNotifyEventQueue() : %x\n", err);
	if (err < 0){
		(void)sys_event_queue_destroy(queue, 0);
		cellAudioPortClose(portNum);
		cellAudioQuit();
		free(dataBuffer);
		return -1;
	}
#endif

	return 0;
}

void audioQuit(void)
{
	int err;

#if USE_NOTIFY_EVENT != 0
	err = cellAudioRemoveNotifyEventQueue(queueKey);
	printf("cellAudioDeleteNotifyEventQueue() : %x\n", err);

	err = sys_event_queue_destroy(queue, 0);
	printf("sys_event_queue_destroy() : %x\n", err);
#endif

	err = cellAudioPortClose(portNum);
	printf("cellAudioPortClose() : %x\n", err);

	err = cellAudioQuit();
	printf("cellAudioQuit() : %x\n", err);

	free(dataBuffer);
}

int sysmoduleInit(void)
{
	int err;

	err = cellSysmoduleInitialize();
	printf("cellSysmoduleInitialize() : %x\n", err);
	if (err != CELL_OK){
		return -1;
	}

#if 0
	err = cellSysmoduleLoadModule(CELL_SYSMODULE_...);
	printf("cellSysmoduleLoadModule() : %x\n", err);
	if (err != CELL_OK){
		(void)cellSysmoduleFinalize();
		return -1;
	}
#endif

	return 0;
}

void sysmoduleQuit(void)
{
#if 0
	(void)cellSysmoduleUnloadModule(CELL_SYSMODULE_...);
#endif
	(void)cellSysmoduleFinalize();

	return;
}

/* sysutil callback:
 * resistered by cellSysutilRegisterCallback(); init.c */
void sysutilCallback(uint64_t status,
					 uint64_t param    __attribute__((unused)),
					 void    *userdata __attribute__((unused)))
{
	printf("sysutil callback: status=%016llx\n", status) ;

	switch (status){
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		printf("sysutil callback: EXITGAME is received.\n");
		isRunning = 0;			/* stop `soundMain' thread */
		break;
	}

	return;
}

SYS_PROCESS_PARAM(1001, 0x10000);

/* main */
int main(void)
{
	int		 err;
	uint64_t return_status;

	/* PRX environment init */
	err = sysmoduleInit();
	if (err < 0){
		printf("sysmoduleInit() error.\n");
		return -1;
	}

	/* file open. */
	err = fileOpen(&dataBuffer);
	if (err < 0){
		printf("fileOpen() error.\n");
		return -1;
	}
	dataSize = err;
	
	/* system utility init */
	err = systemUtilityInit((void *)sysutilCallback); /* init.c */
	if (err < 0){
		printf("systemUtilityInit() error.\n");
		return -1;
	}

	/* audio init */
	err = audioInit();
	if (err < 0){
		return -1;
	}
	
	isRunning = 1;

	/* sound thread create */
	err = sys_ppu_thread_create(&soundThread, soundMain,
								0, 100, 0x8000,
								SYS_PPU_THREAD_CREATE_JOINABLE,
								(char *)"sound thread");
	while (isRunning != 0){
		v_interval();			/* init.c */
	}

	/* waiting for stop soundMain thread */
	sys_ppu_thread_join(soundThread, &return_status);
	printf("soundMain exit = %lld\n", return_status);

	/* audio quit */
	audioQuit();

	/* system utility quit */
	systemUtilityQuit();	/* init.c */

	/* PRX environment quit */
	sysmoduleQuit();
	
	return 0;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * End:
 * vi:ts=4:sw=4
 */
