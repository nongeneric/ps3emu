#include "log.h"
#include "utils.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <spdlog/spdlog.h>

namespace {
    thread_local std::string thread_name;
    log_severity_t active_severity;
    int active_types;
    std::shared_ptr<spdlog::logger> logger;
}

void log_init(int sink_flags, log_severity_t severity, int types, bool date) {
    active_severity = severity;
    active_types = types;

    if (boost::filesystem::exists("/tmp/ps3.log"))
        boost::filesystem::remove("/tmp/ps3.log");

    std::vector<spdlog::sink_ptr> sinks;
    if (sink_flags & log_console) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    }
    if (sink_flags & log_file) {
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "/tmp/ps3", "log", (1ull << 30) * 128, 1, true));
    }
    logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
    spdlog::register_logger(logger);
    if (date) {
        spdlog::set_pattern("%M:%S.%f %v");
    } else {
        spdlog::set_pattern("%v");
    }
}

void log_set_thread_name(std::string name) {
    thread_name = "[" + name + "] ";
}

void log_unconditional(log_severity_t severity, log_type_t type, const char* message) {
    auto const& formatted = ssnprintf("%s %s%s: %s",
                                      type == log_spu ? "SPU" :
                                      type == log_libs ? "LIB" :
                                      type == log_debugger ? "DBG" :
                                      type == log_rsx ? "RSX"
                                      : "?",
                                      thread_name,
                                      severity == log_info ? "INFO" :
                                      severity == log_warning ? "WARNING" :
                                      severity == log_error ? "ERROR"
                                      : "?",
                                      message);
    logger->info(formatted);
}

bool log_should(log_severity_t severity, log_type_t type) {
    return severity >= active_severity && (type & active_types);
}

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
        throw std::runtime_error("unknown logging sink");
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
        throw std::runtime_error("unknown logging filter");
    }
    return static_cast<log_type_t>(res);
}

log_severity_t log_parse_verbosity(std::string const& str) {
    int res = 0;
    PARSE(str, res, info)
    PARSE(str, res, warning)
    PARSE(str, res, error)
    throw std::runtime_error("unknown logging verbosity");
    return static_cast<log_severity_t>(res);
}

#undef PARSE
