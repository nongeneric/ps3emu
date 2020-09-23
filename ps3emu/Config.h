#pragma once

#include <string>
#include <vector>
#include <stdint.h>

struct SysPrxInfo {
    std::string name;
    bool loadx86;
    bool loadx86spu;
    uint32_t imageBase;
    uint32_t size;
    bool x86trace;
};

class Config {
    std::string _configPath;

public:
    Config();
    std::vector<std::string> x86Paths;
    std::string prxStorePath;
    std::string configDirPath;
    std::vector<SysPrxInfo> sysPrxInfos;
    bool captureAudio = false;
    bool fullscreen = false;
    int fpsCap = 30;

    void save();
};

std::string getTestOutputDir();
