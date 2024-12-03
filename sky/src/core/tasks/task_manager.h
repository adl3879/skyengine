#pragma once

#include <skypch.h>
#include "task.h"

namespace sky
{
class TaskManager
{
  public:
    TaskManager(size_t threadCount = std::thread::hardware_concurrency());
    ~TaskManager();
   
    template <typename Result> void submitTask(const Ref<Task<Result>> &task)
    {
        Ref<TaskBase> baseTask = std::static_pointer_cast<TaskBase>(task);
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_tasks.push(baseTask);
        }
        {
            std::unique_lock<std::mutex> lock(m_taskMapMutex);
            m_taskMap[task->getId()] = std::static_pointer_cast<TaskBase>(task);
        }
        m_condition.notify_one();
    }

    template <typename Result> Ref<Task<Result>> getTask(const std::string &id)
    {
        std::unique_lock<std::mutex> lock(m_taskMapMutex);
        if (m_taskMap.count(id))
        {
            return std::dynamic_pointer_cast<Task<Result>>(m_taskMap[id]);
        }
        return nullptr;
    }

  private:
    bool m_stop;
    std::vector<std::thread> m_workers;
    std::queue<Ref<TaskBase>> m_tasks;
    std::unordered_map<std::string, Ref<TaskBase>> m_taskMap;
    std::mutex m_queueMutex;
    std::mutex m_taskMapMutex;
    std::condition_variable m_condition;
};
}