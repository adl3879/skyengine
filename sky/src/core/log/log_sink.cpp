#include "log_sink.h"

namespace sky
{
void LogSink::sink_it_(const spdlog::details::log_msg &msg) 
{
    spdlog::memory_buf_t formatted;
    base_sink<std::mutex>::formatter_->format(msg, formatted);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logEntries.emplace_back(LogEntry{msg.level, fmt::to_string(formatted)});

    // Limit stored logs (optional, to prevent unbounded growth)
    if (m_logEntries.size() > m_maxEntries)
    {
        m_logEntries.pop_front();
    }
}

void LogSink::flush_() 
{
}
} // namespace sky