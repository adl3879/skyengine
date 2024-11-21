#include "log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace sky
{
Ref<spdlog::logger> Log::s_coreLogger;
Ref<spdlog::logger> Log::s_clientLogger;
Ref<LogSink>        Log::s_logSink;

void Log::Init()
{
    // Create the custom log sink
    s_logSink = CreateRef<LogSink>();

    // Create a console sink
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // Set the log pattern for the console
    consoleSink->set_pattern("%^[%T] %n: %v%$");

    // Combine sinks into a multi-sink logger
    std::vector<spdlog::sink_ptr> sinks = {consoleSink, s_logSink};

    // Create core logger
    s_coreLogger = std::make_shared<spdlog::logger>("Engine", sinks.begin(), sinks.end());
    s_coreLogger->set_level(spdlog::level::trace);
    spdlog::register_logger(s_coreLogger);

    // Create client logger
    s_clientLogger = std::make_shared<spdlog::logger>("App", sinks.begin(), sinks.end());
    s_clientLogger->set_level(spdlog::level::trace);
    spdlog::register_logger(s_clientLogger);
}
} // namespace sky