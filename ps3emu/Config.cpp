#include "Config.h"

#include "ps3emu/utils.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using namespace std::filesystem;
using namespace nlohmann;

Config::Config() {
    auto configDirPath = path(std::getenv("HOME")) / ".ps3emu";
    create_directories(configDirPath);
    this->prxStorePath = (configDirPath / "ps3bin").string();
    this->configDirPath = configDirPath.string();
    _configPath = (configDirPath / "ps3emu.json").string();
    if (exists(_configPath)) {
        std::fstream f(_configPath, std::ios_base::in);
        json j;
        f >> j;
        auto& jprxInfos = j["sysPrxInfos"];
        for (auto it = begin(jprxInfos); it != jprxInfos.end(); ++it) {
            SysPrxInfo info;
            info.name = it.key();
            auto& jloadx86 = j["sysPrxInfos"][info.name]["loadx86"];
            info.loadx86 = !jloadx86.is_null() ? jloadx86.get<bool>() : false;
            auto& jloadx86spu = j["sysPrxInfos"][info.name]["loadx86spu"];
            info.loadx86spu = !jloadx86spu.is_null() ? jloadx86spu.get<bool>() : false;
            info.imageBase =
                std::stoi(j["sysPrxInfos"][info.name]["imageBase"].get<std::string>(), 0, 16);
            auto jsize = j["sysPrxInfos"][info.name]["size"];
            if (!jsize.is_null()) {
                info.size = std::stoi(jsize.get<std::string>(), 0, 16);
            }
            info.x86trace = j["sysPrxInfos"][info.name]["x86trace"];
            sysPrxInfos.push_back(info);
        }

        auto fullscreen = j["fullscreen"];
        this->fullscreen = fullscreen.is_null() ? false : fullscreen.get<bool>();

        auto fpsCap = j["fpsCap"];
        this->fpsCap = fpsCap.is_null() ? false : fpsCap.get<int>();
    }
}

void Config::save() {
    json j;
    for (auto& info : sysPrxInfos) {
        j["sysPrxInfos"][info.name]["imageBase"] = sformat("{:x}", info.imageBase);
        j["sysPrxInfos"][info.name]["size"] = sformat("{:x}", info.size);
        j["sysPrxInfos"][info.name]["loadx86"] = info.loadx86;
        j["sysPrxInfos"][info.name]["loadx86spu"] = info.loadx86spu;
        j["sysPrxInfos"][info.name]["x86trace"] = info.x86trace;
    }
    j["fullscreen"] = fullscreen;
    j["fpsCap"] = fpsCap;
    std::ofstream f(_configPath);
    f << j.dump(4);
}

std::string getTestOutputDir() {
    auto path = "/tmp/ps3emu_tests";
    create_directories(path);
    return path;
}
