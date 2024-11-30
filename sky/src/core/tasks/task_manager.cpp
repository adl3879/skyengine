#include "task_manager.h"

namespace sky
{
TaskManager::TaskManager(size_t threadCount) : m_stop(false)
{
    for (size_t i = 0; i < threadCount; ++i)
    {
        m_workers.emplace_back(
            [this]
            {
                while (true)
                {
                    std::shared_ptr<TaskBase> task;
                    {
                        std::unique_lock<std::mutex> lock(m_queueMutex);
                        m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                        if (m_stop && m_tasks.empty()) return;
                        task = m_tasks.front();
                        m_tasks.pop();
                    }
                    task->run();
                }
            });
    }
}

TaskManager::~TaskManager()
{
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    m_condition.notify_all();
    for (std::thread &worker : m_workers)
    {
        worker.join();
    }
}
} // namespace sky