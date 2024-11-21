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
                    Task *task;
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
    for (std::thread &worker : m_workers) worker.join();
}

void TaskManager::submitTask(const Ref<Task> &task) 
{
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_tasks.push(task.get());
    }
    m_taskMap[task->getId()] = task;
    m_condition.notify_one();
}

Ref<Task> TaskManager::getTask(const std::string &id) 
{
    std::unique_lock<std::mutex> lock(m_queueMutex);
    return m_taskMap.count(id) ? m_taskMap[id] : nullptr;
}
} // namespace sky