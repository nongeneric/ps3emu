#include "SPUGroupManager.h"
#include "ps3emu/state.h"
#include "ps3emu/Process.h"
#include "ps3emu/log.h"

#define SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT            0x0001
#define SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT      0x0002
#define SYS_SPU_THREAD_GROUP_JOIN_TERMINATED            0x0004

bool anyRunning(ThreadGroup* group) {
    for (auto id : group->threads) {
        if (group->causes[id] == SPUThreadExitCause::StillRunning) {
            return true;
        }
    }
    return false;
}

bool anyStopped(ThreadGroup* group) {
    for (auto id : group->threads) {
        if (group->causes[id] != SPUThreadExitCause::StillRunning) {
            return true;
        }
    }
    return false;
}

std::string printGroup(ThreadGroup* group) {
    std::string threads;
    for (auto th : group->threads) {
        threads += sformat("{:x} ", th->getId());
    }
    return sformat("{} [p{}] {}", group->name, group->priority, threads);
}

template <class T>
std::string printGroups(T& groups) {
    std::string res;
    for (auto group : groups) {
        res += printGroup(group) + " ";
    }
    return res;
}

void SPUGroupManager::add(ThreadGroup* group) {
    INFO(sync) << sformat("SPUGroupManager: add ({})", group->name);
    auto lock = boost::lock_guard(_mutex);
    _initialized.push_back(group);
    INFO(sync) <<  printGroups(_initialized);
}

void SPUGroupManager::destroy(ThreadGroup* group) {
    INFO(sync) << sformat("SPUGroupManager: destroy ({})", group->name);
    auto lock = boost::lock_guard(_mutex);
    auto it = std::find(begin(_initialized), end(_initialized), group);
    EMU_ASSERT(it != end(_initialized));
    _initialized.erase(it);
}

void SPUGroupManager::start(ThreadGroup* group) {
    INFO(sync) << sformat("SPUGroupManager: start ({})", group->name);
    auto lock = boost::lock_guard(_mutex);
    for (auto th : group->threads) {
        group->initializers[th]();
        group->causes[th] = SPUThreadExitCause::StillRunning;
        th->run(true);
    }
    auto it = std::find(begin(_initialized), end(_initialized), group);
    EMU_ASSERT(it != end(_initialized));
    _initialized.erase(it);
    _ready.insert(group);
    dispatch();
}

int32_t SPUGroupManager::suspend(ThreadGroup* group) {
    INFO(sync) << sformat("SPUGroupManager: suspend ({})", group->name);
    auto lock = boost::lock_guard(_mutex);
    if (std::find(begin(_suspended), end(_suspended), group) != end(_suspended)) {
        return 0;
    }
    auto ready = std::find(begin(_ready), end(_ready), group);
    auto running = std::find(begin(_running), end(_running), group);
    if (ready == end(_ready) && running == end(_running)) {
        return CELL_ESTAT;
    }
    if (anyStopped(group))
        return 0;
    if (running != end(_running)) {
        for (auto th : group->threads) {
            th->suspend();
        }
        _running.erase(running);
    } else {
        _ready.erase(ready);
    }
    _suspended.push_back(group);
    dispatch();
    return 0;
}

int32_t SPUGroupManager::resume(ThreadGroup* group) {
    INFO(sync) << sformat("SPUGroupManager: resume ({})", group->name);
    auto lock = boost::lock_guard(_mutex);
    auto it = std::find(begin(_suspended), end(_suspended), group);
    if (it == end(_suspended))
        return CELL_ESTAT;
    _suspended.erase(it);
    _ready.insert(group);
    dispatch();
    return 0;
}

std::tuple<uint32_t, uint32_t> SPUGroupManager::join(ThreadGroup* group) {
    INFO(sync) << sformat("SPUGroupManager: join started ({})", group->name);
    {
        auto lock = boost::lock_guard(_dbgJoinMutex);
        _dbgJoin.push_back(group);
    }
    
    bool groupExit = false;
    bool threadExit = true;
    bool groupTerminate = false;
    int32_t terminateStatus;
    int32_t groupExitStatus;
    for (auto th : group->threads) {
        auto info = th->join();
        groupExit |= info.cause == SPUThreadExitCause::GroupExit;
        groupTerminate |= info.cause == SPUThreadExitCause::GroupTerminate;
        threadExit &= info.cause == SPUThreadExitCause::Exit;
        if (groupTerminate) {
            terminateStatus = info.status;
        }
        if (groupExit) {
            groupExitStatus = info.status;
        }
        group->errorCodes[th] = info.status;
    }
    {
        auto lock = boost::lock_guard(_mutex);
        EMU_ASSERT(std::find(begin(_initialized), end(_initialized), group) != end(_initialized));
    }
    {
        auto lock = boost::lock_guard(_dbgJoinMutex);
        auto it = std::find(begin(_dbgJoin), end(_dbgJoin), group);
        EMU_ASSERT(it != end(_dbgJoin));
        _dbgJoin.erase(it);
    }
    std::tuple<uint32_t, uint32_t> res;
    if (groupTerminate) {
        res = {SYS_SPU_THREAD_GROUP_JOIN_TERMINATED, terminateStatus};
    } else if (groupExit) {
        res = {SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT, groupExitStatus};
    } else if (threadExit) {
        res = {SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT, 0};
    } else {
        res = {0, 0};
    }
    INFO(sync) << sformat(
        "SPUGroupManager: join finished {} {} {:x}",
        group->name,
        std::get<0>(res) == SYS_SPU_THREAD_GROUP_JOIN_TERMINATED
            ? "terminated"
            : std::get<0>(res) == SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT ? "group_exit"
                                                                       : "regular_exit",
        std::get<1>(res));
    return res;
}

void SPUGroupManager::dispatch() {
    INFO(sync) << sformat("{}{}\n{}{}\n{}{}\n{}{}",
                              "SPUGroupManager(0), initialized: ", printGroups(_initialized),
                              "SPUGroupManager(0), ready: ", printGroups(_ready),
                              "SPUGroupManager(0), running: ", printGroups(_running),
                              "SPUGroupManager(0), suspended: ", printGroups(_suspended));
    std::vector<ThreadGroup*> vec;
    for (auto th : _running) {
        _ready.insert(th);
    }
    _running.clear();
    
    auto left = 6;
    for (auto group : _ready) {
        auto size = (int)group->threads.size();
        if (left - size >= 0) {
            _running.push_back(group);
            left -= size;
        }
    }
    for (auto group : _running) {
        auto it = _ready.find(group);
        _ready.erase(it);
    }
    for (auto group : _ready) {
        for (auto th : group->threads) {
            th->suspend();
        }
    }
    for (auto group : _running) {
        for (auto th : group->threads) {
            th->resume();
        }
    }
    INFO(sync) << sformat("{}{}\n{}{}\n{}{}\n{}{}",
                              "SPUGroupManager(0), initialized: ", printGroups(_initialized),
                              "SPUGroupManager(0), ready: ", printGroups(_ready),
                              "SPUGroupManager(0), running: ", printGroups(_running),
                              "SPUGroupManager(0), suspended: ", printGroups(_suspended));
}

void SPUGroupManager::notifyThreadStopped(SPUThread* thread, SPUThreadExitCause cause) {
    INFO(sync) << sformat("SPUGroupManager: thread stopped {:x} {}",
                              thread->getId(),
                              to_string(cause));
    auto lock = boost::lock_guard(_mutex);
    auto group = thread->group();

    // raw thread
    if (!group)
        return;

    group->causes[thread] = cause;
    if (!anyRunning(group)) {
        auto running = std::find(begin(_running), end(_running), group);
        // the group might have been preempted by another group with higher priority
        // but due to the lack of synchronization between start/notify
        // this group has been moved to ready before it has been moved to initialized
        auto ready = _ready.find(group);
        auto suspended = std::find(begin(_suspended), end(_suspended), group);
        EMU_ASSERT(running != end(_running) || ready != end(_ready) || suspended != end(_suspended));
        if (running != end(_running)) {
            _running.erase(running);
        }
        if (suspended != end(_suspended)) {
            _suspended.erase(suspended);
        }
        if (ready != end(_ready)) {
            _ready.erase(ready);
        }
        _initialized.push_back(group);
        dispatch();
    }
}

std::string SPUGroupManager::dbgDumpGroups() {
    auto lock = boost::lock_guard(_mutex);
    return sformat("{}: {}\n{}: {}\n{}: {}\n{}: {}\n{}: {}",
                     "initialized", printGroups(_initialized),
                     "ready", printGroups(_ready),
                     "running", printGroups(_running),
                     "suspended", printGroups(_suspended),
                     "join", printGroups(_dbgJoin));
}
