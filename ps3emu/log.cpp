#include "log.h"

#include "ps3emu/ppu/PPUThread.h"
#include "ps3emu/profiler.h"
#include "ps3emu/spu/SPUThread.h"
#include "ps3emu/state.h"
#include "utils.h"

#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm.hpp>
#include <execinfo.h>

#include <filesystem>
#include <stdio.h>
#include <sys/prctl.h>

log_type_t log_parse_type(std::string const& str);

namespace {

using Levels = std::array<log_level_t, enum_traits<log_type_t>::size()>;

consteval Levels makeDefaultLevels() {
    Levels result;
    std::fill(begin(result), end(result), log_level_none);
    return result;
}

thread_local std::string thread_name;
std::shared_ptr<spdlog::logger> logger;
constinit Levels levels = makeDefaultLevels();

void parse_config(std::string config) {
    std::vector<std::string> vec;
    boost::split(vec, config, boost::is_any_of(","), boost::token_compress_on);
    for (auto& part : vec) {
        if (part.empty())
            continue;

        std::optional<log_type_t> type;
        auto level = log_level_none;

        switch (part[0]) {
            case '-': level = log_level_none; break;
            case 'E': level = log_error; break;
            case 'W': level = log_warning; break;
            case 'I': level = log_info; break;
            case 'D': level = log_detail; break;
            default: throw std::runtime_error("parse error");
        }

        if (part.size() > 1) {
            type = parse_enum<log_type_t>(part.c_str() + 1);
        }

        if (type) {
            levels[static_cast<int>(*type)] = level;
        } else {
            boost::fill(levels, level);
        }
    }
}

}

void log_init(int sink_flags, std::string config, log_format_t format) {
    parse_config(config);

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

    spdlog::set_pattern("%v");
    logger->info("ps3emu log");

    if (format == log_date) {
        spdlog::set_pattern("%M:%S.%f %v");
    } else {
        spdlog::set_pattern("%v");
    }
    spdlog::flush_every(std::chrono::seconds(3));
}

void log_set_thread_name(std::string name) {
    thread_name = "[" + name + "] ";
    prctl(PR_SET_NAME, (unsigned long)name.c_str(), 0, 0, 0);
    __itt_thread_set_name(name.c_str());
}

void log_unconditional(log_level_t level, log_type_t type, const char* message) {
    std::string nip;
    if (g_state.th) {
        nip = ssnprintf("%08x ", g_state.th->getNIP());
    } else if (g_state.sth) {
        nip = ssnprintf("%08x ", g_state.sth->getNip());
    }
    std::string backtrace;
    if (level == log_error) {
        backtrace = print_backtrace();
    }
    auto levelStr = level == log_info ? "INFO"
                  : level == log_warning ? "WARNING"
                  : level == log_error ? "ERROR"
                  : level == log_detail ? "DETAIL"
                  : "?";
    auto const& formatted = ssnprintf("%s %s%s%s: %s%s",
                                      to_string(type).c_str() + sizeof("log"),
                                      thread_name,
                                      nip,
                                      levelStr,
                                      message,
                                      backtrace);
    logger->info(formatted);
}

#ifdef LOG_ENABLED
bool log_should(log_level_t level, log_type_t type) {
    return level >= levels[static_cast<int>(type)];
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

log_format_t log_parse_format(std::string const& str) {
    int res = 0;
    PARSE(str, res, date)
    PARSE(str, res, simple)
    throw std::runtime_error("unknown log format");
    return static_cast<log_format_t>(res);
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
