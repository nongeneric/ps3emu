#pragma once

struct DbgConfig {
    bool StopAtNewSpuThread = true;
    bool LogSpu = true;
    
    template<class Archive>
    void serialize(Archive& ar) {
        ar(StopAtNewSpuThread);
    }
};

class DbgConfigLoader {
    DbgConfig _config;
    
public:
    DbgConfig& config();
    void load();
    void save();
};

extern DbgConfigLoader g_config;
