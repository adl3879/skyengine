#pragma once

#include <skypch.h>

namespace sky
{
class UUID
{
  public:
    UUID();
    UUID(uint64_t uuid);
    UUID(const UUID &other) = default;

    static const UUID generate();

    const std::string toString() const;

    operator uint64_t() const { return m_uuid; }

  private:
    uint64_t m_uuid;
};

static const UUID NULL_UUID = 0;
} // namespace sky

namespace std
{
template <> struct hash<sky::UUID>
{
    size_t operator()(const sky::UUID &uuid) const
    {
        return hash<uint64_t>()(static_cast<uint64_t>(uuid));
    }
};
}