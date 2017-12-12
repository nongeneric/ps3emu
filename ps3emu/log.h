#pragma once

#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/cat.hpp>

#include <string>

enum log_severity_t { log_info, log_warning, log_error };
enum log_area_t { log_trace = 1, log_perf = 2, log_cache = 4, log_sync = 8 };
enum log_type_t { log_spu = 1, log_rsx = 2, log_libs = 4, log_debugger = 8 };
enum log_sink_t { log_file = 1, log_console = 2 };
enum log_format_t { log_date, log_simple };

void log_init(int sinks, log_severity_t severity, int types, int areas, log_format_t format);
void log_set_thread_name(std::string name);
void log_unconditional(log_severity_t severity,
                       log_type_t type,
                       log_area_t area,
                       const char* message);
inline void log_unconditional(log_severity_t severity,
                              log_type_t type,
                              log_area_t area,
                              std::string const& message) {
    log_unconditional(severity, type, area, message.c_str());
}

#ifdef LOG_ENABLED
bool log_should(log_severity_t severity, log_type_t type, log_area_t area);
#else
#define log_should(a, b, c) false
#endif

class log_sink {
    log_severity_t _severity;
    log_type_t _type;
    log_area_t _area;
public:
    log_sink(log_severity_t severity, log_type_t type, log_area_t area)
        : _severity(severity), _type(type), _area(area) {}
    template <typename T>
    log_sink& operator<<(T&& message) {
        log_unconditional(_severity, _type, _area, message);
        return *this;
    }
};

#define LOGMSG_IMPL(severity, type, area) \
    if (!log_should(severity, type, area)) (void)0; \
        else log_sink(severity, type, area)
#define LOGMSG(...) \
    BOOST_PP_IF( \
        BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), 3), \
            LOGMSG_IMPL( \
                BOOST_PP_CAT(log_, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)), \
                BOOST_PP_CAT(log_, BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__)), \
                BOOST_PP_CAT(log_, BOOST_PP_VARIADIC_ELEM(2, __VA_ARGS__))), \
            LOGMSG_IMPL( \
                BOOST_PP_CAT(log_, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)), \
                BOOST_PP_CAT(log_, BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__)), \
                log_trace) \
    )
#define INFO(...) LOGMSG(info, __VA_ARGS__)
#define WARNING(...) LOGMSG(warning, __VA_ARGS__)
#define ERROR(...) LOGMSG(error, __VA_ARGS__)

#define PARSE(s, res, name) if (s == #name) { res |= log_##name; } else

log_sink_t log_parse_sinks(std::string const& str);
log_type_t log_parse_filter(std::string const& str);
log_severity_t log_parse_verbosity(std::string const& str);
log_format_t log_parse_format(std::string const& str);
log_area_t log_parse_area(std::string const& str);
std::string print_backtrace();
