#pragma once

#include "ps3emu/int.h"
#include "ps3emu/spu/SPUThread.h"

#include <vector>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <memory>
#include <boost/thread/recursive_mutex.hpp>

#define CELL_ESTAT -2147418097

struct ThreadGroup {
    std::vector<SPUThread*> threads;
    std::string name;
    uint32_t priority;
    std::map<SPUThread*, int32_t> errorCodes;
    std::map<SPUThread*, SPUThreadExitCause> causes;
    std::map<SPUThread*, std::function<void()>> initializers;
    uint32_t id;
};

struct PriorityComparer {
    inline bool operator()(ThreadGroup* x, ThreadGroup* y) const {
        return std::tie(x->priority, x->id) < std::tie(y->priority, y->id);
    }
};

class SPUGroupManager {
    boost::mutex _mutex;
    std::set<ThreadGroup*, PriorityComparer> _ready;
    std::vector<ThreadGroup*> _initialized;
    std::vector<ThreadGroup*> _suspended;
    std::vector<ThreadGroup*> _running;
    boost::mutex _dbgJoinMutex;
    std::vector<ThreadGroup*> _dbgJoin;
    
public:
    void add(ThreadGroup* group);
    void destroy(ThreadGroup* group);
    void start(ThreadGroup* group);
    int32_t suspend(ThreadGroup* group);
    int32_t resume(ThreadGroup* group);
    std::tuple<uint32_t, uint32_t> join(ThreadGroup* group);
    void dispatch();
    void notifyThreadStopped(SPUThread* thread, SPUThreadExitCause cause);
    std::string dbgDumpGroups();
};
