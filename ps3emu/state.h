#pragma once

class Process;
class MainMemory;
class InternalMemoryManager;
class ContentManager;
class Rsx;
class ELFLoader;
class PPUThread;
class SPUThread;
class EmuCallbacks;
class Config;
struct ReservationGranule;
struct ExecutionMapCollection;
class HeapMemoryAlloc;
class BBCallMap;
class SPUGroupManager;

struct g_state_t {
    g_state_t();
    Process* proc;
    MainMemory* mm;
    // internal emu memory allocator
    InternalMemoryManager* memalloc;
    // regular ps3 memory allocator
    HeapMemoryAlloc* heapalloc;
    ContentManager* content;
    Rsx* rsx;
    ELFLoader* elf;
    EmuCallbacks* callbacks;
    Config* config;
    ExecutionMapCollection* executionMaps;
    BBCallMap* bbcallMap;
    SPUGroupManager* spuGroupManager;
    thread_local static PPUThread* th;
    thread_local static SPUThread* sth;
    thread_local static bool rewriter_ncall;
    thread_local static ReservationGranule* granule;
};

extern g_state_t g_state;
