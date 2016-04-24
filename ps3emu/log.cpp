#include "log.h"

#include <iostream>
#include <boost/log/core.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

using namespace boost::log;

namespace {
    thread_local std::string thread_name;
}

void log_init(bool file_only) {
    add_common_attributes();
    core::get()->add_global_attribute(
        "ThreadName", attributes::make_function([] { return thread_name; }));
    
    auto format = (expressions::stream
                   << expressions::format_date_time<boost::posix_time::ptime>(
                          "TimeStamp", "%H:%M:%S.%f")
                   << " ["
                   << expressions::attr<std::string>("ThreadName")
                   << "] "
                   << expressions::smessage);

    if (!file_only) {
        add_console_log(
            std::cout, keywords::auto_flush = true, keywords::format = format);
    }
    
    add_file_log(keywords::file_name = "/tmp/ps3run.log",
                keywords::auto_flush = true,
                keywords::format = format);
}

void log_set_thread_name(std::string name) {
    thread_name = name;
}