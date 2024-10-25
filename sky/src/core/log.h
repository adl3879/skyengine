#pragma once

#include <skypch.h>

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace sky
{
class Log
{
  public:
    static void Init();

    inline static Ref<spdlog::logger> &GetCoreLogger() { return s_coreLogger; }
    inline static Ref<spdlog::logger> &GetClientLogger() { return s_clientLogger; }

  private:
    static Ref<spdlog::logger> s_coreLogger;
    static Ref<spdlog::logger> s_clientLogger;
};
} // namespace sky

#define SKY_CORE_TRACE(...) ::sky::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define SKY_CORE_INFO(...) ::sky::Log::GetCoreLogger()->info(__VA_ARGS__)
#define SKY_CORE_WARN(...) ::sky::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define SKY_CORE_ERROR(...) ::sky::Log::GetCoreLogger()->error(__VA_ARGS__)
#define SKY_CORE_CRITICAL(...) ::sky::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define SKY_TRACE(...) ::sky::Log::GetClientLogger()->trace(__VA_ARGS__)
#define SKY_INFO(...) ::sky::Log::GetClientLogger()->info(__VA_ARGS__)
#define SKY_WARN(...) ::sky::Log::GetClientLogger()->warn(__VA_ARGS__)
#define SKY_ERROR(...) ::sky::Log::GetClientLogger()->error(__VA_ARGS__)
#define SKY_CRITICAL(...) ::sky::Log::GetClientLogger()->critical(__VA_ARGS__)