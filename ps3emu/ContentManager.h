#pragma once

#include <string>
#include <string_view>
#include <boost/variant.hpp>

#include <vector>

enum {
    CELL_GAME_PARAMID_TITLE = 0,
    CELL_GAME_PARAMID_TITLE_DEFAULT,
    CELL_GAME_PARAMID_TITLE_JAPANESE,
    CELL_GAME_PARAMID_TITLE_ENGLISH,
    CELL_GAME_PARAMID_TITLE_FRENCH,
    CELL_GAME_PARAMID_TITLE_SPANISH,
    CELL_GAME_PARAMID_TITLE_GERMAN,
    CELL_GAME_PARAMID_TITLE_ITALIAN,
    CELL_GAME_PARAMID_TITLE_DUTCH,
    CELL_GAME_PARAMID_TITLE_PORTUGUESE,
    CELL_GAME_PARAMID_TITLE_RUSSIAN,
    CELL_GAME_PARAMID_TITLE_KOREAN,
    CELL_GAME_PARAMID_TITLE_CHINESE_T,
    CELL_GAME_PARAMID_TITLE_CHINESE_S,
    CELL_GAME_PARAMID_TITLE_FINNISH,
    CELL_GAME_PARAMID_TITLE_SWEDISH,
    CELL_GAME_PARAMID_TITLE_DANISH,
    CELL_GAME_PARAMID_TITLE_NORWEGIAN,
    CELL_GAME_PARAMID_TITLE_POLISH,
    CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL,
    CELL_GAME_PARAMID_TITLE_ENGLISH_UK,
    CELL_GAME_PARAMID_TITLE_ID = 100,
    CELL_GAME_PARAMID_VERSION,
    CELL_GAME_PARAMID_PARENTAL_LEVEL,
    CELL_GAME_PARAMID_RESOLUTION,
    CELL_GAME_PARAMID_SOUND_FORMAT,
    CELL_GAME_PARAMID_PS3_SYSTEM_VER,
    CELL_GAME_PARAMID_APP_VER
};

struct SFOEntry {
    uint32_t id;
    std::string key;
    boost::variant<std::string, uint32_t> data;
};

class ContentManager {
    std::string _elfPath;
    std::vector<SFOEntry> _sfo;

public:
    void setElfPath(std::string_view path);
    std::string usrDir();
    std::string contentDir();
    std::string toHost(std::string_view path);
    std::string cacheDir();
    const std::vector<SFOEntry>& sfo();
};
