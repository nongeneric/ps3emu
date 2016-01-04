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

SYS_PROCESS_PARAM(1001, 0x10000)

bool dirpred(CellFsDirent& left, CellFsDirent& right) {
	return std::string(left.d_name) < std::string(right.d_name);
}

bool pred(CellFsDirectoryEntry& left, CellFsDirectoryEntry& right) {
	return std::string(left.entry_name.d_name) < std::string(right.entry_name.d_name);
}

int main(void) {
	int fd;
	int res = cellFsOpendir("/app_home/data", &fd);
	if (res != CELL_FS_SUCCEEDED) {
		printf("can't open dir\n");
		return -1;
	}

	CellFsDirent dir;
	std::vector<CellFsDirent> dirs;
	for (;;) {
	uint64_t nread;
		cellFsReaddir(fd, &dir, &nread);
		if (nread == 0)
			break;
		dirs.push_back(dir);
	}

	cellFsClosedir(fd);

	cellFsOpendir("/app_home/data", &fd);

	std::sort(dirs.begin(), dirs.end(), dirpred);

	for (int i = 0; i < dirs.size(); ++i) {
		printf("type: %d, namelen: %d, name: %s\n", dirs[i].d_type, dirs[i].d_namlen, dirs[i].d_name);
	}

	CellFsDirectoryEntry entry;
	std::vector<CellFsDirectoryEntry> entries;
	
	for (;;) {
		uint32_t count;
		res = cellFsGetDirectoryEntries(fd, &entry, sizeof(CellFsDirectoryEntry), &count);
		if (res != CELL_FS_SUCCEEDED) {
			printf("get entries failed %d\n", res);
			return -1;
		}
		if (count == 0)
			break;
		entries.push_back(entry);
	}

	std::sort(entries.begin(), entries.end(), pred);

	for (int i = 0; i < entries.size(); ++i) {
		printf("size %d, block size %d, name %s\n", 
			entries[i].attribute.st_size,
			entries[i].attribute.st_blksize,
			entries[i].entry_name.d_name);
	}

	cellFsClosedir(fd);

	return 0;
}
