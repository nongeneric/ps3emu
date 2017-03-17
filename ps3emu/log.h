#pragma once

#include <string>

enum log_severity_t { log_info, log_warning, log_error };
enum log_type_t { log_spu = 1, log_rsx = 2, log_libs = 4, log_debugger = 8, log_perf = 16 };
enum log_sink_t { log_file = 1, log_console = 2 };

void log_init(int sinks, log_severity_t severity, int types, bool date);
void log_set_thread_name(std::string name);
void log_unconditional(log_severity_t severity,
                       log_type_t type,
                       const char* message);
inline void log_unconditional(log_severity_t severity,
                              log_type_t type,
                              std::string const& message) {
    log_unconditional(severity, type, message.c_str());
}
bool log_should(log_severity_t severity, log_type_t type);

class log_sink {
    log_severity_t _severity;
    log_type_t _type;
public:
    log_sink(log_severity_t severity, log_type_t type)
        : _severity(severity), _type(type) {}
    template <typename T>
    log_sink& operator<<(T&& message) {
        log_unconditional(_severity, _type, message);
        return *this;
    }
};

#define LOG LOGMSG(info, libs)
#define LOGMSG(severity, type) \
    if (!log_should(log_##severity, log_##type)) (void)0; \
        else log_sink(log_##severity, log_##type)
#define INFO(type) LOGMSG(info, type)
#define WARNING(type) LOGMSG(warning, type)
#define ERROR(type) LOGMSG(error, type)

#define PARSE(s, res, name) if (s == #name) { res |= log_##name; } else

log_sink_t log_parse_sinks(std::string const& str);
log_type_t log_parse_filter(std::string const& str);
log_severity_t log_parse_verbosity(std::string const& str);
std::string print_backtrace();
