#include "logger.hpp"
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/file_sinks.h>

#ifdef _WIN32
#include <spdlog/sinks/wincolor_sink.h>
#else
#include <spdlog/sinks/ansicolor_sink.h>
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
#include <spdlog/sinks/msvc_sink.h>
#endif  // _DEBUG && _MSC_VER

fetch::oef::Logger::Logger(std::string section) : section_{std::move(section)} {
  logger_ = spdlog::get(fetch::oef::Logger::logger_name);

  if (logger_ == nullptr) {
#ifdef _WIN32
    auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
    auto dist_sink = std::make_shared<spdlog::sinks::dist_sink_st>();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("log.txt", 1024*1024*10, 10);
    dist_sink->add_sink(color_sink);
    dist_sink->add_sink(rotating_sink);
#if defined(_DEBUG) && defined(_MSC_VER)
    auto debug_sink = std::make_shared<spdlog::sinks::msvc_sink_st>();
    dist_sink->add_sink(debug_sink);
#endif  // _DEBUG && _MSC_VER
    logger_ = spdlog::details::registry::instance().create(fetch::oef::Logger::logger_name, dist_sink);
  }
}
