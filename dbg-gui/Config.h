#pragma once

struct DbgConfig {
    bool StopAtNewPpuThread = true;
    bool StopAtNewSpuThread = true;
    bool LogSpu = true;
    
    template<class Archive>
    void serialize(Archive& ar) {
        ar(StopAtNewPpuThread, StopAtNewSpuThread, LogSpu);
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
