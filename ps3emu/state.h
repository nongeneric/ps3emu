#pragma once

#include <memory>

class Process;
class MainMemory;
class InternalMemoryManager;
class ContentManager;
class Rsx;
class ELFLoader;
class PPUThread;
class SPUThread;
class IEmuCallbacks;
class Config;
struct ReservationGranule;
struct ExecutionMapCollection;
class HeapMemoryAlloc;
class BBCallMap;
class SPUGroupManager;

struct g_state_t {
    Process* proc = nullptr;
    MainMemory* mm = nullptr;
    // internal emu memory allocator
    InternalMemoryManager* memalloc = nullptr;
    // regular ps3 memory allocator
    HeapMemoryAlloc* heapalloc = nullptr;
    ContentManager* content = nullptr;
    Rsx* rsx = nullptr;
    ELFLoader* elf = nullptr;
    IEmuCallbacks* callbacks = nullptr;
    std::unique_ptr<Config> config;
    std::unique_ptr<ExecutionMapCollection> executionMaps;
    std::unique_ptr<BBCallMap> bbcallMap;
    std::unique_ptr<SPUGroupManager> spuGroupManager;
    thread_local static PPUThread* th;
    thread_local static SPUThread* sth;
    thread_local static bool rewriter_ncall;
    thread_local static ReservationGranule* granule;
    void init();
    ~g_state_t();
};

extern g_state_t g_state;
