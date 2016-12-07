#pragma once

#include <string>
#include <vector>
#include <stdint.h>

struct SysPrxInfo {
    std::string name;
    std::string x86name;
    bool loadx86;
    uint32_t imageBase;
    uint32_t size;
    bool x86trace;
};

class Config {
    std::string _configPath;
    
public:
    Config();
    std::string x86Path;
    std::string prxStorePath;
    std::string configDirPath;
    std::vector<SysPrxInfo> sysPrxInfos;
    
    void save();
};
