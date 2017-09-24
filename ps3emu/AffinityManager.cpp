#include "AffinityManager.h"

#include "ps3emu/log.h"

void assignAffinity(pthread_t thread, AffinityGroup group) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    if (group == AffinityGroup::PPUEmu) {
        CPU_SET(0, &cpuset);
        CPU_SET(4, &cpuset);
    } else if (group == AffinityGroup::SPUEmu) {
        CPU_SET(1, &cpuset);
        CPU_SET(5, &cpuset);
        CPU_SET(2, &cpuset);
        CPU_SET(6, &cpuset);
    } else if (group == AffinityGroup::PPUHost) {
        CPU_SET(3, &cpuset);
        CPU_SET(7, &cpuset);
    }

    auto s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0) {
        ERROR(libs) << "can't assign thread affinity";
        exit(1);
    }
}
