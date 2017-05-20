#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>

typedef struct {
	sys_mutex_t	mutex;
	sys_cond_t	cond;
}UtilMonitor;

int32_t utilMonitorInit(UtilMonitor* pMonitor);
int32_t utilMonitorFin(UtilMonitor* pMonitor);
int32_t utilMonitorLock(UtilMonitor* pMonitor, usecond_t timeout);
int32_t utilMonitorUnlock(UtilMonitor* pMonitor);
int32_t utilMonitorWait(UtilMonitor* pMonitor, usecond_t timeout);
int32_t utilMonitorSignal(UtilMonitor* pMonitor);