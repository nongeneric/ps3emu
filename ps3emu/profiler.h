#pragma once

#include <mutex>

#ifdef VTUNE_ENABLED
#include <ittnotify.h>
#else
struct __itt_domain { };
struct __itt_string_handle { };
struct __itt_id { };
struct __itt_event { };
inline constexpr int __itt_null = 0;
inline void __itt_thread_set_name(const void*) {}
inline __itt_domain* __itt_domain_create(const void*) { return nullptr; }
inline void __itt_pause() {}
inline void __itt_resume() {}
inline void __itt_detach() {}
inline void __itt_task_begin(const void*, int, int, const void*) {}
inline void __itt_task_end(const void*) {}
inline __itt_string_handle* __itt_string_handle_create(const void*) { return nullptr; }
inline void __itt_frame_begin_v3(const void*, const void*) {}
inline void __itt_frame_end_v3(const void*, const void*) {}
inline void __itt_sync_create(const void*, const void*, const void*, const void*) {}
inline void __itt_sync_destroy(const void*) {}
inline void __itt_sync_prepare(const void*) {}
inline void __itt_sync_cancel(const void*) {}
inline void __itt_sync_acquired(const void*) {}
inline void __itt_sync_releasing(const void*) {}
inline void __itt_event_create(const void*, const void*) { }
inline void __itt_event_start(const void*) {}
inline void __itt_event_end(const void*) {}
#endif

extern __itt_domain* g_profiler_process_domain;

class VtuneTask
{
    friend class std::lock_guard<VtuneTask>;

public:
    VtuneTask() = default;
    VtuneTask(const VtuneTask& rhs) = default;
    VtuneTask& operator=(const VtuneTask& rhs) = default;
    explicit VtuneTask(const char* name);

private:
    void lock();
    void unlock();

private:
    void* m_name{};
};

using VtuneTaskGuard = std::lock_guard<VtuneTask>;