#pragma once

#include <skypch.h>

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

#include "log_sink.h"

namespace sky
{
class Log
{
  public:
    static void Init();

    inline static Ref<spdlog::logger> &getCoreLogger() { return s_coreLogger; }
    inline static Ref<spdlog::logger> &getClientLogger() { return s_clientLogger; }
    inline static Ref<LogSink> &getLogSink() { return s_logSink; }

  private:
    static Ref<spdlog::logger> s_coreLogger;
    static Ref<spdlog::logger> s_clientLogger;
    static Ref<LogSink> s_logSink;

};
} // namespace sky

#define SKY_CORE_TRACE(...) ::sky::Log::getCoreLogger()->trace(__VA_ARGS__)
#define SKY_CORE_INFO(...) ::sky::Log::getCoreLogger()->info(__VA_ARGS__)
#define SKY_CORE_WARN(...) ::sky::Log::getCoreLogger()->warn(__VA_ARGS__)
#define SKY_CORE_ERROR(...) ::sky::Log::getCoreLogger()->error(__VA_ARGS__)
#define SKY_CORE_CRITICAL(...) ::sky::Log::getCoreLogger()->critical(__VA_ARGS__)

#define SKY_TRACE(...) ::sky::Log::getClientLogger()->trace(__VA_ARGS__)
#define SKY_INFO(...) ::sky::Log::getClientLogger()->info(__VA_ARGS__)
#define SKY_WARN(...) ::sky::Log::getClientLogger()->warn(__VA_ARGS__)
#define SKY_ERROR(...) ::sky::Log::getClientLogger()->error(__VA_ARGS__)
#define SKY_CRITICAL(...) ::sky::Log::getClientLogger()->critical(__VA_ARGS__)