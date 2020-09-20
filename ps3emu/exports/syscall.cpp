#include "exports.h"
#include "ps3emu/utils.h"
#include "ps3emu/libs/sys.h"
#include "ps3emu/libs/fs.h"
#include "ps3emu/libs/spu/sysSpu.h"
#include "ps3emu/libs/sync/mutex.h"
#include "ps3emu/libs/sync/cond.h"
#include "ps3emu/libs/sync/rwlock.h"
#include "ps3emu/libs/sync/queue.h"
#include "ps3emu/libs/sync/event_flag.h"
#include "ps3emu/libs/memoryContainer.h"
#include "ps3emu/libs/perf.h"
#include "ps3emu/libs/graphics/gcm.h"
#include "ps3emu/log.h"

void PPUThread::scall() {
    auto index = getGPR(11);
    //INFO(libs) << sformat("  -- scall {} at {:08x}", index, getNIP());
    switch (index) {
        case 352: wrap(sys_memory_get_user_memory_size, this); break;
        case 403: wrap(sys_tty_write, this); break;
        case 988: wrap(sys_dbg_set_mask_to_ppu_exception_handler, this); break;
        case 348: wrap(sys_memory_allocate, this); break;
        case 349: wrap(sys_memory_free, this); break;
        case 141: wrap(sys_timer_usleep, this); break;
        case 142: wrap(sys_timer_sleep, this); break;
        case 801: wrap(sys_fs_open, this); break;
        case 128: wrap(sys_event_queue_create, this); break;
        case 129: wrap(sys_event_queue_destroy, this); break;
        case 130: wrap(sys_event_queue_receive, this); break;
        case 131: wrap(sys_event_queue_tryreceive, this); break;
        case 133: wrap(sys_event_queue_drain, this); break;
        case 134: wrap(sys_event_port_create, this); break;
        case 136: wrap(sys_event_port_connect_local, this); break;
        case 138: wrap(sys_event_port_send, this); break;
        case 147: wrap(sys_time_get_timebase_frequency, this); break;
        case 49: wrap(sys_ppu_thread_get_stack_information, this); break;
        case 100: wrap(sys_mutex_create, this); break;
        case 101: wrap(sys_mutex_destroy, this); break;
        case 102: wrap(sys_mutex_lock, this); break;
        case 103: wrap(sys_mutex_trylock, this); break;
        case 104: wrap(sys_mutex_unlock, this); break;
        case 145: wrap(sys_time_get_current_time, this); break;
        case 144: wrap(sys_time_get_timezone, this); break;
        case 90: wrap(sys_semaphore_create, this); break;
        case 91: wrap(sys_semaphore_destroy, this); break;
        case 92: wrap(sys_semaphore_wait, this); break;
        case 93: wrap(sys_semaphore_trywait, this); break;
        case 94: wrap(sys_semaphore_post, this); break;
        case 114: wrap(sys_semaphore_get_value, this); break;
        case 41: wrap(sys_ppu_thread_exit, this); break;
        case 44: wrap(sys_ppu_thread_join, this); break;
        case 47: wrap(sys_ppu_thread_set_priority, this); break;
        case 48: wrap(sys_ppu_thread_get_priority, this); break;
        case 120: wrap(sys_rwlock_create, this); break;
        case 121: wrap(sys_rwlock_destroy, this); break;
        case 122: wrap(sys_rwlock_rlock, this); break;
        case 123: wrap(sys_rwlock_tryrlock, this); break;
        case 124: wrap(sys_rwlock_runlock, this); break;
        case 125: wrap(sys_rwlock_wlock, this); break;
        case 148: // duplicate
        case 126: wrap(sys_rwlock_trywlock, this); break;
        case 127: wrap(sys_rwlock_wunlock, this); break;
        case 105: wrap(sys_cond_create, this); break;
        case 106: wrap(sys_cond_destroy, this); break;
        case 107: wrap(sys_cond_wait, this); break;
        case 108: wrap(sys_cond_signal, this); break;
        case 109: wrap(sys_cond_signal_all, this); break;
        case 110: wrap(sys_cond_signal_to, this); break;
        case 182: wrap(sys_spu_thread_read_ls, this); break;
        case 169: wrap(sys_spu_initialize, this); break;
        case 170: wrap(sys_spu_thread_group_create, this); break;
        case 171: wrap(sys_spu_thread_group_destroy, this); break;
        case 172: wrap(sys_spu_thread_initialize, this); break;
        case 173: wrap(sys_spu_thread_group_start, this); break;
        case 178: wrap(sys_spu_thread_group_join, this); break;
        case 45: wrap(sys_ppu_thread_detach, this); break;
        case 165: wrap(sys_spu_thread_get_exit_status, this); break;
        case 160: wrap(sys_raw_spu_create, this); break;
        case 156: wrap(sys_spu_image_open, this); break;
        case 157: wrap(sys_spu_image_import, this); break;
        case 150: wrap(sys_raw_spu_create_interrupt_tag, this); break;
        case 151: wrap(sys_raw_spu_set_int_mask, this); break;
        case 81: wrap(sys_interrupt_tag_destroy, this); break;
        case 84: wrap(sys_interrupt_thread_establish, this); break;
        case 89: wrap(sys_interrupt_thread_disestablish, this); break;
        case 154: wrap(sys_raw_spu_get_int_stat, this); break;
        case 155: wrap(sys_spu_image_get_info, this); break;
        case 158: wrap(sys_spu_image_close, this); break;
        case 159: wrap(sys_spu_image_get_modules, this); break;
        case 163: wrap(sys_raw_spu_read_puint_mb, this); break;
        case 153: wrap(sys_raw_spu_set_int_stat, this); break;
        case 88: wrap(sys_interrupt_thread_eoi, this); break;
        case 161: wrap(sys_raw_spu_destroy, this); break;
        case 43: wrap(sys_ppu_thread_yield, this); break;
        case 818: wrap(sys_fs_lseek, this); break;
        case 809: wrap(sys_fs_fstat, this); break;
        case 802: wrap(sys_fs_read, this); break;
        case 803: wrap(sys_fs_write, this); break;
        case 804: wrap(sys_fs_close, this); break;
        case 137: wrap(sys_event_port_disconnect, this); break;
        case 135: wrap(sys_event_port_destroy, this); break;
        case 14: wrap(sys_process_is_spu_lock_line_reservation_address, this); break;
        case 191: wrap(sys_spu_thread_connect_event, this); break;
        case 193: wrap(sys_spu_thread_bind_queue, this); break;
        case 251: wrap(sys_spu_thread_group_connect_event_all_threads, this); break;
        case 1: wrap(sys_process_getpid, this); break;
        case 25: wrap(sys_process_get_sdk_version, this); break;
        case 185: wrap(sys_spu_thread_group_connect_event, this); break;
        case 186: wrap(sys_spu_thread_group_disconnect_event, this); break;
        case 30: wrap(_sys_process_get_paramsfo, this); break;
        case 52: wrap(sys_ppu_thread_create, this); break;
        case 53: wrap(sys_ppu_thread_start, this); break;
        case 22: wrap(sys_process_exit, this); break;
        case 462: wrap(sys_get_process_info, this); break;
        case 484: wrap(sys_prx_register_module, this); break;
        case 486: wrap(sys_prx_register_library, this); break;
        case 480: wrap(sys_prx_load_module, this); break;
        case 481: wrap(sys_prx_start_module, this); break;
        case 871: wrap(sys_ss_access_control_engine, this); break;
        case 465: wrap(sys_prx_load_module_list, this); break;
        case 332: wrap(sys_mmapper_allocate_shared_memory, this); break;
        case 330: wrap(sys_mmapper_allocate_address, this); break;
        case 337: wrap(sys_mmapper_search_and_map, this); break;
        case 335: wrap(sys_mmapper_unmap_shared_memory, this); break;
        case 329: wrap(sys_mmapper_free_shared_memory, this); break;
        case 494: wrap(sys_prx_get_module_list, this); break;
        case 482: wrap(sys_prx_stop_module, this); break;
        case 483: wrap(sys_prx_unload_module, this); break;
        case 496: wrap(sys_prx_get_module_id_by_name, this); break;
        case 82: wrap(sys_event_flag_create, this); break;
        case 83: wrap(sys_event_flag_destroy, this); break;
        case 85: wrap(sys_event_flag_wait, this); break;
        case 87: wrap(sys_event_flag_set, this); break;
        case 118: wrap(sys_event_flag_clear, this); break;
        case 139: wrap(sys_event_flag_get, this); break;
        case 190: wrap(sys_spu_thread_write_spu_mb, this); break;
        case 252: wrap(sys_spu_thread_group_disconnect_event_all_threads, this); break;
        case 194: wrap(sys_spu_thread_unbind_queue, this); break;
        case 192: wrap(sys_spu_thread_disconnect_event, this); break;
        case 184: wrap(sys_spu_thread_write_snr, this); break;
        case 351: wrap(sys_memory_get_page_attribute, this); break;
        case 140: wrap(sys_event_port_connect_ipc, this); break;
        case 324: wrap(sys_memory_container_create, this); break;
        case 325: wrap(sys_memory_container_destroy, this); break;
        case 343: wrap(sys_memory_container_get_size, this); break;
        case 497: wrap(sys_prx_load_module_on_memcontainer, this); break;
        case 962: wrap(sys_perf_insert_cbe_bookmark, this); break;
        case 174: wrap(sys_spu_thread_group_suspend, this); break;
        case 175: wrap(sys_spu_thread_group_resume, this); break;
        case 817: wrap(sys_fs_fcntl, this); break;
        case 831: wrap(sys_fs_truncate, this); break;
        case 812: wrap(sys_fs_rename, this); break;
        case 808: wrap(sys_fs_stat, this); break;
        case 811: wrap(sys_fs_mkdir, this); break;
        case 832: wrap(sys_fs_ftruncate, this); break;
        case 805: wrap(sys_fs_opendir, this); break;
        case 806: wrap(sys_fs_readdir, this); break;
        case 807: wrap(sys_fs_closedir, this); break;
        case 814: wrap(sys_fs_unlink, this); break;
        case 813: wrap(sys_fs_rmdir, this); break;
        case 821: wrap(sys_fs_fget_block_size, this); break;
        case 827: wrap(sys_fs_lsn_lock, this); break;
        case 828: wrap(sys_fs_lsn_unlock, this); break;
        case 825: wrap(sys_fs_lsn_get_cda_size, this); break;
        case 820: wrap(sys_fs_fsync, this); break;
        case 840: wrap(sys_fs_disk_free, this); break;
        case 822: wrap(sys_fs_fget_block_size, this); break;
        case 675: wrap(emu::Gcm::sys_rsx_device_map, this); break;
        case 668: wrap(emu::Gcm::sys_rsx_memory_allocate, this); break;
        case 670: wrap(emu::Gcm::sys_rsx_context_allocate, this); break;
        case 671: wrap(emu::Gcm::sys_rsx_context_free, this); break;
        case 669: wrap(emu::Gcm::sys_rsx_memory_free, this); break;
        case 672: wrap(emu::Gcm::sys_rsx_context_iomap, this); break;
        case 673: wrap(emu::Gcm::sys_rsx_context_iounmap, this); break;
        case 674: wrap(emu::Gcm::sys_rsx_context_attribute, this); break;
        case 677: wrap(emu::Gcm::sys_rsx_attribute, this); break;
        default: ERROR(libs) << sformat("unknown syscall {}", index); exit(1);
    }
}
