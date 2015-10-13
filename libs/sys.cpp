#include "../ps3emu/utils.h"
#include "sys.h"
#include <time.h>
#include <stdio.h>
#include <stdexcept>
#include "../ps3emu/PPU.h"
#include <boost/chrono.hpp>
#include <boost/log/trivial.hpp>

using namespace boost::chrono;

high_resolution_clock::time_point system_start;

void init_sys_lib() {
    system_start = high_resolution_clock::now();
}

void sys_initialize_tls(uint64_t undef, uint32_t unk1, uint32_t unk2) {
    
}

int sys_lwmutex_create(sys_lwmutex_t* mutex_id, sys_lwmutex_attribute_t* attr) {
    return CELL_OK;
}

int sys_lwmutex_destroy(sys_lwmutex_t* lwmutex_id) {
    return CELL_OK;
}

int sys_lwmutex_lock(sys_lwmutex_t* lwmutex_id, usecond_t timeout) {
    return CELL_OK;
}

int sys_lwmutex_unlock(sys_lwmutex_t* lwmutex_id) {
    return CELL_OK;
}

int sys_memory_get_user_memory_size(sys_memory_info_t* mem_info) {
    mem_info->available_user_memory = 256 * (1 << 20);
    mem_info->total_user_memory = 512 * (1 << 20);
    return CELL_OK;
}

system_time_t sys_time_get_system_time() {
    auto diff = high_resolution_clock::now() - system_start;
    auto elapsed = duration_cast<microseconds>(diff);
    return elapsed.count();
}

int _sys_process_atexitspawn() {
    return CELL_OK;
}

int _sys_process_at_Exitspawn() {
    return CELL_OK;
}

int sys_ppu_thread_get_id(sys_ppu_thread_t* thread_id) {
    *thread_id = 7;
    return CELL_OK;
}

#define SYS_TTYP_MAX  (0x10)

#define SYS_TTYP0     (0)
#define SYS_TTYP1     (1)
#define SYS_TTYP2     (2)
#define SYS_TTYP3     (3)
#define SYS_TTYP4     (4)
#define SYS_TTYP5     (5)
#define SYS_TTYP6     (6)
#define SYS_TTYP7     (7)
#define SYS_TTYP8     (8)
#define SYS_TTYP9     (9)
#define SYS_TTYP10   (10)
#define SYS_TTYP11   (11)
#define SYS_TTYP12   (12)
#define SYS_TTYP13   (13)
#define SYS_TTYP14   (14)
#define SYS_TTYP15   (15)

#define  SYS_TTYP_PPU_STDIN    (SYS_TTYP0)
#define  SYS_TTYP_PPU_STDOUT   (SYS_TTYP0)
#define  SYS_TTYP_PPU_STDERR   (SYS_TTYP1)
#define  SYS_TTYP_SPU_STDOUT   (SYS_TTYP2)
#define  SYS_TTYP_USER1        (SYS_TTYP3)
#define  SYS_TTYP_USER2        (SYS_TTYP4)
#define  SYS_TTYP_USER3        (SYS_TTYP5)
#define  SYS_TTYP_USER4        (SYS_TTYP6)
#define  SYS_TTYP_USER5        (SYS_TTYP7)
#define  SYS_TTYP_USER6        (SYS_TTYP8)
#define  SYS_TTYP_USER7        (SYS_TTYP9)
#define  SYS_TTYP_USER8        (SYS_TTYP10)
#define  SYS_TTYP_USER9        (SYS_TTYP11)
#define  SYS_TTYP_USER10       (SYS_TTYP12)
#define  SYS_TTYP_USER11       (SYS_TTYP13)
#define  SYS_TTYP_USER12       (SYS_TTYP14)
#define  SYS_TTYP_USER13       (SYS_TTYP15)

int sys_tty_write(unsigned int ch, const void* buf, unsigned int len, unsigned int* pwritelen) {
    auto asChar = (const char*)buf;
    if (ch == SYS_TTYP_PPU_STDOUT) {
        fwrite(buf, 1, len, stdout);
        fflush(stdout);
        BOOST_LOG_TRIVIAL(trace) << "stdout: " << std::string(asChar, asChar + len);
        return CELL_OK;
    }
    if (ch == SYS_TTYP_PPU_STDERR) {
        fwrite(buf, 1, len, stderr);
        fflush(stderr);
        BOOST_LOG_TRIVIAL(trace) << "stderr: " << std::string(asChar, asChar + len);
        return CELL_OK;
    }
    throw std::runtime_error(ssnprintf("unknown channel %d", ch));
}

int sys_dbg_set_mask_to_ppu_exception_handler(uint64_t mask, uint64_t flags) {
    return CELL_OK;
}

int sys_prx_exitspawn_with_level(uint64_t level) {
    return CELL_OK;
}

int sys_memory_allocate(size_t size, uint64_t flags, sys_addr_t* alloc_addr, PPU* ppu) {
    *alloc_addr = ppu->malloc(size);
    return CELL_OK;
}

int sys_timer_usleep(usecond_t sleep_time) {
    boost::this_thread::sleep_for( microseconds(sleep_time) );
    return CELL_OK;
}
