#pragma once

struct DbgConfig {
    bool StopAtNewPpuThread = true;
    bool StopAtNewSpuThread = true;
    bool StopAtNewModule = false;
    bool LogSpu = true;
    bool LogRsx = true;
    bool EnableSpursTrace = true;
    bool LogDates = false;
    
    template<class Archive>
    void serialize(Archive& ar) {
        ar(StopAtNewPpuThread,
           StopAtNewSpuThread,
           StopAtNewModule,
           LogSpu,
           LogRsx,
           EnableSpursTrace,
           LogDates);
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
