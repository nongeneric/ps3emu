#pragma once

#include <string>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/logger.hpp>

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(logger, boost::log::sources::logger_mt)
#define LOG BOOST_LOG(logger::get())

void log_init(bool file_only);
void log_set_thread_name(std::string name);