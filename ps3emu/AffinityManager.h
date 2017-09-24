#pragma once

#include <pthread.h>

enum class AffinityGroup {
    PPUEmu, SPUEmu, PPUHost
};

void assignAffinity(pthread_t thread, AffinityGroup group);
