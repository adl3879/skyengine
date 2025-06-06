#pragma once

#include <skypch.h>

namespace sky
{
class TaskBase
{
  public:
    virtual ~TaskBase() = default;
    virtual void run() = 0; // Pure virtual function for running the task.
    virtual const std::string &getId() const = 0;
};

template <typename Result> 
class Task : public TaskBase
{
  public:
    enum class Status
    {
        Pending,
        Running,
        Completed,
        Failed
    };

    Task(const std::string &id, std::function<Result()> func) : m_id(id), m_func(func), m_status(Status::Pending) {}

    void run() override
    {
        {
            std::unique_lock lock(m_mutex);
            m_status = Status::Running;
        }
    
        try
        {
            Result result = m_func(); // run outside lock
            {
                std::unique_lock lock(m_mutex);
                m_result = result;
                m_status = Status::Completed;
            }
    
            if (m_callback) m_callback(*m_result);
        }
        catch (const std::exception &)
        {
            std::unique_lock lock(m_mutex);
            m_status = Status::Failed;
            assert(false);
        }
    
        m_cv.notify_all();
    }

    void setCallback(std::function<void(const Result &)> callback) { m_callback = callback; }

    Status getStatus() const { return m_status; }
    const std::string &getId() const override { return m_id; }

    std::optional<Result> getResult() const
    {
        // Return result only if the task completed successfully.
        if (m_status == Status::Completed) return m_result;
        return std::nullopt;
    }

    void wait()
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [this] { return m_status == Status::Completed || m_status == Status::Failed; });
    }

    static Ref<Task<Result>> create(const std::string &id, std::function<Result()> func)
    {
        return CreateRef<Task<Result>>(id, func);
    }

  private:
    std::string m_id;
    std::function<Result()> m_func;
    std::optional<Result> m_result; // Store the result.
    std::function<void(const Result &)> m_callback;
    Status m_status;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
};
} 
// namespace sky