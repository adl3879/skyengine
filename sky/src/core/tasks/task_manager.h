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

    void submitTask(const Ref<Task> &task);
    Ref<Task> getTask(const std::string &id);

  private:
    std::vector<std::thread> m_workers;
    std::queue<Task *> m_tasks;
    std::unordered_map<std::string, std::shared_ptr<Task>> m_taskMap;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    bool m_stop;
};
}