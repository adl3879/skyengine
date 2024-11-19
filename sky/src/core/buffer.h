#pragma once

#include <skypch.h>

namespace sky
{
// Non-owning raw buffer class
struct Buffer
{
    uint8_t *data = nullptr;
    uint64_t size = 0;

    Buffer() = default;
    Buffer(uint64_t size) { allocate(size); }
    Buffer(const void *data, uint64_t size) : data((uint8_t *)data), size(size) {}
    Buffer(const Buffer &) = default;

    static Buffer copy(Buffer other)
    {
        Buffer result(other.size);
        memcpy(result.data, other.data, other.size);
        return result;
    }

    void allocate(uint64_t size)
    {
        release();
        data = (uint8_t *)malloc(size);
        size = size;
    }

    void release()
    {
        free(data);
        data = nullptr;
        size = 0;
    }

    template <typename T> 
    T *As() 
    { 
        return (T *)data;
    }

    operator bool() const { return (bool)data; }
};

struct ScopedBuffer
{
    ScopedBuffer(Buffer buffer) : m_buffer(buffer) {}
    ScopedBuffer(uint64_t size) : m_buffer(size) {}
    ~ScopedBuffer() { m_buffer.release(); }

    uint8_t *data() { return m_buffer.data; }
    uint64_t size() { return m_buffer.size; }

    template <typename T> 
    T *As() 
    { 
        return m_buffer.As<T>();
    }

    operator bool() const { return m_buffer; }

  private:
    Buffer m_buffer;
};
} // namespace sky