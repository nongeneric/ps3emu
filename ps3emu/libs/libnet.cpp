#include "libnet.h"

namespace {
    bool init = false;
}

int32_t sys_net_initialize_network_ex(const sys_net_initialize_parameter_t* param) {
    if (param->flags & SYS_NET_INIT_ERROR_CHECK)
        return init ? SYS_NET_ERROR_EBUSY : CELL_OK;
    return CELL_OK;
}

int32_t sys_net_free_thread_context(uint32_t tid, int flags) {
    return CELL_OK;
}
