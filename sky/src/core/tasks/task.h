#pragma once

#include <skypch.h>

namespace sky
{
class Task
{
  public:
    enum class Status
    {
        Pending,
        Running,
        Completed,
        Failed
    };

    Task(const std::string &id, std::function<void()> func);

    void run();

    Status getStatus() const { return m_status; }
    const std::string &getId() const { return m_id; }

  private:
    std::string m_id;
    std::function<void()> m_func;
    Status m_status;
};
}