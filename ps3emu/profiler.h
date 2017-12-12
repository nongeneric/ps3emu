#pragma once

#ifdef VTUNE_ENABLED
#include <ittnotify.h>
#else
#define __itt_thread_set_name(a)
#define __itt_domain_create(a) nullptr
#define __itt_pause()
#define __itt_resume()
#define __itt_detach()
#define __itt_task_begin(a, b, c, d)
#define __itt_task_end(a)
#define __itt_string_handle_create(a) nullptr
#define __itt_null __itt_id()
#define __itt_frame_begin_v3(a, b)
#define __itt_frame_end_v3(a, b)
#define __itt_sync_create(a, b, c, d)
#define __itt_sync_destroy(a)
#define __itt_sync_prepare(a)
#define __itt_sync_cancel(a)
#define __itt_sync_acquired(a)
#define __itt_sync_releasing(a)
#define __itt_event_create(a, b) { }
#define __itt_event_start(a)
#define __itt_event_end(a)
struct __itt_domain { };
struct __itt_string_handle { };
struct __itt_id { };
struct __itt_event { };
#endif

extern __itt_domain* g_profiler_process_domain;
