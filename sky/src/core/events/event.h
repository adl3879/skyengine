#pragma once

#include <functional>

namespace sky
{

// Events in Hazel are currently blocking, meaning when an event occurs it
// immediately gets dispatched and must be dealt with right then an there.
// For the future, a better strategy might be to buffer events in an event
// bus and process them during the "event" part of the update stage.

enum class EventType
{
    None = 0,
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    AppTick,
    AppUpdate,
    AppRender,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

enum EventCategory
{
    None = 0,
    EventCategoryApplication = 1 << 0,
    EventCategoryInput = 1 << 1,
    EventCategoryKeyboard = 1 << 2,
    EventCategoryMouse = 1 << 3,
    EventCategoryMouseButton = 1 << 4
};

#define EVENT_CLASS_TYPE(type)                                                                                         \
    static EventType getStaticType() { return EventType::type; }                                                       \
    virtual EventType getEventType() const override { return getStaticType(); }                                        \
    virtual const char *getName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category)                                                                                 \
    virtual int getCategoryFlags() const override { return category; }

class Event
{
  public:
    virtual ~Event() = default;

    bool handled = false;

    virtual EventType getEventType() const = 0;
    virtual const char *getName() const = 0;
    virtual int getCategoryFlags() const = 0;
    virtual std::string toString() const { return getName(); }

    bool isInCategory(EventCategory category) { return getCategoryFlags() & category; }
};

class EventDispatcher
{
  public:
    EventDispatcher(Event &event) : m_event(event) {}

    // F will be deduced by the compiler
    template <typename T, typename F> bool dispatch(const F &func)
    {
        if (m_event.getEventType() == T::getStaticType())
        {
            m_event.handled |= func(static_cast<T &>(m_event));
            return true;
        }
        return false;
    }

  private:
    Event &m_event;
};

inline std::ostream &operator<<(std::ostream &os, const Event &e) { return os << e.toString(); }

} // namespace sky
