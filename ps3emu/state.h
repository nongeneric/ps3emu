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

struct g_state_t {
    g_state_t();
    Process* proc;
    MainMemory* mm;
    InternalMemoryManager* memalloc;
    InternalMemoryManager* heapalloc;
    ContentManager* content;
    Rsx* rsx;
    ELFLoader* elf;
    EmuCallbacks* callbacks;
    Config* config;
    thread_local static PPUThread* th;
    thread_local static SPUThread* sth;
};

extern g_state_t g_state;
