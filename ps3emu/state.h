#pragma once

class Process;
class MainMemory;
class InternalMemoryManager;
class ContentManager;
class Rsx;
class ELFLoader;

struct g_state_t {
    g_state_t();
    Process* proc;
    MainMemory* mm;
    InternalMemoryManager* memalloc;
    ContentManager* content;
    Rsx* rsx;
    ELFLoader* elf;
};

extern g_state_t g_state;
