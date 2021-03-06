#include "ContentManager.h"

#include "ps3emu/Config.h"
#include "ps3emu/state.h"
#include "ps3emu/utils.h"
#include <filesystem>
#include <fstream>
#include <assert.h>
#include <map>

using namespace std::filesystem;

enum class MountPoint {
    GameData,
    SystemCache,
    MemoryStick,
    Usb,
    Bluray,
    HostHome,
    HostAbsolute,
    DevFlash,
    Relative
};

MountPoint splitPathImpl(const char* path, const char** point, const char** relative) {
#define check(str, type) \
    if (memcmp(str, path, strlen(str)) == 0) { \
        *point = str; \
        *relative = path + strlen(str); \
        while (**relative == '/') \
            (*relative)++; \
        return type; \
    }
    check("/dev_hdd0", MountPoint::GameData);
    check("/dev_hdd1", MountPoint::SystemCache);
    check("/dev_ms", MountPoint::MemoryStick);
    check("/dev_usb", MountPoint::Usb);
    check("/dev_bdvd", MountPoint::Bluray);
    check("/app_home", MountPoint::HostHome);
    check("/host_root", MountPoint::HostAbsolute);
    check("/dev_flash", MountPoint::DevFlash);
    check("", MountPoint::Relative);
#undef check
    throw std::runtime_error(sformat("illegal mount point {}", path));
}

void ContentManager::setElfPath(std::string_view path) {
    _elfPath = absolute(begin(path)).string();
}

std::string ContentManager::usrDir() {
    return contentDir() + "/USRDIR";
}

std::string ContentManager::contentDir() {
    return "/dev_hdd0";
}

std::string ContentManager::cacheDir() {
    auto path = "/dev_hdd1";
    create_directories(toHost(path));
    return path;
}

std::string ContentManager::toHost(std::string_view path) {
    const char* point;
    const char* relative;
    auto type = splitPathImpl(begin(path), &point, &relative);
    if (type == MountPoint::HostAbsolute) {
        relative += 3;
    }
    std::filesystem::path exe(_elfPath);
    auto elfdir = exe.parent_path();
    auto approot = elfdir.parent_path();
    switch (type) {
        case MountPoint::Relative:
        case MountPoint::HostHome: return absolute(elfdir / "app_home" / relative).string();
        case MountPoint::HostAbsolute: return absolute(elfdir / "host_root" / relative).string();
        case MountPoint::Bluray: return absolute(approot / ".." / relative).string();
        case MountPoint::GameData: return absolute(approot / relative).string();
        case MountPoint::SystemCache: return absolute(approot / "sys_cache" / relative).string();
        case MountPoint::DevFlash: return absolute(std::filesystem::path(g_state.config->prxStorePath) / relative).string();
        default: throw std::runtime_error("unknown mount point");
    }
}

struct sfo_header {
    uint32_t magic;
    uint32_t version;
    uint32_t key_table_start;
    uint32_t data_table_start;
    uint32_t table_entries;
};
static_assert(sizeof(sfo_header) == 20, "");

enum class entry_fmt_t : uint16_t {
    utf8_not_null_terminated = 0x304,
    utf8_null_terminated = 0x204,
    integer = 0x404
};

struct sfo_index_table_entry {
    uint16_t key_offset;
    entry_fmt_t data_fmt;
    uint32_t data_len;
    uint32_t data_max_len;
    uint32_t data_offset;
};
static_assert(sizeof(sfo_index_table_entry) == 16, "");

const std::vector<SFOEntry>& ContentManager::sfo() {
    if (!_sfo.empty())
        return _sfo;

    auto sfoPath = path(toHost(contentDir())) / "PARAM.SFO";
    std::ifstream f(sfoPath.string());
    if (!f.is_open())
        return _sfo;
    std::vector<char> vec(file_size(sfoPath));
    f.read(&vec[0], vec.size());
    auto header = (sfo_header*)&vec[0];
    if (header->magic != 0x46535000)
        throw std::runtime_error("corrupt PARAM.SFO");
    
    std::map<std::string, uint32_t> knownIds = {
        { "TITLE", CELL_GAME_PARAMID_TITLE },
        { "TITLE_DEFAULT", CELL_GAME_PARAMID_TITLE_DEFAULT },
        { "TITLE_JAPANESE", CELL_GAME_PARAMID_TITLE_JAPANESE },
        { "TITLE_ENGLISH", CELL_GAME_PARAMID_TITLE_ENGLISH },
        { "TITLE_FRENCH", CELL_GAME_PARAMID_TITLE_FRENCH },
        { "TITLE_SPANISH", CELL_GAME_PARAMID_TITLE_SPANISH },
        { "TITLE_GERMAN", CELL_GAME_PARAMID_TITLE_GERMAN },
        { "TITLE_ITALIAN", CELL_GAME_PARAMID_TITLE_ITALIAN },
        { "TITLE_DUTCH", CELL_GAME_PARAMID_TITLE_DUTCH },
        { "TITLE_PORTUGUESE", CELL_GAME_PARAMID_TITLE_PORTUGUESE },
        { "TITLE_RUSSIAN", CELL_GAME_PARAMID_TITLE_RUSSIAN },
        { "TITLE_KOREAN", CELL_GAME_PARAMID_TITLE_KOREAN },
        { "TITLE_CHINESE_T", CELL_GAME_PARAMID_TITLE_CHINESE_T },
        { "TITLE_CHINESE_S", CELL_GAME_PARAMID_TITLE_CHINESE_S },
        { "TITLE_FINNISH", CELL_GAME_PARAMID_TITLE_FINNISH },
        { "TITLE_SWEDISH", CELL_GAME_PARAMID_TITLE_SWEDISH },
        { "TITLE_DANISH", CELL_GAME_PARAMID_TITLE_DANISH },
        { "TITLE_NORWEGIAN", CELL_GAME_PARAMID_TITLE_NORWEGIAN },
        { "TITLE_POLISH", CELL_GAME_PARAMID_TITLE_POLISH },
        { "TITLE_PORTUGUESE_BRAZIL", CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL },
        { "TITLE_ENGLISH_UK", CELL_GAME_PARAMID_TITLE_ENGLISH_UK },
        { "TITLE_ID", CELL_GAME_PARAMID_TITLE_ID },
        { "VERSION", CELL_GAME_PARAMID_VERSION },
        { "PARENTAL_LEVEL", CELL_GAME_PARAMID_PARENTAL_LEVEL },
        { "RESOLUTION", CELL_GAME_PARAMID_RESOLUTION },
        { "SOUND_FORMAT", CELL_GAME_PARAMID_SOUND_FORMAT },
        { "PS3_SYSTEM_VER", CELL_GAME_PARAMID_PS3_SYSTEM_VER },
        { "APP_VER", CELL_GAME_PARAMID_APP_VER }
    };
    
    _sfo.resize(header->table_entries);
    auto keys = &vec[header->key_table_start];
    auto data = &vec[header->data_table_start];
    auto fileEntry = (sfo_index_table_entry*)&vec[sizeof(sfo_header)];
    for (auto& entry : _sfo) {
        entry.key = std::string(&keys[fileEntry->key_offset]);
        auto knownId = knownIds.find(entry.key);
        entry.id = knownId != end(knownIds) ? knownId->second : -1;
        if (fileEntry->data_fmt == entry_fmt_t::integer) {
            assert(fileEntry->data_len == 4);
            assert(fileEntry->data_max_len == 4);
            entry.data = *(uint32_t*)&data[fileEntry->data_offset];
        } else {
            auto beg = &data[fileEntry->data_offset];
            entry.data = std::string(beg, beg + fileEntry->data_len);
        }
        fileEntry++;
    }
    return _sfo;
}
