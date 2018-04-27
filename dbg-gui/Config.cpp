#include "Config.h"

#include <cereal/archives/json.hpp>
#include <filesystem>
#include <fstream>
#include <assert.h>
#include <cstdlib>

namespace {
 
    std::filesystem::path configPath() {
        std::filesystem::path home = std::getenv("HOME");
        return home / ".ps3emu/debug.json";
    }

}

DbgConfig& DbgConfigLoader::config() {
    return _config;
}

void DbgConfigLoader::load() {
    auto path = configPath();
    if (std::filesystem::exists(path)) {
        std::ifstream f(path.string());
        assert(f.is_open());
        cereal::JSONInputArchive archive(f);
        archive(_config);
    }
}

void DbgConfigLoader::save() {
    std::ofstream f(configPath().string());
    assert(f.is_open());
    cereal::JSONOutputArchive archive(f);
    archive(_config);
}

DbgConfigLoader g_config;
