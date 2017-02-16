#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

void test_mutex() {
	int res;
	sys_mutex_attribute_t attr;
	sys_mutex_attribute_initialize(attr);
	sys_mutex_t mutex;
	sys_mutex_create(&mutex, &attr);

	printf("create RECURSIVE LOCK\n");
	res = sys_mutex_create(0, &attr);
	printf("%x\n", res);
	res = sys_mutex_create(&mutex, 0);
	printf("%x\n", res);
	res = sys_mutex_create(0, 0);
	printf("%x\n", res);

	attr.attr_recursive = SYS_SYNC_RECURSIVE; 
	res = sys_mutex_create(&mutex, &attr);
	printf("%x\n", res);

	printf("locking\n");
	res = sys_mutex_lock(mutex, 0);
	printf("%x\n", res);
	res = sys_mutex_lock(mutex, 0);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_mutex_destroy(0);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);

	printf("unlocking\n");
	res = sys_mutex_unlock(0);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);




	printf("create NOT_RECURSIVE LOCK\n");
	res = sys_mutex_create(0, &attr);
	printf("%x\n", res);
	res = sys_mutex_create(&mutex, 0);
	printf("%x\n", res);
	res = sys_mutex_create(0, 0);
	printf("%x\n", res);

	attr.attr_recursive = SYS_SYNC_NOT_RECURSIVE; 
	res = sys_mutex_create(&mutex, &attr);
	printf("%x\n", res);

	printf("locking\n");
	res = sys_mutex_lock(mutex, 0);
	printf("%x\n", res);
	res = sys_mutex_lock(mutex, 0);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_mutex_destroy(0);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);

	printf("unlocking\n");
	res = sys_mutex_unlock(0);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);



	printf("create RECURSIVE TRYLOCK\n");
	res = sys_mutex_create(0, &attr);
	printf("%x\n", res);
	res = sys_mutex_create(&mutex, 0);
	printf("%x\n", res);
	res = sys_mutex_create(0, 0);
	printf("%x\n", res);

	attr.attr_recursive = SYS_SYNC_RECURSIVE; 
	res = sys_mutex_create(&mutex, &attr);
	printf("%x\n", res);

	printf("locking\n");
	res = sys_mutex_trylock(mutex);
	printf("%x\n", res);
	res = sys_mutex_trylock(mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_mutex_destroy(0);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);

	printf("unlocking\n");
	res = sys_mutex_unlock(0);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);




	printf("create NOT_RECURSIVE TRYLOCK\n");
	res = sys_mutex_create(0, &attr);
	printf("%x\n", res);
	res = sys_mutex_create(&mutex, 0);
	printf("%x\n", res);
	res = sys_mutex_create(0, 0);
	printf("%x\n", res);

	attr.attr_recursive = SYS_SYNC_NOT_RECURSIVE; 
	res = sys_mutex_create(&mutex, &attr);
	printf("%x\n", res);

	printf("locking\n");
	res = sys_mutex_trylock(mutex);
	printf("%x\n", res);
	res = sys_mutex_trylock(mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_mutex_destroy(0);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);

	printf("unlocking\n");
	res = sys_mutex_unlock(0);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);
	res = sys_mutex_unlock(mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
	res = sys_mutex_destroy(mutex);
	printf("%x\n", res);
}

void test_lwmutex() {
	int res;
	sys_lwmutex_attribute_t attr;
	sys_lwmutex_attribute_initialize(attr);
	sys_lwmutex_t mutex;
	sys_lwmutex_create(&mutex, &attr);

	printf("create LW RECURSIVE LOCK\n");
	attr.attr_recursive = SYS_SYNC_RECURSIVE; 
	res = sys_lwmutex_create(&mutex, &attr);
	printf("%x\n", res);

	printf("locking\n");
	res = sys_lwmutex_lock(&mutex, 0);
	printf("%x\n", res);
	res = sys_lwmutex_lock(&mutex, 0);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);

	printf("unlocking\n");
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);




	printf("create LW NOT_RECURSIVE LOCK\n");
	attr.attr_recursive = SYS_SYNC_NOT_RECURSIVE; 
	res = sys_lwmutex_create(&mutex, &attr);
	printf("%x\n", res);

	printf("locking\n");
	res = sys_lwmutex_lock(&mutex, 0);
	printf("%x\n", res);
	res = sys_lwmutex_lock(&mutex, 0);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);

	printf("unlocking\n");
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);



	printf("create LW RECURSIVE TRYLOCK\n");
	attr.attr_recursive = SYS_SYNC_RECURSIVE; 
	res = sys_lwmutex_create(&mutex, &attr);
	printf("%x\n", res);

	printf("locking\n");
	res = sys_lwmutex_trylock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_trylock(&mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);

	printf("unlocking\n");
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);




	printf("create LW NOT_RECURSIVE TRYLOCK\n");
	attr.attr_recursive = SYS_SYNC_NOT_RECURSIVE; 
	res = sys_lwmutex_create(&mutex, &attr);
	printf("%x\n", res);

	printf("locking\n");
	res = sys_lwmutex_trylock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_trylock(&mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);

	printf("unlocking\n");
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_unlock(&mutex);
	printf("%x\n", res);

	printf("destroying\n");
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
	res = sys_lwmutex_destroy(&mutex);
	printf("%x\n", res);
}

int main(void) {
	test_mutex();
	test_lwmutex();
	printf("done\n");
}
