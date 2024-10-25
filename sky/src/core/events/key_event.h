#pragma once

#include "event.h"
#include "key_codes.h"

namespace sky
{
class KeyEvent : public Event
{
  public:
    KeyCode getKeyCode() const { return m_keyCode; }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
  protected:
    KeyEvent(const KeyCode keycode) : m_keyCode(keycode) {}

    KeyCode m_keyCode;
};

class KeyPressedEvent : public KeyEvent
{
  public:
    KeyPressedEvent(const KeyCode keycode, bool isRepeat = false) : KeyEvent(keycode), m_isRepeat(isRepeat) {}

    bool isRepeat() const { return m_isRepeat; }

    std::string toString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_keyCode << " (repeat = " << m_isRepeat << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed)
  private:
    bool m_isRepeat;
};

class KeyReleasedEvent : public KeyEvent
{
  public:
    KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

    std::string toString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << m_keyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent
{
  public:
    KeyTypedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

    std::string toString() const override
    {
        std::stringstream ss;
        ss << "KeyTypedEvent: " << m_keyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyTyped)
};
} // namespace Hazel