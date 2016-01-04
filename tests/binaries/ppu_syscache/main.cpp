#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sysutil\sysutil_sysparam.h>
#include <sysutil\sysutil_common.h>
#include <sysutil\sysutil_syscache.h>
#include <sys/fs.h>
#include <cell/cell_fs.h>
#include <sys/paths.h>
#include <string.h>

SYS_PROCESS_PARAM(1001, 0x10000)

#define SYSCACHE_1		"GAME-DATA1"
#define SYSCACHE_2		"GAME-DATA2"
#define DATA_DIR		SYS_APP_HOME "/DATA"

int fileAllocLoad(const char *filePath, void **buf, unsigned int *size)
{
	int ret;
	int fd;
	CellFsStat status;
	uint64_t readlen;

	ret = cellFsOpen(filePath, CELL_FS_O_RDONLY, &fd, NULL, 0);
	if(ret != CELL_FS_SUCCEEDED){
		printf("file %s open error : 0x%x\n", filePath, ret);
		return -1;
	}

	ret = cellFsFstat(fd, &status);
	if(ret != CELL_FS_SUCCEEDED){
		printf("file %s get stat error : 0x%x\n", filePath, ret);
		cellFsClose(fd);
		return -1;
	}

	*buf = malloc( (size_t)status.st_size );
	if( *buf == NULL ) {
		printf("alloc failed\n");
		cellFsClose(fd);
		return -1;
	}

	ret = cellFsRead(fd, *buf, status.st_size, &readlen);
	if(ret != CELL_FS_SUCCEEDED || status.st_size != readlen ) {
		printf("file %s read error : 0x%x\n", filePath, ret);
		cellFsClose(fd);
		free(*buf);
		*buf = NULL;
		return -1;
	}

	ret = cellFsClose(fd);
	if(ret != CELL_FS_SUCCEEDED){
		printf("file %s close error : 0x%x\n", filePath, ret);
		free(*buf);
		*buf = NULL;
		return -1;
	}

	*size = status.st_size;

	return 0;
}

int fileSimpleSave(const char *filePath, void *buf, unsigned int fileSize)
{
	int ret;
	int fd;
	uint64_t writelen;

	if( buf == NULL ) {
		printf("buffer is null\n");
	}

	ret = cellFsOpen(filePath, CELL_FS_O_WRONLY|CELL_FS_O_CREAT|CELL_FS_O_TRUNC, &fd, NULL, 0);
	if(ret != CELL_FS_SUCCEEDED){
		printf("file %s open error : 0x%x\n", filePath, ret);
		return -1;
	}

	ret = cellFsWrite(fd, buf, fileSize, &writelen);
	if(ret != CELL_FS_SUCCEEDED || fileSize != writelen ) {
		printf("file %s read error : 0x%x\n", filePath, ret);
		cellFsClose(fd);
		return -1;
	}

	ret = cellFsClose(fd);
	if(ret != CELL_FS_SUCCEEDED){
		printf("file %s close error : 0x%x\n", filePath, ret);
		return -1;
	}

	return 0;
}

void saveSampleData( char *cachePath )
{
	int ret ;
	void *_file_buffer = NULL;
	unsigned int fsize = 0;
	char filePath_I[CELL_FS_MAX_FS_PATH_LENGTH];
	char filePath_O[CELL_FS_MAX_FS_PATH_LENGTH];

	printf("Save sample data\n");

	const char *fileList[] = {
		SYSCACHE_1,
		SYSCACHE_2
	};
	int cnt = sizeof(fileList)/sizeof(char *);
	int i = 0 ;
	for(i=0 ; i < cnt ; i++){
		snprintf(filePath_I, CELL_FS_MAX_FS_PATH_LENGTH, "%s/%s", DATA_DIR, fileList[i]);
		ret = fileAllocLoad(filePath_I, &_file_buffer, &fsize);

		if(ret == 0){
			snprintf(filePath_O, CELL_FS_MAX_FS_PATH_LENGTH, "%s/%s", cachePath, fileList[i]);
			fileSimpleSave(filePath_O, _file_buffer, fsize);
		}
		if(_file_buffer){
			free(_file_buffer) ;
			_file_buffer = NULL ;
		}
	}
}

int checkSampleData( char *cachePath )
{
	int ret ;
	int fd;
	char filePath_I[CELL_FS_MAX_FS_PATH_LENGTH];

	printf("Check sample data\n");

	const char *fileList[] = {
		SYSCACHE_1,
		SYSCACHE_2
	};
	int cnt = sizeof(fileList)/sizeof(char *);
	int i = 0 ;
	for(i=0 ; i < cnt ; i++){
		snprintf(filePath_I, CELL_FS_MAX_FS_PATH_LENGTH, "%s/%s", cachePath, fileList[i]);
		ret = cellFsOpen(filePath_I, CELL_FS_O_RDONLY, &fd, NULL, 0);
		if(ret != CELL_FS_SUCCEEDED){
			printf("    file %s open error : 0x%x\n", filePath_I, ret);
			printf("Check NG\n");
			return -1 ;
		} else {
			printf("    file %s check OK\n", filePath_I);
			cellFsClose(fd);
		}
	}
	printf("Check OK\n");

	return 0 ;
}

int main(void) {
	CellSysCacheParam param ;
	memset(&param, 0x00 , sizeof(CellSysCacheParam)) ;
	strncpy(param.cacheId, "CACHE123", sizeof(param.cacheId)) ;

	int ret = cellSysCacheMount( &param ) ;
	printf("cellSysCacheMount() : 0x%x  sysCachePath:[%s]\n", ret, param.getCachePath);

	ret = cellSysCacheClear();
	if(ret){
		printf("cellSysCacheClear Error: 0x%08x\n", ret);
	} else {
		printf("cellSysCacheClear Ok\n");
		saveSampleData( param.getCachePath ) ;
	}
	
	checkSampleData( param.getCachePath );

	return 0;
}
