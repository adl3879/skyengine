#include "log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace sky
{
Ref<spdlog::logger> Log::s_coreLogger;
Ref<spdlog::logger> Log::s_clientLogger;

void Log::Init()
{
    spdlog::set_pattern("%^[%T] %n: %v%$");
    s_coreLogger = spdlog::stdout_color_mt("Engine");
    s_coreLogger->set_level(spdlog::level::trace);

    s_clientLogger = spdlog::stdout_color_mt("App");
    s_clientLogger->set_level(spdlog::level::trace);
}
} // namespace sky