#include "profiler.h"

__itt_domain* g_profiler_process_domain;

VtuneTask::VtuneTask(const char* name)
{
    m_name = __itt_string_handle_create(name);
    static bool domainInitialized = false;
    if (!domainInitialized)
    {
        g_profiler_process_domain = __itt_domain_create("ps3emu");
    }
}

void VtuneTask::lock()
{
    __itt_task_begin(g_profiler_process_domain, __itt_null, __itt_null, reinterpret_cast<__itt_string_handle*>(m_name));
}

void VtuneTask::unlock()
{
    __itt_task_end(g_profiler_process_domain);
}
