#include "task.h"

namespace sky
{
Task::Task(const std::string &id, std::function<void()> func) 
 : m_id(id), m_func(std::move(func)), m_status(Status::Pending)
{
}

void Task::run()
{
    m_status = Status::Running;
    try
    {
        m_func();
        m_status = Status::Completed;
    }
    catch (...)
    {
        m_status = Status::Failed;
    }
}
}
