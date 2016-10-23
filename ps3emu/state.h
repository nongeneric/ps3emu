#pragma once

class Process;
class MainMemory;
class InternalMemoryManager;
class ContentManager;
class Rsx;
class ELFLoader;
class PPUThread;
class EmuCallbacks;

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
    thread_local static PPUThread* th;
};

extern g_state_t g_state;
