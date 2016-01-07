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
#include <vector>
#include <string>
#include <algorithm>
#include <simd>
#include <cell/hash/libmd5.h>
#include <cell/hash/libsha1.h>
#include <cell/hash/libsha224.h>
#include <cell/hash/libsha256.h>
#include <cell/hash/libsha384.h>
#include <cell/hash/libsha512.h>

SYS_PROCESS_PARAM(1001, 0x10000)

void printhex(unsigned char* buf, int len) {
	for (int i = 0; i < len; ++i) {
		printf("%02x", buf[i]);
	}
}

int main(void) {
	const int len = 4100;
	char* buf = new char[len];
	for (int i = 0; i < len; ++i) {
		buf[i] = i;
	}
	
	unsigned char digest[64];

	int res;
	res = cellMd5Digest(buf, len, digest);
	printf("md5 %d ", res);
	printhex(digest, 16);
	printf("\n");

	res = cellSha1Digest(buf, len, digest);
	printf("sha1 %d ", res);
	printhex(digest, 20);
	printf("\n");

	res = cellSha224Digest(buf, len, digest);
	printf("sha224 %d ", res);
	printhex(digest, 28);
	printf("\n");

	res = cellSha256Digest(buf, len, digest);
	printf("sha256 %d ", res);
	printhex(digest, 32);
	printf("\n");

	res = cellSha384Digest(buf, len, digest);
	printf("sha384 %d ", res);
	printhex(digest, 48);
	printf("\n");

	res = cellSha512Digest(buf, len, digest);
	printf("sha512 %d ", res);
	printhex(digest, 64);
	printf("\n");

	return 0;
}
