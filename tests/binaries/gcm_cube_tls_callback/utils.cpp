#include "utils.h"

int32_t
utilMonitorInit(UtilMonitor* pMonitor)
{
	sys_mutex_attribute_t mutexAttr;
	sys_mutex_attribute_initialize(mutexAttr);
	int32_t ret = sys_mutex_create(&pMonitor->mutex, &mutexAttr);
	if(ret < CELL_OK){
		return ret;
	}
	sys_cond_attribute_t condAttr;
	sys_cond_attribute_initialize(condAttr);
	ret = sys_cond_create(&pMonitor->cond,
						  pMonitor->mutex,
						  &condAttr);
	if(ret < CELL_OK){
		(void)sys_mutex_destroy(pMonitor->mutex);
		return ret;
	}
	return ret;
}

int32_t
utilMonitorFin(UtilMonitor* pMonitor)
{
	int32_t ret;
	ret = sys_cond_destroy(pMonitor->cond);
	if(ret < CELL_OK){
		
	}
	ret = sys_mutex_destroy(pMonitor->mutex);
	if(ret < CELL_OK){
		
	}
	return ret;
}

int32_t
utilMonitorLock(UtilMonitor* pMonitor, usecond_t timeout)
{
	
	int32_t ret;
	ret = sys_mutex_lock(pMonitor->mutex, timeout);
	if(ret < CELL_OK && (int)ETIMEDOUT != ret){
		
	}
	return ret;
}

int32_t
utilMonitorUnlock(UtilMonitor* pMonitor)
{
	
	int32_t ret;
	ret = sys_mutex_unlock(pMonitor->mutex);
	if(ret < CELL_OK){
		
	}
	return ret;
}

int32_t
utilMonitorWait(UtilMonitor* pMonitor, usecond_t timeout)
{
	int32_t ret;
	ret = sys_cond_wait(pMonitor->cond, timeout);
	if(ret < CELL_OK && (int)ETIMEDOUT != ret){
		
	}
	return ret;
}

int32_t
utilMonitorSignal(UtilMonitor* pMonitor)
{
	int32_t ret;
	ret = sys_cond_signal(pMonitor->cond);
	if(ret < CELL_OK){
		
	}
	return ret;
}
