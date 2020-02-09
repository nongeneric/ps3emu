#pragma once

#include "ps3emu/enum.h"
#include <string>

enum log_level_t { log_detail, log_info, log_warning, log_error, log_level_none };

ENUM(log_type_t,
     (spu, 0),
     (perf, 1),
     (cache, 2),
     (sync, 3),
     (audio, 4),
     (fs, 5),
     (proxy, 6),
     (rsx, 7),
     (libs, 8),
     (debugger, 9));

enum log_sink_t { log_file = 1, log_console = 2 };
enum log_format_t { log_date, log_simple };

void log_init(int sinks, std::string config, log_format_t format);
void log_set_thread_name(std::string name);
void log_unconditional(log_level_t level,
                       log_type_t type,
                       const char* message);
inline void log_unconditional(log_level_t level,
                              log_type_t type,
                              std::string const& message) {
    log_unconditional(level, type, message.c_str());
}

#ifdef LOG_ENABLED
bool log_should(log_level_t level, log_type_t type);
#else
#define log_should(a, b, c) false
#endif

class log_sink {
    log_level_t _level;
    log_type_t _type;
public:
    log_sink(log_level_t level, log_type_t type)
        : _level(level), _type(type) {}
    template <typename T>
    log_sink& operator<<(T&& message) {
        log_unconditional(_level, _type, message);
        return *this;
    }
};

#define LOGMSG_IMPL(level, type) \
    if (!log_should(level, type)) (void)0; \
        else log_sink(level, type)

#define INFO(type) LOGMSG_IMPL(log_info, log_type_t:: type)
#define WARNING(type) LOGMSG_IMPL(log_warning, log_type_t:: type)
#define ERROR(type) LOGMSG_IMPL(log_error, log_type_t:: type)
#define DETAIL(type) LOGMSG_IMPL(log_detail, log_type_t:: type)

log_sink_t log_parse_sinks(std::string const& str);
log_format_t log_parse_format(std::string const& str);
std::string print_backtrace();
