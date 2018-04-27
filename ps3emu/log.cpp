#include "log.h"
#include "utils.h"
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <spdlog/spdlog.h>
#include <sys/prctl.h>
#include "ps3emu/ppu/PPUThread.h"
#include "ps3emu/spu/SPUThread.h"
#include "ps3emu/state.h"
#include "ps3emu/profiler.h"
#include <execinfo.h>
#include <stdio.h>

namespace {
    thread_local std::string thread_name;
    log_severity_t active_severity;
    int active_types;
    int active_areas;
    std::shared_ptr<spdlog::logger> logger;
}

void log_init(int sink_flags, log_severity_t severity, int types, int areas, log_format_t format) {
    active_severity = severity;
    active_types = types;
    active_areas = areas;

    auto fileName = "/tmp/ps3.log";
    fclose(fopen(fileName, "w+"));

    std::vector<spdlog::sink_ptr> sinks;
    if (sink_flags & log_console) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    }
    if (sink_flags & log_file) {
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            fileName, (1ull << 30u) * 128, 1));
    }
    logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
    spdlog::register_logger(logger);
    if (format == log_date) {
        spdlog::set_pattern("%M:%S.%f %v");
    } else {
        spdlog::set_pattern("%v");
    }
}

void log_set_thread_name(std::string name) {
    thread_name = "[" + name + "] ";
    prctl(PR_SET_NAME, (unsigned long)name.c_str(), 0, 0, 0);
    __itt_thread_set_name(name.c_str());
}

void log_unconditional(log_severity_t severity, log_type_t type, log_area_t area, const char* message) {
    std::string nip;
    if (g_state.th) {
        nip = ssnprintf("%08x ", g_state.th->getNIP());
    } else if (g_state.sth) {
        nip = ssnprintf("%08x ", g_state.sth->getNip());
    }
    std::string backtrace;
    if (severity == log_error) {
        backtrace = print_backtrace();
    }
    auto const& formatted = ssnprintf("%s %s%s%s: %s%s",
                                      type == log_spu ? "SPU" :
                                      type == log_libs ? "LIB" :
                                      type == log_debugger ? "DBG" :
                                      type == log_rsx ? "RSX"
                                      : "?",
                                      thread_name,
                                      nip,
                                      severity == log_info ? "INFO" :
                                      severity == log_warning ? "WARNING" :
                                      severity == log_error ? "ERROR"
                                      : "?",
                                      message,
                                      backtrace);
    logger->info(formatted);
}

#ifdef LOG_ENABLED
bool log_should(log_severity_t severity, log_type_t type, log_area_t area) {
    return severity == log_error || (severity >= active_severity &&
                                     (type & active_types) && (area & active_areas));
}
#endif

#define PARSE(s, res, name) if (s == #name) { res |= log_##name; } else

log_sink_t log_parse_sinks(std::string const& str) {
    std::vector<std::string> vec;
    int res = 0;
    boost::split(vec, str, boost::is_any_of(","), boost::token_compress_on);
    for (auto& s : vec) {
        if (s == "")
            continue;
        PARSE(s, res, file)
        PARSE(s, res, console)
        throw std::runtime_error("unknown log sink");
    }
    return static_cast<log_sink_t>(res);
}

log_type_t log_parse_filter(std::string const& str) {
    std::vector<std::string> vec;
    int res = 0;
    boost::split(vec, str, boost::is_any_of(","), boost::token_compress_on);
    for (auto& s : vec) {
        if (s == "")
            continue;
        PARSE(s, res, spu)
        PARSE(s, res, rsx)
        PARSE(s, res, libs)
        PARSE(s, res, debugger)
        PARSE(s, res, perf)
        PARSE(s, res, cache)
        throw std::runtime_error("unknown log filter");
    }
    return static_cast<log_type_t>(res);
}

log_severity_t log_parse_verbosity(std::string const& str) {
    int res = 0;
    PARSE(str, res, info)
    PARSE(str, res, warning)
    PARSE(str, res, error)
    throw std::runtime_error("unknown log verbosity");
    return static_cast<log_severity_t>(res);
}

log_format_t log_parse_format(std::string const& str) {
    int res = 0;
    PARSE(str, res, date)
    PARSE(str, res, simple)
    throw std::runtime_error("unknown log format");
    return static_cast<log_format_t>(res);
}

log_area_t log_parse_area(std::string const& str) {
    int res = 0;
    PARSE(str, res, trace)
    PARSE(str, res, perf)
    PARSE(str, res, cache)
    throw std::runtime_error("unknown log area");
    return static_cast<log_area_t>(res);
}

#undef PARSE

std::string print_backtrace() {
    void* array[40];
    auto size = backtrace(array, 40);
    auto frames = backtrace_symbols(array, size);
    std::string message;
    for (auto i = 0; i < size; ++i) {
        message += "  ";
        message += frames[i];
        message += "\n";
    }
    return message;
}
