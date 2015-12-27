#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sysutil/sysutil_gamecontent.h>
#include <cell/sysmodule.h>
#include <string>
#include <sys/fs.h>

SYS_PROCESS_PARAM(1001, 0x10000)

volatile int tmp = 0;

char buf[256];
char contentdir[256];
char userdir[256];
char param[256];
CellGameContentSize size;

int main(void) {
	cellSysmoduleInitialize();
	cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_GAME);
	uint32_t type;
	uint32_t attributes;
	cellGameBootCheck(&type, &attributes, &size, buf);
	cellGameContentPermit(contentdir, userdir);

	cellGameGetParamString(CELL_GAME_PARAMID_TITLE, param, 256);
	printf("title: %s\n", param);
	printf("gamedir: %s\n", buf);
	printf("contentdir: %s\n", contentdir);
	printf("usrdir: %s\n", userdir);

	CellFsStat st;
	cellFsStat((std::string(userdir) + "/data/file").c_str(), &st);
	printf("filesize: %d\n", (uint32_t)st.st_size);

	return 0;
}
