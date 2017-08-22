#pragma once

struct DbgConfig {
    bool StopAtNewPpuThread = true;
    bool StopAtNewSpuThread = true;
    bool StopAtNewModule = false;
    bool LogSpu = true;
    bool LogRsx = true;
    bool LogLibs = true;
    bool LogCache = true;
    bool CaptureRsx = true;
    bool EnableSpursTrace = true;
    bool LogDates = false;
    bool LogSync = false;
    
    template<class Archive>
    void serialize(Archive& ar) {
        ar(StopAtNewPpuThread,
           StopAtNewSpuThread,
           StopAtNewModule,
           LogSpu,
           LogRsx,
           LogLibs,
           LogCache,
           CaptureRsx,
           EnableSpursTrace,
           LogDates,
           LogSync);
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
