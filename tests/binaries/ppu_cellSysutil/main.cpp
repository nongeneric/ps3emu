#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sysutil/sysutil_gamecontent.h>
#include <sysutil/sysutil_common.h>
#include <sysutil/sysutil_sysparam.h>
#include <cell/sysmodule.h>

#include <string>>

SYS_PROCESS_PARAM(1001, 0x10000)

int main(void) {
	cellSysmoduleInitialize();
	cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL);

	int val;
	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_LANG, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_LANG = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_INTERNET_BROWSER_START_RESTRICT, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_INTERNET_BROWSER_START_RESTRICT = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF = %d\n", val);

	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_MAGNETOMETER, &val);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_MAGNETOMETER = %d\n", val);

	char buf[CELL_SYSUTIL_SYSTEMPARAM_NICKNAME_SIZE];
	cellSysutilGetSystemParamString(CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME, buf, CELL_SYSUTIL_SYSTEMPARAM_NICKNAME_SIZE);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME = %s\n", buf);

	cellSysutilGetSystemParamString(CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME, buf, CELL_SYSUTIL_SYSTEMPARAM_NICKNAME_SIZE);
	printf("CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME = %s\n", buf);

	return 0;
}
