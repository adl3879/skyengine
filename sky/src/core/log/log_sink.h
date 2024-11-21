#pragma once

#include <skypch.h>
#include <spdlog/sinks/base_sink.h>

namespace sky
{
class LogSink : public spdlog::sinks::base_sink<std::mutex>
{
  public:
    struct LogEntry
    {
        spdlog::level::level_enum level;
        std::string message;
    };

    void sink_it_(const spdlog::details::log_msg &msg) override; 
    void flush_() override;

    std::deque<LogEntry> &getLogEntries() { return m_logEntries; }

  private:
    mutable std::mutex m_mutex;
    std::deque<LogEntry> m_logEntries;
    size_t m_maxEntries = 1000; // Adjust as needed
};
}