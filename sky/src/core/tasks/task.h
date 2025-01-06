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
        m_status = Status::Running;
        try
        {
            m_result = m_func(); // Execute the task function and store the result.
            m_status = Status::Completed;

            // Execute the callback after completion
            if (m_callback)
            {
                m_callback(*m_result);
            }
        }
        catch (const std::exception &)
        {
            m_status = Status::Failed;
            assert(false);
        }
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

  private:
    std::string m_id;
    std::function<Result()> m_func;
    std::optional<Result> m_result; // Store the result.
    std::function<void(const Result &)> m_callback;
    Status m_status;
};
} 
// namespace sky